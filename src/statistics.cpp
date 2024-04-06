#include <shogi_camera/shogi_camera.hpp>

#include <opencv2/imgproc.hpp>

#include "base64.hpp"
#include <iostream>
#include <numbers>

using namespace std;

namespace sci {

namespace {

template <class T, class L>
bool IsIdentical(set<T, L> const &a, set<T, L> const &b) {
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

void AppendPromotion(Move &mv,
                     cv::Mat const &boardBefore, cv::Mat const &boardBeforeColor,
                     cv::Mat const &boardAfter, cv::Mat const &boardAfterColor,
                     PieceBook &book,
                     optional<Move> hint,
                     Status const &s,
                     hwm::task_queue &pool) {
  if (!mv.from || IsPromotedPiece(mv.piece)) {
    return;
  }
  if (!CanPromote(mv.piece)) {
    return;
  }
#if !SHOGI_CAMERA_DEBUG
  if (!IsPromotableMove(*mv.from, mv.to, mv.color)) {
    return;
  }
#endif
  if (MustPromote(PieceTypeFromPiece(mv.piece), *mv.from, mv.to, mv.color)) {
    mv.piece = Promote(mv.piece);
    mv.promote = 1;
    return;
  }

#if SHOGI_CAMERA_DISABLE_HINT
  hint = nullopt;
#endif

  // 成る前の駒用の PieceBook::Entry を使って, 移動前後でそれぞれ駒画像との類似度を調べる.
  float meanUnpromoteBefore;
  float stddevUnpromoteBefore;
  float meanUnpromoteAfter;
  float stddevUnpromoteAfter;
  cv::Mat maxImgUnpromoteAfter;
  cv::Mat maxImgUnpromoteBefore;
  Img::ComparePieceCache cacheB;
  Img::ComparePieceCache cacheA;
  {
    auto const &entry = book.store[RemoveColorFromPiece(mv.piece)];
    vector<float> simUnpromoteAfter;
    vector<float> simUnpromoteBefore;
    double maxSimUnpromoteAfter = 0;
    double maxSimUnpromoteBefore = 0;
    entry.each(mv.color, [&](cv::Mat const &img, optional<PieceShape> shape) {
      auto [sb, imgB] = Img::ComparePiece(boardBefore, mv.from->file, mv.from->rank, img, mv.color, shape, pool, cacheB);
      auto [sa, imgA] = Img::ComparePiece(boardAfter, mv.to.file, mv.to.rank, img, mv.color, shape, pool, cacheA);
      simUnpromoteBefore.push_back(sb);
      simUnpromoteAfter.push_back(sa);
      if (sb > maxSimUnpromoteBefore) {
        maxImgUnpromoteBefore = imgB;
      }
      if (sa > maxSimUnpromoteAfter) {
        maxImgUnpromoteAfter = imgA;
      }
    });
    cv::Scalar meanBeforeScalar, stddevBeforeScalar;
    cv::meanStdDev(cv::Mat(simUnpromoteBefore), meanBeforeScalar, stddevBeforeScalar);
    cv::Scalar meanAfterScalar, stddevAfterScalar;
    cv::meanStdDev(cv::Mat(simUnpromoteAfter), meanAfterScalar, stddevAfterScalar);

    meanUnpromoteBefore = meanBeforeScalar[0];
    stddevUnpromoteBefore = stddevBeforeScalar[0];
    meanUnpromoteAfter = meanAfterScalar[0];
    stddevUnpromoteAfter = stddevAfterScalar[0];
  }

  // 成り駒用の PieceBook::Entry を使って, 移動後の駒画像との類似度を調べる.
  optional<float> meanPromoteAfter;
  optional<float> stddevPromoteAfter;
  if (auto entry = book.store.find(static_cast<PieceUnderlyingType>(Promote(RemoveColorFromPiece(mv.piece)))); entry != book.store.end() && !entry->second.images.empty()) {
    vector<float> simPromoteAfter;
    double maxSimPromoteAfter = 0;
    entry->second.each(mv.color, [&](cv::Mat const &img, optional<PieceShape> shape) {
      auto [sa, imgA] = Img::ComparePiece(boardAfter, mv.to.file, mv.to.rank, img, mv.color, shape, pool, cacheA);
      simPromoteAfter.push_back(sa);
    });
    cv::Scalar meanAfterScalar, stddevAfterScalar;
    cv::meanStdDev(cv::Mat(simPromoteAfter), meanAfterScalar, stddevAfterScalar);
    meanPromoteAfter = meanAfterScalar[0];
    stddevPromoteAfter = stddevAfterScalar[0];
  }

  // before の平均値の, after から見たときの偏差値
  // before と after で, book の駒画像との類似度の分布を調べる. before 対 book の分布が, after 対 book の分布とだいたい同じなら不成, 大きくずれていれば成りと判定する.
  float tScoreBeforeUnpromote = 10 * (meanUnpromoteBefore - meanUnpromoteAfter) / stddevUnpromoteAfter + 50;
  float tScoreAfterUnpromote = 10 * (meanUnpromoteAfter - meanUnpromoteBefore) / stddevUnpromoteBefore + 50;

  int promote = mv.promote;

  constexpr float kTScoreThresholdBetweenUnpromote = 40;
  // 実例:
  // 指し手 | tScoreBeforeUnpromote | tScoreAfterUnpromote | diff
  // -------------------------------------------------------------
  // 銀不成 | 63.8766               | 30.8876              | 32.989
  // 角成   | 177.716               | 50.5639              | 228.28
  // 歩成   | 137.282               | -14.2832             | 151.565

  if (meanPromoteAfter && stddevPromoteAfter) {
    // 成り駒画像群と比較した時の類似度の平均値が, 不成駒画像群と比較したときの類似度の集合に居たと仮定した時の偏差値.
    float tScoreAfterPromote = 10 * (*meanPromoteAfter - meanUnpromoteAfter) / stddevUnpromoteAfter + 50;
    // mv が成りならばこの値は tScoreAfterUnpromote より大きく, 不成なら tScoreAfterUnpromote より小さくなるはず.

    cout << "tScoreBeforeUnpromote=" << tScoreBeforeUnpromote << ", tScoreAfterUnpromote=" << tScoreAfterUnpromote << ", (tScoreBeforeUnpromote - tScoreAfterUnpromote)=" << (tScoreBeforeUnpromote - tScoreAfterUnpromote) << endl;
    cout << "tScoreAfterPromote=" << tScoreAfterPromote << ", tScoreAfterUnpromote=" << tScoreAfterUnpromote << ", (tScoreAfterPromote - tScoreAfterUnpromote)=" << (tScoreAfterPromote - tScoreAfterUnpromote) << endl;

    if (tScoreBeforeUnpromote - tScoreAfterUnpromote > kTScoreThresholdBetweenUnpromote || tScoreAfterPromote - tScoreAfterUnpromote > 20) {
      if (!hint || hint->promote == 1) {
        promote = 1;
      } else if (hint && promote != hint->promote) {
        cout << "hint と promote フラグが矛盾する: expected=" << hint->promote << "; actual=1" << endl;
        promote = hint->promote;
      }
    } else {
      if (!hint || hint->promote == -1) {
        promote = -1;
      } else if (hint && promote != hint->promote) {
        cout << "hint と promote フラグが矛盾する: expected=" << hint->promote << "; actual=-1" << endl;
        promote = hint->promote;
      }
    }
  } else {
    cout << "tScoreBeforeUnpromote=" << tScoreBeforeUnpromote << ", tScoreAfterUnpromote=" << tScoreAfterUnpromote << ", (tScoreBeforeUnpromote - tScoreAfterUnpromote)=" << (tScoreBeforeUnpromote - tScoreAfterUnpromote) << endl;

    if (tScoreBeforeUnpromote - tScoreAfterUnpromote > kTScoreThresholdBetweenUnpromote) {
      if (!hint || hint->promote == 1) {
        promote = 1;
      } else if (hint && promote != hint->promote) {
        cout << "hint と promote フラグが矛盾する: expected=" << hint->promote << "; actual=1" << endl;
        promote = hint->promote;
      }
    } else {
      if (!hint || hint->promote == -1) {
        promote = -1;
      } else if (hint && promote != hint->promote) {
        cout << "hint と promote フラグが矛盾する: expected=" << hint->promote << "; actual=-1" << endl;
        promote = hint->promote;
      }
    }
  }
#if 1
  static int count = 0;
  count++;
  auto bp = Img::PieceROI(boardBefore, mv.from->file, mv.from->rank);
  auto ap = Img::PieceROI(boardAfter, mv.to.file, mv.to.rank);
  cout << "b64png(promote_" << count << "_before_promote=" << promote << "):" << base64::to_base64(Img::EncodeToPng(bp)) << endl;
  cout << "b64png(promote_" << count << "_after_promote=" << promote << "):" << base64::to_base64(Img::EncodeToPng(ap)) << endl;
  cout << "b64png(promote_" << count << "_book_promote=" << promote << "):" << base64::to_base64(book.toPng()) << endl;
  cout << "b64png(promote_" << count << "_maxbefore_promote=" << promote << "):" << base64::to_base64(Img::EncodeToPng(maxImgUnpromoteBefore)) << endl;
  cout << "b64png(promote_" << count << "_maxafter_promote=" << promote << "):" << base64::to_base64(Img::EncodeToPng(maxImgUnpromoteAfter)) << endl;
#endif

#if SHOGI_CAMERA_DEBUG
  if (!IsPromotableMove(*mv.from, mv.to, mv.color)) {
    return;
  }
#endif

  if (promote == 1) {
    mv.piece = Promote(mv.piece);
    mv.promote = 1;
  } else if (promote == -1) {
    mv.promote = -1;
  }
}

void BlurFile5(BoardImage::Pack &pack, Position const &p) {
  // 折盤の場合五筋の盤の割れ目の線が顕著だと, Img::Compare で駒の有無の判定が偽陽性となる事がある.
  // 五筋の空きマスの中央付近に blur を掛けて割れ目の線を弱める.
  int const x = File::File5;
  for (BoardImage &bi : pack) {
    cv::Mat img = bi.gray_.clone();
    for (int y = 0; y < 9; y++) {
      if (p.pieces[x][y] != 0) {
        continue;
      }
      auto rect = Img::PieceROIRect(img.size(), x, y);
      int cx = rect.x + rect.width / 2;
      int cy = rect.y + rect.height / 2;
      int w = rect.width * 9 / 10;
      int h = rect.height * 9 / 10;
      cv::Rect r(cx - w / 2, cy - h / 2, w, h);
      auto roi = img(r);
      cv::Mat blurred;
      cv::blur(roi, blurred, cv::Size(3, 3));
      blurred.copyTo(img(r));
    }
    bi.blurGray = img;
  }
}

} // namespace

Statistics::Statistics() : book(make_shared<PieceBook>()), pool(make_unique<hwm::task_queue>(3)) {
  pool->set_wait_before_destructed(true);
}

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
}

optional<Status::Result> Statistics::push(cv::Mat const &board,
                                          cv::Mat const &fullcolor,
                                          Status &s,
                                          Game &g,
                                          vector<Move> &detected,
                                          bool detectMove) {
  if (board.size().area() <= 0) {
    return nullopt;
  }
  BoardImage bi;
  bi.gray_ = board;
  bi.fullcolor = fullcolor;
  bi.blurGray = board.clone();
  boardHistory.push_back(bi);
  if (boardHistory.size() == 1) {
    return nullopt;
  }
  int constexpr stableThresholdFrames = 3;
  int constexpr stableThresholdMoves = 3;
  // 盤面の各マスについて, 直前の画像との類似度を計算する. 将棋は 1 手につきたかだか 2 マス変動するはず. もし変動したマスが 3 マス以上なら,
  // 指が映り込むなどして盤面が正確に検出できなかった可能性がある.
  // 直前から変動した升目数が 0 のフレームが stableThresholdFrames フレーム連続した時, stable になったと判定する.
  BoardImage const &before = boardHistory[boardHistory.size() - 2];
  BoardImage const &after = boardHistory[boardHistory.size() - 1];
  CvPointSet changes;
  Img::Compare(before, after, changes, s.similarity);
  if (!stableBoardHistory.empty()) {
    auto const &last = stableBoardHistory.back();
    CvPointSet tmp;
    Img::Compare(last[2], bi, tmp, s.similarityAgainstStableBoard);
  }
  if (!changes.empty()) {
    // 変動したマス目が検出されているので, 最新のフレームだけ残して捨てる.
    boardHistory.clear();
    boardHistory.push_back(bi);
    moveCandidateHistory.clear();
    return nullopt;
  }
  // 直前の stable board がある場合, stable board と
  if (boardHistory.size() < stableThresholdFrames) {
    // まだ stable じゃない.
    return nullopt;
  }
  // stable になったと判定する. 直近 3 フレームを stableBoardHistory の候補とする.
  BoardImage::Pack history;
  for (int i = 0; i < history.size(); i++) {
    history[i] = boardHistory.back();
    boardHistory.pop_back();
  }
  if (stableBoardHistory.empty()) {
    // 最初の stable board なので登録するだけ.
    BlurFile5(history, g.position);
    stableBoardHistory.push_back(history);
    boardHistory.clear();
    return nullopt;
  }
  // 直前の stable board の各フレームと比較して, 変動したマス目が有るかどうか判定する.
  BoardImage::Pack &last = stableBoardHistory.back();
  int minChange = 81;
  int maxChange = -1;
  vector<CvPointSet> changeset;
  for (int i = 0; i < last.size(); i++) {
    for (int j = 0; j < history.size(); j++) {
      auto const &before = last[i];
      auto const &after = history[j];
      changes.clear();
      Img::Compare(before, after, changes);
      minChange = min(minChange, (int)changes.size());
      maxChange = max(maxChange, (int)changes.size());
      changeset.push_back(changes);
    }
  }
  if (changeset.empty() || minChange != maxChange || minChange > 2) {
    // 有効な変化が発見できなかった
    if ((minChange > 2 || maxChange > 2) && stableBoardHistory.size() == 1 && detected.empty() && !s.started) {
      // まだ stable board が 1 個だけの場合, その stable board が間違った範囲を検出しているせいでずっとここを通過し続けてしまう可能性がある.
      stableBoardInitialResetCounter++;
      stableBoardInitialReadyCounter = 0;
      s.boardReady = false;
      if (stableBoardInitialResetCounter > kStableBoardCounterThreshold) {
        stableBoardHistory.pop_back();
        BlurFile5(history, g.position);
        stableBoardHistory.push_back(history);
        cout << "stableBoardHistoryをリセット" << endl;
      }
    }
    return nullopt;
  }
  // changeset 内の変化位置が全て同じ部分を指しているか確認する. 違っていれば stable とはみなせない.
  for (int i = 1; i < changeset.size(); i++) {
    if (!IsIdentical(changeset[0], changeset[i])) {
      return nullopt;
    }
  }

  stableBoardInitialResetCounter = 0;
  stableBoardInitialReadyCounter = std::min(stableBoardInitialReadyCounter + 1, kStableBoardCounterThreshold + 1);
  s.boardReady = stableBoardInitialReadyCounter > kStableBoardCounterThreshold;

  if (!detectMove) {
    return nullopt;
  }
  Color const color = ColorFromIndex(detected.size(), g.first);
  CvPointSet const &ch = changeset.front();
  optional<Move> hint;
  if (detected.size() + 1 == g.moves.size()) {
    hint = g.moves.back();
  }
  optional<Move> move = Detect(last.back().gray_, last.back().fullcolor,
                               board, fullcolor,
                               ch,
                               g.position,
                               detected,
                               color,
                               g.hand(color),
                               *book,
                               hint,
                               s,
                               *pool);
  if (!move) {
    return nullopt;
  }
  moveCandidateHistory.push_back(*move);
  for (Move const &m : moveCandidateHistory) {
    if (m != *move) {
      moveCandidateHistory.clear();
      moveCandidateHistory.push_back(*move);
      return nullopt;
    }
  }
  if (moveCandidateHistory.size() < stableThresholdMoves) {
    // stable と判定するにはまだ足りない.
    return nullopt;
  }

  // move が確定した
  moveCandidateHistory.clear();
  boardHistory.clear();

  optional<Square> lastMoveTo;
  if (detected.empty()) {
    move->color = g.first;

    // 初手なら画像の上下どちらが先手側か判定する.
    if ((move->to.rank < Rank::Rank5 && g.first == Color::Black) || (move->to.rank > Rank::Rank5 && g.first == Color::White)) {
      // キャプチャした画像で先手が上になっている. 以後 180 度回転して処理する.
      if (move->from) {
        move->from = move->from->rotated();
      }
      move->to = move->to.rotated();
      move->piece = RemoveColorFromPiece(move->piece) | static_cast<PieceUnderlyingType>(color);
      for (int i = 0; i < history.size(); i++) {
        history[i].rotate();
      }
      for (int i = 0; i < last.size(); i++) {
        last[i].rotate();
      }
      rotate = true;
    }

    book->update(g.position, board, s);
  } else {
    lastMoveTo = detected.back().to;
  }
  move->decideSuffix(g.position);
  bool aiHand = detected.size() + 1 == g.moves.size();
  if (aiHand) {
    // g.moves.back() は AI が生成した手なので, それと合致しているか調べる.
    if (*move != g.moves.back()) {
      s.wrongMove = true;
      cout << "AIの示した手と違う手が指されている" << endl;
      return nullopt;
    }
  }
  s.wrongMove = false;
  detected.push_back(*move);
  optional<Status::Result> ret;
  switch (g.apply(*move)) {
  case Game::ApplyResult::Ok:
    if (!aiHand) {
      g.moves.push_back(*move);
    }
    break;
  case Game::ApplyResult::Illegal:
    if (!s.result) {
      Status::Result r;
      if (move->color == Color::Black) {
        r.result = GameResult::WhiteWin;
      } else {
        r.result = GameResult::BlackWin;
      }
      r.reason = GameResultReason::IllegalAction;
      ret = r;
    }
    break;
  case Game::ApplyResult::Repetition:
    if (!s.result) {
      Status::Result r;
      r.result = GameResult::Abort;
      r.reason = GameResultReason::Repetition;
      ret = r;
    }
    break;
  case Game::ApplyResult::CheckRepetitionBlack:
    if (!s.result) {
      Status::Result r;
      r.result = GameResult::WhiteWin;
      r.reason = GameResultReason::CheckRepetition;
      ret = r;
    }
    break;
  case Game::ApplyResult::CheckRepetitionWhite:
    if (!s.result) {
      Status::Result r;
      r.result = GameResult::BlackWin;
      r.reason = GameResultReason::CheckRepetition;
      ret = r;
    }
    break;
  }
  BlurFile5(history, g.position);
  stableBoardHistory.push_back(history);
  book->update(g.position, board, s);
  cout << g.moves.size() << ":" << (char const *)StringFromMoveWithOptionalLast(*move, lastMoveTo).c_str() << endl;
  return ret;
}

optional<Move> Statistics::Detect(cv::Mat const &boardBefore, cv::Mat const &boardBeforeColor,
                                  cv::Mat const &boardAfter, cv::Mat const &boardAfterColor,
                                  CvPointSet const &changes,
                                  Position const &position,
                                  vector<Move> const &moves,
                                  Color const &color,
                                  deque<PieceType> const &hand,
                                  PieceBook &book,
                                  optional<Move> hint,
                                  Status const &s,
                                  hwm::task_queue &pool) {
  optional<Move> move;
  auto [before, after] = Img::Equalize(boardBefore, boardAfter);
  auto [beforeColor, afterColor] = Img::Equalize(boardBeforeColor, boardAfterColor);

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
        cv::Mat roi = Img::PieceROI(boardAfter, ch.x, ch.y);
        optional<Piece> maxSimPiece;
        map<PieceType, pair<double, cv::Mat>> maxSimStat;
        Img::ComparePieceCache cache;
        book.each(color, [&](Piece piece, cv::Mat const &pi, optional<PieceShape> shape) {
          if (!CanDrop(position, piece, MakeSquare(ch.x, ch.y))) {
            return;
          }
          PieceType pt = PieceTypeFromPiece(piece);
          if (find(hand.begin(), hand.end(), pt) == hand.end()) {
            // 持ち駒に無い.
            return;
          }
          auto [sim, img] = Img::ComparePiece(boardAfter, ch.x, ch.y, pi, color, shape, pool, cache);
          auto found = maxSimStat.find(pt);
          if (found == maxSimStat.end()) {
            maxSimStat[pt] = make_pair(sim, img);
          } else if (found->second.first < sim) {
            maxSimStat[pt] = make_pair(sim, img);
          }
          if (sim > maxSim) {
            maxSim = sim;
            maxSimPiece = piece;
          }
        });
        static int cnt = 0;
        cnt++;
        cout << "--" << endl;
        for (auto const &it : maxSimStat) {
          cout << (char const *)ShortStringFromPieceTypeAndStatus(static_cast<PieceUnderlyingType>(it.first)).c_str() << ":" << it.second.first << endl;
        }
#if 1
        int i = 0;
        for (auto const &it : maxSimStat) {
          i++;
          cout << "b64png(drop_" << cnt << "_" << i << "_" << (char const *)ShortStringFromPieceTypeAndStatus(static_cast<PieceUnderlyingType>(it.first)).c_str() << "):" << base64::to_base64(Img::EncodeToPng(it.second.second)) << endl;
        }
#endif
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
      // まず駒の向きが自分と同じ向きになっているものだけを対象に調べる. 見つからなければ駒の向きは無視して調べる.
      for (bool directionAware : {true, false}) {
        for (int y = 0; y < 9; y++) {
          for (int x = 0; x < 9; x++) {
            auto piece = position.pieces[x][y];
            if (piece == 0 || ColorFromPiece(piece) == color) {
              continue;
            }
            if (!Move::CanMove(position, MakeSquare(ch.x, ch.y), MakeSquare(x, y))) {
              continue;
            }
            if (directionAware) {
              cv::Rect rect = Img::PieceROIRect(after.size(), x, y);
              cv::Point2f center(rect.x + rect.width * 0.5f, rect.y + rect.height * 0.5f);
              bool found = false;
              for (auto const &piece : s.pieces) {
                auto wPiece = PerspectiveTransform(piece, s.perspectiveTransform, s.rotate, after.size().width, after.size().height);
                if (!wPiece) {
                  continue;
                }
                if (cv::pointPolygonTest(wPiece->points, center, false) < 0) {
                  continue;
                }
                cv::Point2f dir = wPiece->direction;
                double angle = atan2(dir.y, dir.x) * 180 / numbers::pi;
                while (angle < 0) {
                  angle += 360;
                }
                if (color == Color::Black) {
                  if (180 < angle && angle < 360) {
                    found = true;
                    break;
                  }
                } else {
                  if (0 < angle && angle < 180) {
                    found = true;
                    break;
                  }
                }
              }
              if (!found) {
                continue;
              }
            }
            double sim = Img::Similarity(before, after, x, y);
            cout << (char const *)StringFromSquare(MakeSquare(x, y)).c_str() << ":" << sim << endl;
            if (minSim > sim) {
              minSim = sim;
              minSquare = MakeSquare(x, y);
            }
          }
        }
        if (minSquare) {
          break;
        }
      }
      if (minSquare) {
        Move mv;
        mv.color = color;
        mv.from = MakeSquare(ch.x, ch.y);
        mv.to = *minSquare;
        mv.captured = RemoveColorFromPiece(position.pieces[minSquare->file][minSquare->rank]);
        mv.piece = p;
        if (!moves.empty()) {
          AppendPromotion(mv, before, beforeColor, after, afterColor, book, hint, s, pool);
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
          Square from = MakeSquare(ch0.x, ch0.y);
          Square to = MakeSquare(ch1.x, ch1.y);
          if (moves.empty() || Move::CanMove(position, from, to)) {
            mv.from = from;
            mv.to = to;
            mv.piece = p0;
            mv.captured = RemoveColorFromPiece(p1);
            if (!moves.empty()) {
              AppendPromotion(mv, before, beforeColor, after, afterColor, book, hint, s, pool);
            }
            move = mv;
          } else {
            cout << "利きの無い駒を取ろうとしている" << endl;
          }
        } else {
          // p1 の駒が p0 の駒を取った.
          Square from = MakeSquare(ch1.x, ch1.y);
          Square to = MakeSquare(ch0.x, ch0.y);
          if (moves.empty() || Move::CanMove(position, from, to)) {
            mv.from = from;
            mv.to = to;
            mv.piece = p1;
            mv.captured = RemoveColorFromPiece(p0);
            if (!moves.empty()) {
              AppendPromotion(mv, before, beforeColor, after, afterColor, book, hint, s, pool);
            }
            move = mv;
          } else {
            cout << "利きの無い駒を取ろうとしている" << endl;
          }
        }
      }
    } else if (p0) {
      if (moves.empty() || ColorFromPiece(p0) == color) {
        // p0 の駒が p1 に移動
        Square from = MakeSquare(ch0.x, ch0.y);
        Square to = MakeSquare(ch1.x, ch1.y);
        if (moves.empty() || Move::CanMove(position, from, to)) {
          Move mv;
          mv.color = color;
          mv.from = from;
          mv.to = to;
          mv.piece = p0;
          if (!moves.empty()) {
            AppendPromotion(mv, before, beforeColor, after, afterColor, book, hint, s, pool);
          }
          move = mv;
        } else {
          cout << "利きの無い位置に移動しようとしている" << endl;
        }
      } else {
        cout << "相手の駒を動かしている" << endl;
      }
    } else if (p1) {
      if (moves.empty() || ColorFromPiece(p1) == color) {
        // p1 の駒が p0 に移動
        Square from = MakeSquare(ch1.x, ch1.y);
        Square to = MakeSquare(ch0.x, ch0.y);
        if (moves.empty() || Move::CanMove(position, from, to)) {
          Move mv;
          mv.color = color;
          mv.from = from;
          mv.to = to;
          mv.piece = p1;
          if (!moves.empty()) {
            AppendPromotion(mv, before, beforeColor, after, afterColor, book, hint, s, pool);
          }
          move = mv;
        } else {
          cout << "利きの無い位置に移動しようとしている" << endl;
        }
      } else {
        cout << "相手の駒を動かしている" << endl;
      }
    }
  }
  return move;
}

} // namespace sci
