#include <shogi_camera/shogi_camera.hpp>

#include <opencv2/imgproc.hpp>

#include <iostream>
#include <numbers>

namespace sci {

namespace {

float Angle(cv::Point2f const &a) {
  return atan2f(a.y, a.x);
}

cv::Point2d Rotate(cv::Point2d const &p, double radian) {
  return cv::Point2d(cos(radian) * p.x - sin(radian) * p.y, sin(radian) * p.x + cos(radian) * p.y);
}

float Normalize90To90(float a) {
  using namespace std;
  while (a < -numbers::pi * 0.5) {
    a += numbers::pi;
  }
  while (a > numbers::pi * 0.5) {
    a -= numbers::pi;
  }
  return a;
}

struct RadianAverage {
  void push(double radian) {
    x += cos(radian);
    y += sin(radian);
    count++;
  }

  double get() const {
    return atan2(y, x);
  }

  double x = 0;
  double y = 0;
  size_t count = 0;
};

float MeanAngle(std::initializer_list<float> values) {
  RadianAverage ra;
  for (float v : values) {
    ra.push(v);
  }
  return ra.get();
}

std::optional<float> SquareDirection(std::vector<cv::Point2f> const &points) {
  using namespace std;
  if (points.size() != 4) {
    return nullopt;
  }
  // 最も短い辺のインデックス.
  int index = -1;
  float shortest = numeric_limits<float>::max();
  for (int i = 0; i < (int)points.size(); i++) {
    float length = cv::norm(points[i] - points[(i + 1) % 4]);
    if (length < shortest) {
      shortest = length;
      index = i;
    }
  }
  // 長辺の傾き
  float angle0 = Normalize90To90(Angle(points[(index + 1) % 4] - points[(index + 2) % 4]));
  float angle1 = Normalize90To90(Angle(points[(index + 3) % 4] - points[index % 4]));
  return MeanAngle({angle0, angle1});
}

std::optional<cv::Point2d> Intersection(cv::Point2d const &p1, cv::Point2d const &p2, cv::Point2d const &q1, cv::Point2d const &q2) {
  using namespace std;
  cv::Point2d v = p2 - p1;
  cv::Point2d w = q2 - q1;

  double cross = w.x * v.y - w.y * v.x;
  if (!isnormal(cross)) {
    // 平行
    return nullopt;
  }

  if (w.x != 0) {
    double s = (q1.y * w.x + p1.x * w.y - q1.x * w.y - w.x * p1.y) / cross;
    if (s == 0 || isnormal(s)) {
      return p1 + v * s;
    } else {
      return nullopt;
    }
  } else {
    if (v.x != 0) {
      double s = (q1.x - p1.x) / v.x;
      if (s == 0 || isnormal(s)) {
        return p1 + v * s;
      } else {
        return nullopt;
      }
    } else {
      // v と w がどちらも y 軸に平行
      return nullopt;
    }
  }
}

std::optional<cv::Point2d> Intersection(cv::Vec4f const &a, cv::Vec4f const &b) {
  cv::Point2d p1(a[2], a[3]);
  cv::Point2d p2(a[2] + a[0], a[3] + a[1]);
  cv::Point2d q1(b[2], b[3]);
  cv::Point2d q2(b[2] + b[0], b[3] + b[1]);
  return Intersection(p1, p2, q1, q2);
}

// 直線 line と点 p との距離を計算する. line は [vx, vy, x0, y0] 形式(cv::fitLine の結果の型と同じ形式)
double Distance(cv::Vec4f const &line, cv::Point2d const &p) {
  // line と直行する直線
  cv::Vec4f c(line[1], line[0], p.x, p.y);
  if (auto i = Intersection(line, c); i) {
    return cv::norm(*i - p);
  } else {
    return std::numeric_limits<double>::infinity();
  }
}

std::optional<cv::Vec4f> FitLine(std::vector<cv::Point2f> const &points) {
  if (points.size() < 2) {
    return std::nullopt;
  }
  cv::Vec4f line;
  cv::fitLine(cv::Mat(points), line, cv::DIST_L2, 0, 0.01, 0.01);
  return line;
}

void FindBoard(cv::Mat const &frame, Status &s) {
  using namespace std;
  vector<shared_ptr<Contour>> squares;
  for (auto const &square : s.squares) {
    if (square->points.size() != 4) {
      continue;
    }
    squares.push_back(square);
  }
  if (squares.empty()) {
    return;
  }
  // 四角形の面積の中央値を升目の面積(squareArea)とする.
  sort(squares.begin(), squares.end(), [](shared_ptr<Contour> const &a, shared_ptr<Contour> const &b) { return a->area < b->area; });
  size_t mid = squares.size() / 2;
  if (squares.size() % 2 == 0) {
    auto const &a = squares[mid - 1];
    auto const &b = squares[mid];
    s.squareArea = (a->area + b->area) * 0.5;
    s.aspectRatio = (a->aspectRatio() + b->aspectRatio()) * 0.5;
  } else {
    s.squareArea = squares[mid]->area;
    s.aspectRatio = squares[mid]->aspectRatio();
  }

  {
    // squares の各辺の傾きから, 盤面の向きを推定する.
    vector<double> angles;
    for (auto const &square : s.squares) {
      for (size_t i = 0; i < 3; i++) {
        auto const &a = square->points[i];
        auto const &b = square->points[i + 1];
        double dx = b.x - a.x;
        double dy = b.y - a.y;
        double angle = atan2(dy, dx);
        while (angle < 0) {
          angle += numbers::pi * 2;
        }
        while (numbers::pi * 0.5 < angle) {
          angle -= numbers::pi * 0.5;
        }
        angles.push_back(angle * 180 / numbers::pi);
      }
    }
    for (auto const &piece : s.pieces) {
      float angle = Angle(piece->direction);
      while (angle < 0) {
        angle += numbers::pi * 2;
      }
      while (numbers::pi * 0.5 < angle) {
        angle -= numbers::pi * 0.5;
      }
      angles.push_back(angle * 180 / numbers::pi);
    }
    // 盤面の向きを推定する.
    // 5 度単位でヒストグラムを作り, 最頻値を調べる. angle は [0, 90) に限定しているので index は [0, 17]
    array<int, 18> count;
    count.fill(0);
    for (double const &angle : angles) {
      int index = angle / 5;
      count[index] += 1;
    }
    int maxIndex = -1;
    int maxCount = -1;
    for (int i = 0; i < (int)count.size(); i++) {
      if (maxCount < count[i]) {
        maxCount = count[i];
        maxIndex = i;
      }
    }
    // 最頻となったヒストグラムの位置から, 前後 5 度に収まっている angle について, その平均値を計算する.
    double targetAngle = (maxIndex + 0.5) * 5;
    RadianAverage ra;
    for (double const &angle : angles) {
      if (targetAngle - 5 <= angle && angle <= targetAngle + 5) {
        double radian = angle / 180.0 * numbers::pi;
        ra.push(radian);
      }
    }
    float direction = Normalize90To90(ra.get());
    s.boardDirection = direction;

    // Session.boardDirection は対局者の向きにしたい.
    map<bool, int> vote;
    // square の長手方向から, direction を 90 度回して訂正するか, そのまま採用するか投票する.
    for (auto const &square : s.squares) {
      if (auto d = SquareDirection(square->points); d) {
        bool shouldCorrect = fabs(cos(*d - direction)) < 0.25;
        vote[shouldCorrect] += 1;
      }
    }
    // piece の頂点の向きから, direction を 90 度回して訂正するか, そのまま採用するか投票する.
    for (auto const &piece : s.pieces) {
      float a = Normalize90To90(Angle(piece->direction));
      bool shouldCorrect = fabs(cos(a - direction)) < 0.25;
      vote[shouldCorrect] += 1;
    }
    if (vote[true] > (vote[true] + vote[false]) * 2 / 3) {
      direction = Normalize90To90(direction + numbers::pi * 0.5);
      s.boardDirection = direction;
    }
  }

  if (false) {
    // TODO: ここの処理は斜めから撮影した時に s.outline が盤面を正確に覆わなくなる問題の対策のための処理. 多少斜め方向からの撮影なら, この処理なくても問題なく盤面を認識できているので無くても良い. 本当は斜め角度大のときも正確に検出できるようにしたい.
    //  各 square, piece を起点に, その長軸と短軸それぞれについて, 軸付近に中心を持つ駒・升を検出する.
    //  どの軸にも属さない square, piece を除去する.
    set<shared_ptr<Contour>> squares;     // 抽出した squares. 後で s.squares と swap する.
    set<shared_ptr<PieceContour>> pieces; // 抽出した pieces. 後で s.pieces と swap する.
    double cosThreshold = cos(2.5 / 180.0 * numbers::pi);
    auto Detect = [&](double direction, double tolerance, vector<cv::Vec4f> &grids) {
      set<shared_ptr<Contour>> cSquares;
      set<shared_ptr<PieceContour>> cPieces;
      // tolerance: 軸との距離の最大値. この距離以下なら軸上に居るとみなす.
      for (auto const &square : s.squares) {
        if (square->area <= s.squareArea * 0.7 || s.squareArea * 1.3 <= square->area) {
          // 升目の面積と比べて 3 割以上差がある場合対象外にする.
          continue;
        }
        if (cSquares.find(square) != cSquares.end()) {
          // 処理済み.
          continue;
        }
        cv::Point2f center = square->mean();
        // square の各辺の方向のうち, direction と近い方向をその square の向きとする.
        RadianAverage ra;
        for (int i = 0; i < 4; i++) {
          cv::Point2f edge = square->points[i] - square->points[(i + 1) % 4];
          double dir = Angle(edge);
          if (fabs(cos(dir - direction)) > cosThreshold) {
            ra.push(dir);
          }
        }
        if (ra.count != 2) {
          continue;
        }
        double squareDirection = ra.get();
        cv::Vec4f axis(cos(squareDirection), sin(squareDirection), center.x, center.y);
        vector<cv::Point2f> centers;
        for (auto const &sq : s.squares) {
          cv::Point2f c = sq->mean();
          if (sq == square) {
            // 自分自身.
            centers.push_back(c);
            continue;
          }
          if (cSquares.find(sq) != cSquares.end()) {
            // 処理済み.
            continue;
          }
          if (sq->area <= s.squareArea * 0.7 || s.squareArea * 1.3 <= sq->area) {
            // 升目の面積と比べて 3 割以上差がある場合対象外にする.
            continue;
          }
          double distance = Distance(axis, c);
          if (!isfinite(distance)) {
            continue;
          }
          if (distance <= tolerance) {
            cSquares.insert(sq);
            centers.push_back(c);
          }
        }
        for (auto const &p : s.pieces) {
          if (p->area > s.squareArea) {
            // 升目の面積より大きい piece は対象外にする.
            continue;
          }
          if (cPieces.find(p) != cPieces.end()) {
            // 処理済み
            continue;
          }
          cv::Point2f c = p->mean();
          double distance = Distance(axis, c);
          if (!isfinite(distance)) {
            continue;
          }
          if (distance <= tolerance) {
            cPieces.insert(p);
            centers.push_back(c);
          }
        }
        if (centers.empty()) {
          continue;
        }
        auto grid = FitLine(centers);
        if (!grid) {
          continue;
        }
        if (fabs(atan2((*grid)[1], (*grid)[0]) - direction) <= cosThreshold) {
          // grid の向きと direction が大きく違っていたら, 検出した grid は無視する.
          continue;
        }
        for (auto const &cS : cSquares) {
          squares.insert(cS);
        }
        for (auto const &cP : cPieces) {
          pieces.insert(cP);
        }
        grids.push_back(*grid);
      }
    };
    Detect(s.boardDirection, sqrt(s.squareArea * s.aspectRatio) / 2, s.vgrids);
    Detect(s.boardDirection + numbers::pi * 0.5, sqrt(s.squareArea / s.aspectRatio) / 2, s.hgrids);
    s.squares.clear();
    for (auto const &square : squares) {
      s.squares.push_back(square);
    }
    s.pieces.clear();
    for (auto const &piece : pieces) {
      s.pieces.push_back(piece);
    }
  }

  {
    // squares と pieces を -1 * boardDirection 回転した状態で矩形を検出する.
    vector<cv::Point2d> centers;
    for (auto const &square : s.squares) {
      auto center = Rotate(square->mean(), -s.boardDirection);
      centers.push_back(center);
    }
    for (auto const &piece : s.pieces) {
      auto center = Rotate(piece->mean(), -s.boardDirection);
      centers.push_back(center);
    }
    if (!centers.empty()) {
      cv::Point2f top = centers[0];
      cv::Point2f bottom = top;
      cv::Point2f left = top;
      cv::Point2f right = top;
      for (auto const &center : centers) {
        if (center.y < top.y) {
          top = center;
        }
        if (center.y > bottom.y) {
          bottom = center;
        }
        if (center.x < left.x) {
          left = center;
        }
        if (center.x > right.x) {
          right = center;
        }
      }
      cv::Point2f lt(left.x, top.y);
      cv::Point2f rt(right.x, top.y);
      cv::Point2f rb(right.x, bottom.y);
      cv::Point2f lb(left.x, bottom.y);
      s.outline.points = {
          Rotate(lt, s.boardDirection),
          Rotate(rt, s.boardDirection),
          Rotate(rb, s.boardDirection),
          Rotate(lb, s.boardDirection),
      };
      s.outline.area = fabs(cv::contourArea(cv::Mat(s.outline.points)));
      s.bx = left.x;
      s.by = top.y;
      s.bwidth = right.x - left.x;
      s.bheight = bottom.y - top.y;
    }
  }
}

// [x, y] の升目の中心位置を返す.
// [0, 0] が 9一, [8, 8] が 1九. 戻り値の座標は入力画像での座標系.
cv::Point2f PieceCenter(Status const &s, int x, int y) {
  float fx = s.bx + x * s.bwidth / 8;
  float fy = s.by + y * s.bheight / 8;
  return Rotate(cv::Point2f(fx, fy), s.boardDirection);
}

void FindPieces(cv::Mat const &frame, Status &s) {
  using namespace std;
  {
    set<pair<bool, int>> attached; // first => s.square なら true, s.pieces なら false, second => s.squares または s.pieces のインデックス.
    float const squareSize = sqrtf(s.squareArea);
    for (int y = 0; y < 9; y++) {
      for (int x = 0; x < 9; x++) {
        cv::Point2f center = PieceCenter(s, x, y);
        // center に最も中心が近い Contour を s.pieces か s.squares の中から探す.
        // 距離は駒サイズのスケール sqrt(squareArea) / 2 より離れていると対象外にする.
        optional<pair<bool, int>> index;
        float nearest = numeric_limits<float>::max();
        for (int i = 0; i < s.pieces.size(); i++) {
          pair<bool, int> key = make_pair(false, i);
          if (attached.find(key) != attached.end()) {
            continue;
          }
          cv::Point2f m = s.pieces[i]->mean();
          float distance = cv::norm(m - center);
          if (distance > squareSize * 0.5) {
            continue;
          }
          if (distance < nearest) {
            nearest = distance;
            index = key;
          }
        }
        if (!index) {
          // 駒の方を優先で探す. 既に見つかっているならそちらを優先.
          for (int i = 0; i < s.squares.size(); i++) {
            pair<bool, int> key = make_pair(true, i);
            if (attached.find(key) != attached.end()) {
              continue;
            }
            cv::Point2f m = s.squares[i]->mean();
            float distance = cv::norm(m - center);
            if (distance > squareSize * 0.5) {
              continue;
            }
            if (distance < nearest) {
              nearest = distance;
              index = key;
            }
          }
        }
        if (!index) {
          continue;
        }
        attached.insert(*index);
        if (index->first) {
          s.detected[x][y] = s.squares[index->second];
        } else {
          s.detected[x][y] = make_shared<Contour>(s.pieces[index->second]->toContour());
        }
      }
    }
  }

  {
    // 検出した駒・升を元に, より正確な盤面の矩形を検出する.
    // 先手から見て盤面の上辺
    optional<cv::Vec4f> top;
    {
      vector<cv::Point2f> points;
      for (int x = 0; x < 9; x++) {
        if (s.detected[x][0]) {
          points.push_back(s.detected[x][0]->mean());
        }
      }
      if (points.size() > 1) {
        cv::Vec4f v;
        cv::fitLine(cv::Mat(points), v, cv::DIST_L2, 0, 0.01, 0.01);
        top = v;
      }
    }
    optional<cv::Vec4f> bottom;
    {
      vector<cv::Point2f> points;
      for (int x = 0; x < 9; x++) {
        if (s.detected[x][8]) {
          points.push_back(s.detected[x][8]->mean());
        }
      }
      if (points.size() > 1) {
        cv::Vec4f v;
        cv::fitLine(cv::Mat(points), v, cv::DIST_L2, 0, 0.01, 0.01);
        bottom = v;
      }
    }
    optional<cv::Vec4f> left;
    {
      vector<cv::Point2f> points;
      for (int y = 0; y < 9; y++) {
        if (s.detected[0][y]) {
          points.push_back(s.detected[0][y]->mean());
        }
      }
      if (points.size() > 1) {
        cv::Vec4f v;
        cv::fitLine(cv::Mat(points), v, cv::DIST_L2, 0, 0.01, 0.01);
        left = v;
      }
    }
    optional<cv::Vec4f> right;
    {
      vector<cv::Point2f> points;
      for (int y = 0; y < 9; y++) {
        if (s.detected[8][y]) {
          points.push_back(s.detected[8][y]->mean());
        }
      }
      if (points.size() > 1) {
        cv::Vec4f v;
        cv::fitLine(cv::Mat(points), v, cv::DIST_L2, 0, 0.01, 0.01);
        right = v;
      }
    }
    if (top && bottom && left && right) {
      auto tl = Intersection(*top, *left);
      auto tr = Intersection(*top, *right);
      auto br = Intersection(*bottom, *right);
      auto bl = Intersection(*bottom, *left);
      if (tl && tr && br && bl) {
        // tl, tr, br, bl は駒中心を元に計算しているので 8x8 の範囲しか無い. 半マス分増やす
        cv::Point2d topLeft = (*tl) + (((*tl) - (*tr)) / 16) + (((*tl) - (*bl)) / 16);
        cv::Point2d topRight = (*tr) + (((*tr) - (*tl)) / 16) + (((*tr) - (*br)) / 16);
        cv::Point2d bottomRight = (*br) + (((*br) - (*bl)) / 16) + (((*br) - (*tr)) / 16);
        cv::Point2d bottomLeft = (*bl) + (((*bl) - (*br)) / 16) + (((*bl) - (*tl)) / 16);

        Contour preciseOutline;
        preciseOutline.points = {topLeft, topRight, bottomRight, bottomLeft};
        preciseOutline.area = fabs(cv::contourArea(preciseOutline.points));
        s.preciseOutline = preciseOutline;
      }
    }
  }
}

void CreateWarpedBoard(cv::Mat const &frame, Status &s, Statistics const &stat) {
  using namespace std;
  optional<Contour> preciseOutline = stat.preciseOutline;
  if (!stat.preciseOutline || !stat.aspectRatio || !stat.squareArea) {
    return;
  }
  // 台形補正. キャプチャ画像と同じ面積で, アスペクト比が Status.aspectRatio と等しいサイズとなるような台形補正済み画像を作る.
  double area = *stat.squareArea * 81;
  if (area > frame.size().area()) {
    // キャプチャ画像より盤面が広いことはありえない.
    return;
  }
  // a = w/h, w = a * h
  // area = w * h = a * h^2, h = sqrt(area/a), w = sqrt(area * a)
  int width = (int)round(sqrt(area * (*stat.aspectRatio)));
  int height = (int)round(sqrt(area / (*stat.aspectRatio)));
  vector<cv::Point2f> dst({
      cv::Point2f(width, 0),
      cv::Point2f(width, height),
      cv::Point2f(0, height),
      cv::Point2f(0, 0),
  });
  cv::Mat mtx = cv::getPerspectiveTransform(stat.preciseOutline->points, dst);
  if (stat.rotate) {
    cv::Mat tmp1;
    cv::warpPerspective(frame, tmp1, mtx, cv::Size(width, height));
    cv::rotate(tmp1, s.boardWarped, cv::ROTATE_180);
  } else {
    cv::warpPerspective(frame, s.boardWarped, mtx, cv::Size(width, height));
  }
}

} // namespace

Session::Session() {
  s = std::make_shared<Status>();
  stop = false;
  std::thread th(std::bind(&Session::run, this));
  this->th.swap(th);
}

Session::~Session() {
  stop = true;
  cv.notify_one();
  th.join();
}

void Session::run() {
  using namespace std;
  while (!stop) {
    std::unique_lock<std::mutex> lock(mut);
    cv.wait(lock, [this]() { return !queue.empty() || stop; });
    if (stop) {
      lock.unlock();
      break;
    }
    cv::Mat frame = queue.front();
    queue.pop_front();
    lock.unlock();

    cv::cvtColor(frame, frame, cv::COLOR_RGB2GRAY);

    auto s = std::make_shared<Status>();
    s->stableBoardMaxSimilarity = BoardImage::kStableBoardMaxSimilarity;
    s->stableBoardThreshold = BoardImage::kStableBoardThreshold;
    s->blackResign = this->s->blackResign;
    s->whiteResign = this->s->whiteResign;
    s->boardReady = this->s->boardReady;
    s->wrongMove = this->s->wrongMove;
    s->width = frame.size().width;
    s->height = frame.size().height;
    Img::FindContours(frame, s->contours, s->squares, s->pieces);
    FindBoard(frame, *s);
    FindPieces(frame, *s);
    stat.update(*s);
    CreateWarpedBoard(frame, *s, stat);
    if (auto prePlayers = this->prePlayers; !players && prePlayers) {
      if (prePlayers->black) {
        if (auto move = prePlayers->black->next(game.position, game.moves, game.handBlack, game.handWhite); move) {
          move->decideSuffix(game.position);
          game.moves.push_back(*move);
        }
      }
      players = prePlayers;
      this->prePlayers = nullptr;
    }
    stat.push(s->boardWarped, *s, game, detected, players != nullptr);
    if (players && detected.size() == game.moves.size()) {
      if (game.moves.size() % 2 == 0) {
        // 次が先手番
        if (players->black && !s->blackResign) {
          auto move = players->black->next(game.position, game.moves, game.handBlack, game.handWhite);
          if (move) {
            move->decideSuffix(game.position);
            optional<Square> lastTo;
            if (game.moves.size() > 0) {
              lastTo = game.moves.back().to;
            }
            game.moves.push_back(*move);
            cout << (char const *)StringFromMoveWithOptionalLast(*move, lastTo).c_str() << endl;
          } else {
            s->blackResign = true;
            cout << "先手番AIが投了" << endl;
          }
          cout << "========================" << endl;
          cout << (char const *)game.position.debugString().c_str();
          cout << "------------------------" << endl;
        }
      } else {
        // 次が後手番
        if (players->white && !s->whiteResign) {
          auto move = players->white->next(game.position, game.moves, game.handWhite, game.handBlack);
          if (move) {
            move->decideSuffix(game.position);
            optional<Square> lastTo;
            if (game.moves.size() > 0) {
              lastTo = game.moves.back().to;
            }
            game.moves.push_back(*move);
            cout << (char const *)StringFromMoveWithOptionalLast(*move, lastTo).c_str() << endl;
          } else {
            s->whiteResign = true;
            cout << "後手番AIが投了" << endl;
          }
          cout << "========================" << endl;
          cout << (char const *)game.position.debugString().c_str();
          cout << "------------------------" << endl;
        }
      }
    }
    s->waitingMove = detected.size() != game.moves.size();
    s->game = game;
    if (!stat.stableBoardHistory.empty()) {
      s->stableBoard = stat.stableBoardHistory.back().back().image;
    }
    this->s = s;
  }
}

void Session::push(cv::Mat const &frame) {
  {
    std::lock_guard<std::mutex> lock(mut);
    queue.clear();
    queue.push_back(frame);
  }
  cv.notify_all();
}

} // namespace sci
