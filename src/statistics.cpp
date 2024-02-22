#include <shogi_camera_input/shogi_camera_input.hpp>

#include <opencv2/imgproc.hpp>

#include <iostream>

namespace sci {

namespace {

template <class T, class L>
bool IsIdentical(std::set<T, L> const &a, std::set<T, L> const &b) {
  if (a.size() != b.size()) {
    return false;
  }
  if (a.empty()) {
    return true;
  }
  for (auto const &i : a) {
    if (b.find(i) == b.end()) {
      return false;
    }
  }
  return true;
}

// 手番 color の駒が from から to に移動したとき, 成れる条件かどうか.
bool CanPromote(Square from, Square to, Color color) {
  if (color == Color::Black) {
    return from.rank <= Rank::Rank3 || to.rank <= Rank::Rank3;
  } else {
    return from.rank >= Rank::Rank7 || to.rank >= Rank::Rank7;
  }
}

void AppendPromotion(Move &mv, cv::Mat const &boardBefore, cv::Mat const &boardAfter, PieceBook &book) {
  if (!mv.from || IsPromotedPiece(mv.piece)) {
    return;
  }
  if (!CanPromote(*mv.from, mv.to, mv.color)) {
    return;
  }
  PieceBook::Entry &entry = book.store[RemoveColorFromPiece(mv.piece)];
  ClosedRange<double> range = entry.ensureSimilarityRange();
  // なる前の駒の画像コレクション同士の類似度の最低値より, 今回の駒画像の前後で比べた時の類似度が下回っている場合成りとみなす.
  // 既存の画像のどの駒画像にも似てない => 成りなのでは.
  auto bp = Img::PieceROI(boardBefore, mv.from->file, mv.from->rank);
  auto ap = Img::PieceROI(boardAfter, mv.to.file, mv.to.rank);
  double sim = Img::Similarity(bp, ap);
  std::cout << "sim=" << sim << "; range=[" << range.minimum << ", " << range.maximum << "]" << std::endl;
  if (sim < range.minimum) {
    mv.piece = Promote(mv.piece);
    mv.promote_ = true;
  } else {
    mv.promote_ = false;
  }
}

} // namespace

void Statistics::update(Status const &s) {
  int constexpr maxHistory = 32;
  if (s.squareArea > 0) {
    squareAreaHistory.push_back(s.squareArea);
    if (squareAreaHistory.size() > maxHistory) {
      squareAreaHistory.pop_front();
    }
    int count = 0;
    float sum = 0;
    for (float v : squareAreaHistory) {
      if (v > 0) {
        sum += v;
        count++;
      }
    }
    if (count > 0) {
      squareArea = sum / count;
    }
  }
  if (s.aspectRatio > 0) {
    aspectRatioHistory.push_back(s.aspectRatio);
    if (aspectRatioHistory.size() > maxHistory) {
      aspectRatioHistory.pop_front();
    }
    int count = 0;
    float sum = 0;
    for (float v : aspectRatioHistory) {
      if (v > 0) {
        sum += v;
        count++;
      }
    }
    if (count > 0) {
      aspectRatio = sum / count;
    }
  }
  if (s.preciseOutline) {
    preciseOutlineHistory.push_back(*s.preciseOutline);
    if (preciseOutlineHistory.size() > maxHistory) {
      preciseOutlineHistory.pop_front();
    }
    Contour sum;
    sum.points.resize(4);
    fill_n(sum.points.begin(), 4, cv::Point2f(0, 0));
    int count = 0;
    for (auto const &v : preciseOutlineHistory) {
      if (v.points.size() != 4) {
        continue;
      }
      for (int i = 0; i < 4; i++) {
        sum.points[i] += v.points[i];
      }
      count++;
    }
    for (int i = 0; i < 4; i++) {
      sum.points[i] = sum.points[i] / count;
    }
    sum.area = fabs(cv::contourArea(sum.points));
    preciseOutline = sum;
  }
}

void Statistics::push(cv::Mat const &board, Status &s, Game &g) {
  using namespace std;
  if (board.size().area() <= 0) {
    return;
  }
  BoardImage bi;
  bi.image = board;
  boardHistory.push_back(bi);
  if (boardHistory.size() == 1) {
    return;
  }
  int constexpr stableThresholdFrames = 3;
  int constexpr stableThresholdMoves = 3;
  // 盤面の各マスについて, 直前の画像との類似度を計算する. 将棋は 1 手につきたかだか 2 マス変動するはず. もし変動したマスが 3 マス以上なら,
  // 指が映り込むなどして盤面が正確に検出できなかった可能性がある.
  // 直前から変動した升目数が 0 のフレームが stableThresholdFrames フレーム連続した時, stable になったと判定する.
  BoardImage const &before = boardHistory[boardHistory.size() - 2];
  BoardImage const &after = boardHistory[boardHistory.size() - 1];
  CvPointSet changes;
  Img::Compare(before, after, changes, &s.similarity);
  if (!stableBoardHistory.empty()) {
    auto const &last = stableBoardHistory.back();
    CvPointSet tmp;
    Img::Compare(last[2], bi, tmp, &s.similarityAgainstStableBoard);
  }
  if (!changes.empty()) {
    // 変動したマス目が検出されているので, 最新のフレームだけ残して捨てる.
    boardHistory.clear();
    boardHistory.push_back(bi);
    moveCandidateHistory.clear();
    return;
  }
  // 直前の stable board がある場合, stable board と
  if (boardHistory.size() < stableThresholdFrames) {
    // まだ stable じゃない.
    return;
  }
  // stable になったと判定する. 直近 3 フレームを stableBoardHistory の候補とする.
  array<BoardImage, 3> history;
  for (int i = 0; i < history.size(); i++) {
    history[i] = boardHistory.back();
    boardHistory.pop_back();
  }
  if (stableBoardHistory.empty()) {
    // 最初の stable board なので登録するだけ.
    stableBoardHistory.push_back(history);
    boardHistory.clear();
    if (false) {
      cout << Img::EncodeToBase64(board) << endl;
    }
    return;
  }
  // 直前の stable board の各フレームと比較して, 変動したマス目が有るかどうか判定する.
  array<BoardImage, 3> &last = stableBoardHistory.back();
  int minChange = 81;
  int maxChange = -1;
  vector<CvPointSet> changeset;
  for (int i = 0; i < last.size(); i++) {
    for (int j = 0; j < history.size(); j++) {
      auto const &before = last[i];
      auto const &after = history[j];
      changes.clear();
      Img::Compare(before, after, changes);
      if (changes.size() == 0) {
        // 直前の stable board と比べて変化箇所が無い場合は無視.
        return;
      } else if (changes.size() > 2) {
        // 変化箇所が 3 以上ある場合, 将棋の駒以外の変化が盤面に現れているので無視.
        if (stableBoardHistory.size() == 1 && g.moves.empty()) {
          // まだ stable board が 1 個だけの場合, その stable board が間違った範囲を検出しているせいでずっとここを通過し続けてしまう可能性がある.
          stableBoardHistory.pop_back();
          stableBoardHistory.push_back(history);
        }
        return;
      }
      minChange = min(minChange, (int)changes.size());
      maxChange = max(maxChange, (int)changes.size());
      changeset.push_back(changes);
    }
  }
  if (changeset.empty() || minChange != maxChange) {
    // 有効な変化が発見できなかった
    return;
  }
  // changeset 内の変化位置が全て同じ部分を指しているか確認する. 違っていれば stable とはみなせない.
  for (int i = 1; i < changeset.size(); i++) {
    if (!IsIdentical(changeset[0], changeset[i])) {
      return;
    }
  }
  // index 番目の手.
  size_t const index = g.moves.size();
  Color const color = ColorFromIndex(index);
  CvPointSet const &ch = changeset.front();
  optional<Move> move = Statistics::Detect(last.back().image, board, ch, g.position, g.moves, color, g.hand(color), book);
  if (!move) {
    return;
  }
  for (Move const &m : moveCandidateHistory) {
    if (m != *move) {
      moveCandidateHistory.clear();
      moveCandidateHistory.push_back(*move);
      return;
    }
  }
  moveCandidateHistory.push_back(*move);
  if (moveCandidateHistory.size() < stableThresholdMoves) {
    // stable と判定するにはまだ足りない.
    return;
  }

  // move が確定した
  moveCandidateHistory.clear();
  boardHistory.clear();

  optional<Square> lastMoveTo;
  if (!g.moves.empty()) {
    lastMoveTo = g.moves.back().to;
  }
  cout << (char const *)StringFromMove(*move, lastMoveTo).c_str() << endl;
  // 初手なら画像の上下どちらが先手側か判定する.
  if (g.moves.empty() && move->to.rank < 5) {
    // キャプチャした画像で先手が上になっている. 以後 180 度回転して処理する.
    if (move->from) {
      move->from = move->from->rotated();
    }
    move->to = move->to.rotated();
    for (int i = 0; i < history.size(); i++) {
      cv::Mat rotated;
      cv::rotate(history[i].image, rotated, cv::ROTATE_180);
      history[i].image = rotated;
    }
    for (int i = 0; i < last.size(); i++) {
      cv::Mat rotated;
      cv::rotate(last[i].image, rotated, cv::ROTATE_180);
      last[i].image = rotated;
    }
    rotate = true;
  }
  if (g.moves.empty()) {
    book.update(g.position, last.back().image);
    cout << "b64png(book" << g.moves.size() << "):" << book.toPng() << endl;
  }
  stableBoardHistory.push_back(history);
  g.moves.push_back(*move);
  g.apply(*move);
  book.update(g.position, board);
  cout << "b64png(book" << g.moves.size() << "):" << book.toPng() << endl;
}

std::optional<Move> Statistics::Detect(cv::Mat const &boardBefore, cv::Mat const &boardAfter, CvPointSet const &changes, Position const &position, std::vector<Move> const &moves, Color const &color, std::deque<PieceType> const &hand, PieceBook &book) {
  using namespace std;
  optional<Move> move;
  auto [before, after] = Img::Equalize(boardBefore, boardAfter);
  if (changes.size() == 1) {
    cv::Point ch = *changes.begin();
    Piece p = position.pieces[ch.x][ch.y];
    if (p == 0) {
      // 変化したマスが空きなのでこの手は駒打ち.
      if (hand.empty()) {
        cout << "持ち駒が無いので駒打ちは検出できない" << endl;
      } else if (hand.size() == 1) {
        Move mv;
        mv.color = color;
        mv.to = MakeSquare(ch.x, ch.y);
        mv.piece = MakePiece(color, *hand.begin());
        move = mv;
      } else {
        double maxSim = 0;
        std::optional<Piece> maxSimPiece;
        cv::Mat roi = Img::PieceROI(boardAfter, ch.x, ch.y).clone();
        book.each(color, [&](Piece piece, cv::Mat const &pi) {
          if (IsPromotedPiece(piece)) {
            // 成り駒は打てない.
            return;
          }
          PieceType pt = PieceTypeFromPiece(piece);
          if (find(hand.begin(), hand.end(), pt) == hand.end()) {
            // 持ち駒に無い.
            return;
          }
          double sim = Img::Similarity(pi, roi);
          if (sim > maxSim) {
            maxSim = sim;
            maxSimPiece = piece;
          }
        });
        if (maxSimPiece) {
          Move mv;
          mv.color = color;
          mv.to = MakeSquare(ch.x, ch.y);
          mv.piece = *maxSimPiece;
          move = mv;
        } else {
          cout << "打った駒の検出に失敗" << endl;
        }
      }
    } else {
      // 相手の駒がいるマス全てについて, 直前のマス画像との類似度を調べる. 類似度が最も低かったマスを, 取られた駒の居たマスとする.
      double minSim = numeric_limits<double>::max();
      optional<Square> minSquare;
      for (int y = 0; y < 9; y++) {
        for (int x = 0; x < 9; x++) {
          auto piece = position.pieces[x][y];
          if (piece == 0 || ColorFromPiece(piece) == color) {
            continue;
          }
          auto bp = Img::PieceROI(before, x, y);
          auto ap = Img::PieceROI(after, x, y);
          double sim = Img::Similarity(bp, ap);
          if (minSim > sim) {
            minSim = sim;
            minSquare = MakeSquare(x, y);
          }
        }
      }
      if (minSquare) {
        Move mv;
        mv.color = color;
        mv.from = MakeSquare(ch.x, ch.y);
        mv.to = *minSquare;
        mv.newHand = PieceTypeFromPiece(position.pieces[minSquare->file][minSquare->rank]);
        mv.piece = p;
        if (!moves.empty()) {
          AppendPromotion(mv, before, after, book);
        }
        move = mv;
      } else {
        cout << "取った駒を検出できなかった" << endl;
      }
    }
  } else if (changes.size() == 2) {
    // from と to どちらも駒がある場合 => from が to の駒を取る
    // to が空きマス, from が手番の駒 => 駒の移動
    // それ以外 => エラー
    auto it = changes.begin();
    cv::Point ch0 = *it;
    it++;
    cv::Point ch1 = *it;
    Piece p0 = position.pieces[ch0.x][ch0.y];
    Piece p1 = position.pieces[ch1.x][ch1.y];
    if (p0 != 0 && p1 != 0) {
      if (ColorFromPiece(p0) == ColorFromPiece(p1)) {
        cout << "自分の駒を自分で取っている" << endl;
      } else {
        Move mv;
        mv.color = color;
        if (moves.empty() || ColorFromPiece(p0) == color) {
          // p0 の駒が p1 の駒を取った.
          mv.from = MakeSquare(ch0.x, ch0.y);
          mv.to = MakeSquare(ch1.x, ch1.y);
          mv.piece = p0;
          mv.newHand = PieceTypeFromPiece(p1);
          if (!moves.empty()) {
            AppendPromotion(mv, before, after, book);
          }
        } else {
          // p1 の駒が p0 の駒を取った.
          mv.from = MakeSquare(ch1.x, ch1.y);
          mv.to = MakeSquare(ch0.x, ch0.y);
          mv.piece = p1;
          mv.newHand = PieceTypeFromPiece(p0);
          if (!moves.empty()) {
            AppendPromotion(mv, before, after, book);
          }
        }
        move = mv;
      }
    } else if (p0) {
      if (moves.empty() || ColorFromPiece(p0) == color) {
        // p0 の駒が p1 に移動
        Move mv;
        mv.color = color;
        mv.from = MakeSquare(ch0.x, ch0.y);
        mv.to = MakeSquare(ch1.x, ch1.y);
        mv.piece = p0;
        if (!moves.empty()) {
          AppendPromotion(mv, before, after, book);
        }
        move = mv;
      } else {
        cout << "相手の駒を動かしている" << endl;
      }
    } else if (p1) {
      if (moves.empty() || ColorFromPiece(p1) == color) {
        // p1 の駒が p0 に移動
        Move mv;
        mv.color = color;
        mv.from = MakeSquare(ch1.x, ch1.y);
        mv.to = MakeSquare(ch0.x, ch0.y);
        mv.piece = p1;
        if (!moves.empty()) {
          AppendPromotion(mv, before, after, book);
        }
        move = mv;
      } else {
        cout << "相手の駒を動かしている" << endl;
      }
    }
  }
  return move;
}

} // namespace sci