#include <shogi_camera/shogi_camera.hpp>

#include <opencv2/imgproc.hpp>

#include "base64.hpp"
#include <iostream>
#include <numbers>

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

void AppendPromotion(Move &mv, cv::Mat const &boardBefore, cv::Mat const &boardAfter, PieceBook &book, std::optional<Move> hint, Status const &s, hwm::task_queue &pool) {
  using namespace std;
  if (!mv.from || IsPromotedPiece(mv.piece)) {
    return;
  }
  if (!CanPromote(mv.piece)) {
    return;
  }
#if !defined(SHOGI_CAMERA_DEBUG)
  if (!IsPromotableMove(*mv.from, mv.to, mv.color)) {
    return;
  }
#endif
  if (MustPromote(PieceTypeFromPiece(mv.piece), *mv.from, mv.to, mv.color)) {
    mv.piece = Promote(mv.piece);
    mv.promote = 1;
    return;
  }

#if defined(SHOGI_CAMERA_DISABLE_HINT)
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
    entry.each(mv.color, [&](cv::Mat const &img, std::optional<PieceShape> shape) {
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
    entry->second.each(mv.color, [&](cv::Mat const &img, std::optional<PieceShape> shape) {
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

  if (meanPromoteAfter && stddevPromoteAfter) {
    // 成り駒画像群と比較した時の類似度の平均値が, 不成駒画像群と比較したときの類似度の集合に居たと仮定した時の偏差値.
    float tScoreAfterPromote = 10 * (*meanPromoteAfter - meanUnpromoteAfter) / stddevUnpromoteAfter + 50;
    // mv が成りならばこの値は tScoreAfterUnpromote より大きく, 不成なら tScoreAfterUnpromote より小さくなるはず.

    cout << "tScoreBeforeUnpromote=" << tScoreBeforeUnpromote << ", tScoreAfterUnpromote=" << tScoreAfterUnpromote << ", (tScoreBeforeUnpromote - tScoreAfterUnpromote)=" << (tScoreBeforeUnpromote - tScoreAfterUnpromote) << endl;
    cout << "tScoreAfterPromote=" << tScoreAfterPromote << ", tScoreAfterUnpromote=" << tScoreAfterUnpromote << ", (tScoreAfterPromote - tScoreAfterUnpromote)=" << (tScoreAfterPromote - tScoreAfterUnpromote) << endl;

    if (tScoreBeforeUnpromote - tScoreAfterUnpromote > 20 || tScoreAfterPromote - tScoreAfterUnpromote > 20) {
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

    if (tScoreBeforeUnpromote - tScoreAfterUnpromote > 20) {
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

#if defined(SHOGI_CAMERA_DEBUG)
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

} // namespace

Statistics::Statistics() : book(std::make_shared<PieceBook>()), pool(std::make_unique<hwm::task_queue>(3)) {
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

void Statistics::push(cv::Mat const &board, Status &s, Game &g, std::vector<Move> &detected, bool detectMove) {
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
      minChange = min(minChange, (int)changes.size());
      maxChange = max(maxChange, (int)changes.size());
      changeset.push_back(changes);
    }
  }
  if (changeset.empty() || minChange != maxChange || minChange > 2) {
    // 有効な変化が発見できなかった
    if ((minChange > 2 || maxChange > 2) && stableBoardHistory.size() == 1 && detected.empty() && g.moves.empty()) {
      // まだ stable board が 1 個だけの場合, その stable board が間違った範囲を検出しているせいでずっとここを通過し続けてしまう可能性がある.
      stableBoardInitialResetCounter++;
      stableBoardInitialReadyCounter = 0;
      s.boardReady = false;
      if (stableBoardInitialResetCounter > kStableBoardCounterThreshold) {
        stableBoardHistory.pop_back();
        stableBoardHistory.push_back(history);
        cout << "stableBoardHistoryをリセット" << endl;
      }
    }
    return;
  }
  // changeset 内の変化位置が全て同じ部分を指しているか確認する. 違っていれば stable とはみなせない.
  for (int i = 1; i < changeset.size(); i++) {
    if (!IsIdentical(changeset[0], changeset[i])) {
      return;
    }
  }

  stableBoardInitialResetCounter = 0;
  stableBoardInitialReadyCounter = std::min(stableBoardInitialReadyCounter + 1, kStableBoardCounterThreshold + 1);
  s.boardReady = stableBoardInitialReadyCounter > kStableBoardCounterThreshold;

  if (!detectMove) {
    return;
  }
  // index 番目の手.
  size_t const index = detected.size();
  Color const color = ColorFromIndex(index);
  CvPointSet const &ch = changeset.front();
  optional<Move> hint;
  if (detected.size() + 1 == g.moves.size()) {
    hint = g.moves.back();
  }
  optional<Move> move = Detect(last.back().image,
                               board,
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
    return;
  }
  moveCandidateHistory.push_back(*move);
  for (Move const &m : moveCandidateHistory) {
    if (m != *move) {
      moveCandidateHistory.clear();
      moveCandidateHistory.push_back(*move);
      return;
    }
  }
  if (moveCandidateHistory.size() < stableThresholdMoves) {
    // stable と判定するにはまだ足りない.
    return;
  }

  // move が確定した
  moveCandidateHistory.clear();
  boardHistory.clear();

  optional<Square> lastMoveTo;
  if (!detected.empty()) {
    lastMoveTo = detected.back().to;
  }
  // 初手なら画像の上下どちらが先手側か判定する.
  if (detected.empty() && move->to.rank < 5) {
    // キャプチャした画像で先手が上になっている. 以後 180 度回転して処理する.
    if (move->from) {
      move->from = move->from->rotated();
    }
    move->to = move->to.rotated();
    move->piece = RemoveColorFromPiece(move->piece) | static_cast<PieceUnderlyingType>(color);
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
  if (detected.empty()) {
    book->update(g.position, last.back().image, s);
  }
  move->decideSuffix(g.position);
  if (detected.size() + 1 == g.moves.size()) {
#if !defined(SHOGI_CAMERA_DISABLE_HINT)
    // g.moves_.back() は AI が生成した手なので, それと合致しているか調べる.
    if (*move != g.moves.back()) {
      s.wrongMove = true;
      cout << "AIの示した手と違う手が指されている" << endl;
      return;
    }
#endif
  } else {
    g.moves.push_back(*move);
  }
  s.wrongMove = false;
  stableBoardHistory.push_back(history);
  detected.push_back(*move);
  g.apply(*move);
  book->update(g.position, board, s);
  cout << g.moves.size() << ":" << (char const *)StringFromMoveWithOptionalLast(*move, lastMoveTo).c_str() << endl;
}

std::optional<Move> Statistics::Detect(cv::Mat const &boardBefore,
                                       cv::Mat const &boardAfter,
                                       CvPointSet const &changes,
                                       Position const &position,
                                       std::vector<Move> const &moves,
                                       Color const &color,
                                       std::deque<PieceType> const &hand,
                                       PieceBook &book,
                                       std::optional<Move> hint,
                                       Status const &s,
                                       hwm::task_queue &pool) {
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
        cv::Mat roi = Img::PieceROI(boardAfter, ch.x, ch.y);
        map<PieceType, double> maxSimStat;
        Img::ComparePieceCache cache;
        book.each(color, [&](Piece piece, cv::Mat const &pi, optional<PieceShape> shape) {
          if (IsPromotedPiece(piece)) {
            // 成り駒は打てない.
            return;
          }
          PieceType pt = PieceTypeFromPiece(piece);
          if (find(hand.begin(), hand.end(), pt) == hand.end()) {
            // 持ち駒に無い.
            return;
          }
          auto [sim, _] = Img::ComparePiece(boardAfter, ch.x, ch.y, pi, color, shape, pool, cache);
          if (maxSimStat[pt] < sim) {
            maxSimStat[pt] = sim;
          }
          if (sim > maxSim) {
            maxSim = sim;
            maxSimPiece = piece;
          }
        });
        cout << "--" << endl;
        for (auto const &it : maxSimStat) {
          cout << (char const *)ShortStringFromPieceTypeAndStatus(static_cast<PieceUnderlyingType>(it.first)).c_str() << ":" << it.second << endl;
        }
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
          AppendPromotion(mv, before, after, book, hint, s, pool);
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
              AppendPromotion(mv, before, after, book, hint, s, pool);
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
              AppendPromotion(mv, before, after, book, hint, s, pool);
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
            AppendPromotion(mv, before, after, book, hint, s, pool);
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
            AppendPromotion(mv, before, after, book, hint, s, pool);
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
