#include <opencv2/calib3d.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/core/types_c.h>
#include <opencv2/features2d.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgproc/types_c.h>

#include <iostream>
#include <numbers>
#include <set>
#include <vector>

#include "base64.hpp"
#include <shogi_camera_input/shogi_camera_input.hpp>

namespace sci {

namespace {
int const N = 11;
int const thresh = 50;

double Angle(cv::Point pt1, cv::Point pt2, cv::Point pt0) {
  double dx1 = pt1.x - pt0.x;
  double dy1 = pt1.y - pt0.y;
  double dx2 = pt2.x - pt0.x;
  double dy2 = pt2.y - pt0.y;
  return (dx1 * dx2 + dy1 * dy2) / sqrt((dx1 * dx1 + dy1 * dy1) * (dx2 * dx2 + dy2 * dy2) + 1e-10);
}

cv::Point2d Rotate(cv::Point2d const &p, double radian) {
  return cv::Point2d(cos(radian) * p.x - sin(radian) * p.y, sin(radian) * p.x + cos(radian) * p.y);
}

float Distance(cv::Point2f const &a, cv::Point2f const &b) {
  float dx = a.x - b.x;
  float dy = a.y - b.y;
  return sqrtf(dx * dx + dy * dy);
}

float Angle(cv::Point2f const &a) {
  return atan2f(a.y, a.x);
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

std::optional<cv::Point2f> PieceDirection(std::vector<cv::Point2f> const &points) {
  using namespace std;
  if (points.size() != 5) {
    return nullopt;
  }
  vector<int> indices;
  for (int i = 0; i < 5; i++) {
    indices.push_back(i);
  }
  sort(indices.begin(), indices.end(), [&points](int a, int b) {
    float lengthA = cv::norm(points[a] - points[(a + 1) % 5]);
    float lengthB = cv::norm(points[b] - points[(b + 1) % 5]);
    return lengthA < lengthB;
  });
  // 最も短い辺が隣り合っている場合, 将棋の駒らしいと判定する.
  int min0 = indices[0];
  int min1 = indices[1];
  if (min1 < min0) {
    swap(min0, min1);
  }
  if (min0 + 1 != min1 && !(min0 == 0 && min1 == 4)) {
    return nullopt;
  }
  if (min0 == 0 && min1 == 4) {
    swap(min0, min1);
  }
  // 将棋の駒の頂点
  cv::Point2f apex = points[min1];
  // 底辺の中点
  cv::Point2f midBottom = (points[(min0 + 3) % 5] + points[(min0 + 4) % 5]) * 0.5;

  return apex - midBottom;
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

cv::Point2d Normalize(cv::Point2d const &a) {
  return a / cv::norm(a);
}

void Normalize(cv::Mat &a) {
  double min, max;
  cv::minMaxLoc(a, &min, &max);
  a = (a - min) / (max - min);
}

std::optional<cv::Vec4f> FitLine(std::vector<cv::Point2f> const &points) {
  if (points.size() < 2) {
    return std::nullopt;
  }
  cv::Vec4f line;
  cv::fitLine(cv::Mat(points), line, CV_DIST_L2, 0, 0.01, 0.01);
  return line;
}

// 2 つの画像を同じサイズになるよう変形する
std::pair<cv::Mat, cv::Mat> Equalize(cv::Mat const &a, cv::Mat const &b) {
  using namespace std;
  if (a.size() == b.size()) {
    return make_pair(a.clone(), b.clone());
  }
  int width = std::max(a.size().width, b.size().width);
  int height = std::max(a.size().height, b.size().height);
  cv::Size size(width, height);
  cv::Mat ra;
  cv::Mat rb;
  cv::resize(a, ra, size);
  cv::resize(b, rb, size);
  return make_pair(ra.clone(), rb.clone());
}

void FindContours(cv::Mat const &image, Status &s) {
  using namespace std;
  s.contours.clear();
  s.squares.clear();
  s.pieces.clear();

  cv::Mat timg(image);
  cv::cvtColor(image, timg, CV_RGB2GRAY);

  cv::Size size = image.size();
  s.width = size.width;
  s.height = size.height;

  cv::Mat gray0(image.size(), CV_8U), gray;

  vector<vector<cv::Point>> contours;
  double area = size.width * size.height;

  int ch[] = {0, 0};
  mixChannels(&timg, 1, &gray0, 1, ch, 1);

  for (int l = 0; l < N; l++) {
    if (l == 0) {
      Canny(gray0, gray, 5, thresh, 5);
    } else {
      gray = gray0 >= (l + 1) * 255 / N;
    }

    findContours(gray, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

    for (size_t i = 0; i < contours.size(); i++) {
      auto contour = make_shared<Contour>();
      approxPolyDP(cv::Mat(contours[i]), contour->points, arcLength(cv::Mat(contours[i]), true) * 0.02, true);

      if (!isContourConvex(cv::Mat(contour->points))) {
        continue;
      }
      contour->area = fabs(contourArea(cv::Mat(contour->points)));

      if (contour->area <= area / 648.0) {
        continue;
      }
      s.contours.push_back(contour);

      if (area / 81.0 <= contour->area) {
        continue;
      }
      switch (contour->points.size()) {
      case 4: {
        // アスペクト比が 0.6 未満の四角形を除去
        if (contour->aspectRatio() < 0.6) {
          break;
        }
        double maxCosine = 0;

        for (int j = 2; j < 5; j++) {
          // find the maximum cosine of the angle between joint edges
          double cosine = fabs(Angle(contour->points[j % 4], contour->points[j - 2], contour->points[j - 1]));
          maxCosine = std::max(maxCosine, cosine);
        }

        // if cosines of all angles are small
        // (all angles are ~90 degree) then write quandrange
        // vertices to resultant sequence
        if (maxCosine >= 0.3) {
          break;
        }
        s.squares.push_back(contour);
        break;
      }
      case 5: {
        if (auto pc = PieceContour::Make(contour->points); pc && pc->aspectRatio >= 0.6) {
          s.pieces.push_back(pc);
        }
        break;
      }
      }
    }
  }
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
        cv::fitLine(cv::Mat(points), v, CV_DIST_L2, 0, 0.01, 0.01);
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
        cv::fitLine(cv::Mat(points), v, CV_DIST_L2, 0, 0.01, 0.01);
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
        cv::fitLine(cv::Mat(points), v, CV_DIST_L2, 0, 0.01, 0.01);
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
        cv::fitLine(cv::Mat(points), v, CV_DIST_L2, 0, 0.01, 0.01);
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
  cv::Mat tmp1;
  cv::warpPerspective(frame, tmp1, mtx, cv::Size(width, height));
  if (stat.rotate) {
    cv::Mat tmp2;
    cv::rotate(tmp1, tmp2, cv::ROTATE_180);
    cv::cvtColor(tmp2, s.boardWarped, CV_RGB2GRAY);
  } else {
    cv::cvtColor(tmp1, s.boardWarped, CV_RGB2GRAY);
  }
}

cv::Mat PieceROI(cv::Mat const &board, int x, int y, float shrink = 1) {
  int w = board.size().width;
  int h = board.size().height;
  double sw = w / 9.0;
  double sh = h / 9.0;
  double cx = sw * (x + 0.5);
  double cy = sh * (y + 0.5);
  int x0 = (int)round(cx - sw * 0.5 * shrink);
  int y0 = (int)round(cy - sh * 0.5 * shrink);
  int x1 = std::min(w, (int)round(cx + sw * 0.5 * shrink));
  int y1 = std::min(h, (int)round(cy + sh * 0.5 * shrink));
  return cv::Mat(board, cv::Rect(x0, y0, x1 - x0, y1 - y0));
}

void Compare(BoardImage const &before, BoardImage const &after, CvPointSet &buffer, std::array<std::array<double, 9>, 9> *similarity = nullptr) {
  using namespace std;

  // 2 枚の盤面画像を比較する. 変動が検出された升目を buffer に格納する.

  buffer.clear();
  auto [b, a] = Equalize(before.image, after.image);
  double sim[9][9];
  for (int y = 0; y < 9; y++) {
    for (int x = 0; x < 9; x++) {
      cv::Mat pb = PieceROI(b, x, y).clone();
      cv::Mat pa = PieceROI(a, x, y).clone();
      cv::adaptiveThreshold(pb, pb, 255, cv::THRESH_BINARY, cv::ADAPTIVE_THRESH_GAUSSIAN_C, 5, 0);
      cv::adaptiveThreshold(pa, pa, 255, cv::THRESH_BINARY, cv::ADAPTIVE_THRESH_GAUSSIAN_C, 5, 0);
      double s = cv::matchShapes(pb, pa, cv::CONTOURS_MATCH_I1, 0);
      sim[x][y] = s;
      if (similarity) {
        (*similarity)[x][y] = s;
      }
    }
  }
  for (int y = 0; y < 9; y++) {
    for (int x = 0; x < 9; x++) {
      double s = sim[x][y];
      if (s > BoardImage::kStableBoardThreshold) {
        buffer.insert(cv::Point(x, y));
      }
    }
  }
}

void Bitblt(cv::Mat const &src, cv::Mat &dst, int x, int y) {
  float t[2][3] = {{1, 0, 0}, {0, 1, 0}};
  t[0][2] = x;
  t[1][2] = y;
  cv::warpAffine(src, dst, cv::Mat(2, 3, CV_32F, t), dst.size(), cv::INTER_LINEAR, cv::BORDER_TRANSPARENT);
}

void PrintAsBase64(cv::Mat const &image, std::string const &title) {
  using namespace std;
  vector<uchar> buffer;
  cv::imencode(".png", image, buffer);
  string cbuffer;
  copy(buffer.begin(), buffer.end(), back_inserter(cbuffer));
  cout << "== " << title << endl;
  cout << base64::to_base64(cbuffer) << endl;
  cout << "--" << endl;
}

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

// 2 枚の画像を比較する. right を ±degrees 度, x と y 方向にそれぞれ ±width*translationRatio, ±height*translationRatio 移動して画像の一致度を計算し, 最大の一致度を返す.
double Similarity(cv::Mat const &left, cv::Mat const &right, int degrees = 5, float translationRatio = 0.5f) {
  auto [a, b] = Equalize(left, right);
  cv::adaptiveThreshold(b, b, 255, cv::THRESH_BINARY, cv::ADAPTIVE_THRESH_GAUSSIAN_C, 5, 0);
  cv::adaptiveThreshold(a, a, 255, cv::THRESH_BINARY, cv::ADAPTIVE_THRESH_GAUSSIAN_C, 5, 0);
  int w = a.size().width;
  int h = a.size().height;
  int cx = w / 2;
  int cy = h / 2;
  int dx = (int)round(w * translationRatio);
  int dy = (int)round(h * translationRatio);
  float minSum = std::numeric_limits<float>::max();
  int minDegrees;
  int minDx;
  int minDy;
  for (int t = -degrees; t <= degrees; t++) {
    cv::Mat m = cv::getRotationMatrix2D(cv::Point2f(cx, cy), t, 1);
    cv::Mat rotated;
    cv::warpAffine(a, rotated, m, a.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT);

    for (int iy = -dy; iy <= dy; iy++) {
      for (int ix = -dx; ix <= dx; ix++) {
        float sum = 0;
        for (int j = 0; j < h; j++) {
          for (int i = 0; i < w; i++) {
            if (0 <= i + ix && i + ix < w && 0 <= j + iy && j + iy < h) {
              float diff = b.at<uint8_t>(i, j) - rotated.at<uint8_t>(i + ix, j + iy);
              sum += diff * diff;
            } else {
              float diff = b.at<uint8_t>(i, j);
              sum += diff * diff;
            }
          }
        }
        if (minSum > sum) {
          minSum = sum;
          minDx = ix;
          minDy = iy;
          minDegrees = t;
        }
        minSum = std::min(minSum, sum);
      }
    }
  }

  return 1 - minSum / (w * h * 255.0 * 255.0);
}

bool IsPromoted(cv::Mat const &pieceBefore, cv::Mat const &pieceAfter) {
  double sim = Similarity(pieceBefore, pieceAfter);
  std::cout << __FUNCTION__ << "; sim=" << sim << std::endl;
  return sim < 0.98;
}

// 手番 color の駒が from から to に移動したとき, 成れる条件かどうか.
bool CanPromote(Square from, Square to, Color color) {
  if (color == Color::Black) {
    return from.rank <= Rank::Rank3 || to.rank <= Rank::Rank3;
  } else {
    return from.rank >= Rank::Rank7 || to.rank >= Rank::Rank7;
  }
}

void AppendPromotion(Move &mv, cv::Mat const &boardBefore, cv::Mat const &boardAfter) {
  if (!mv.from || IsPromotedPiece(mv.piece)) {
    return;
  }
  if (!CanPromote(*mv.from, mv.to, mv.color)) {
    return;
  }
  auto bp = PieceROI(boardBefore, mv.from->file, mv.from->rank);
  auto ap = PieceROI(boardAfter, mv.to.file, mv.to.rank);
  if (IsPromoted(bp, ap)) {
    mv.piece = Promote(mv.piece);
    mv.promote_ = true;
  } else {
    mv.promote_ = false;
  }
}
} // namespace

std::optional<cv::Point2f> Contour::direction(float length) const {
  if (auto d = PieceDirection(points); d) {
    double norm = cv::norm(*d);
    return *d / norm * length;
  } else {
    return std::nullopt;
  }
}

std::shared_ptr<PieceContour> PieceContour::Make(std::vector<cv::Point2f> const &points) {
  using namespace std;
  if (points.size() != 5) {
    return nullptr;
  }
  vector<int> indices;
  for (int i = 0; i < 5; i++) {
    indices.push_back(i);
  }
  sort(indices.begin(), indices.end(), [&points](int a, int b) {
    float lengthA = cv::norm(points[a] - points[(a + 1) % 5]);
    float lengthB = cv::norm(points[b] - points[(b + 1) % 5]);
    return lengthA < lengthB;
  });
  // 最も短い辺が隣り合っている場合, 将棋の駒らしいと判定する.
  int min0 = indices[0];
  int min1 = indices[1];
  if (min1 < min0) {
    swap(min0, min1);
  }
  if (min0 + 1 != min1 && !(min0 == 0 && min1 == 4)) {
    return nullptr;
  }
  if (min0 == 0 && min1 == 4) {
    swap(min0, min1);
  }
  // 将棋の駒の頂点
  cv::Point2f apex = points[min1];
  // 底辺の中点
  cv::Point2f bottom1 = points[(min0 + 3) % 5];
  cv::Point2f bottom2 = points[(min0 + 4) % 5];
  cv::Point2f midBottom = (bottom1 + bottom2) * 0.5;

  auto ret = make_shared<PieceContour>();
  for (int i = 0; i < 5; i++) {
    ret->points.push_back(points[(min1 + i) % 5]);
  }
  ret->area = fabs(cv::contourArea(ret->points));
  ret->direction = (apex - midBottom) / cv::norm(apex - midBottom);
  ret->aspectRatio = cv::norm(bottom1 - bottom2) / cv::norm(apex - midBottom);

  return ret;
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
  Compare(before, after, changes, &s.similarity);
  if (!stableBoardHistory.empty()) {
    auto const &last = stableBoardHistory.back();
    CvPointSet tmp;
    Compare(last[2], bi, tmp, &s.similarityAgainstStableBoard);
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
    book.update(g.position, board);
    if (false) {
      PrintAsBase64(board, "");
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
      Compare(before, after, changes);
      if (changes.size() == 0) {
        // 直前の stable board と比べて変化箇所が無い場合は無視.
        return;
      } else if (changes.size() > 2) {
        // 変化箇所が 3 以上ある場合, 将棋の駒以外の変化が盤面に現れているので無視.
        // TODO: まだ stable board が 1 個だけの場合, その stable board が間違った範囲を検出しているせいでずっとここを通過し続けてしまう可能性がある.
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
    rotate = true;
  }
  stableBoardHistory.push_back(history);
  g.moves.push_back(*move);
  g.apply(*move);
  book.update(g.position, board);
}

std::optional<Move> Statistics::Detect(cv::Mat const &boardBefore, cv::Mat const &boardAfter, CvPointSet const &changes, Position const &position, std::vector<Move> const &moves, Color const &color, std::deque<PieceType> const &hand, PieceBook const &book) {
  using namespace std;
  optional<Move> move;
  auto [before, after] = Equalize(boardBefore, boardAfter);
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
        cv::Mat roi = PieceROI(boardAfter, ch.x, ch.y).clone();
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
          double sim = Similarity(pi, roi);
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
          auto bp = PieceROI(before, x, y);
          auto ap = PieceROI(after, x, y);
          double sim = Similarity(bp, ap);
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
        AppendPromotion(mv, before, after);
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
          AppendPromotion(mv, before, after);
        } else {
          // p1 の駒が p0 の駒を取った.
          mv.from = MakeSquare(ch1.x, ch1.y);
          mv.to = MakeSquare(ch0.x, ch0.y);
          mv.piece = p1;
          mv.newHand = PieceTypeFromPiece(p0);
          AppendPromotion(mv, before, after);
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
        AppendPromotion(mv, before, after);
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
        AppendPromotion(mv, before, after);
        move = mv;
      } else {
        cout << "相手の駒を動かしている" << endl;
      }
    }
  }
  return move;
}

void PieceBook::Entry::each(Color color, std::function<void(cv::Mat const &)> cb) const {
  cv::Mat img;
  if (color == Color::Black) {
    if (blackInit) {
      cb(blackInit->clone());
    }
    if (whiteInit) {
      cv::rotate(*whiteInit, img, cv::ROTATE_180);
      cb(img);
    }
    if (blackLast) {
      cb(blackLast->clone());
    }
    if (whiteLast) {
      cv::rotate(*whiteLast, img, cv::ROTATE_180);
      cb(img);
    }
  } else {
    if (whiteInit) {
      cb(whiteInit->clone());
    }
    if (blackInit) {
      cv::rotate(*blackInit, img, cv::ROTATE_180);
      cb(img);
    }
    if (whiteLast) {
      cb(whiteLast->clone());
    }
    if (blackLast) {
      cv::rotate(*blackLast, img, cv::ROTATE_180);
      cb(img);
    }
  }
}

void PieceBook::Entry::push(cv::Mat const &img, Color color) {
  if (color == Color::Black) {
    if (!blackInit) {
      blackInit = img;
      return;
    }
    blackLast = img;
  } else {
    cv::Mat tmp;
    cv::rotate(img, tmp, cv::ROTATE_180);
    if (!whiteInit) {
      whiteInit = tmp;
      return;
    }
    whiteLast = tmp;
  }
}

void PieceBook::each(Color color, std::function<void(Piece, cv::Mat const &)> cb) const {
  for (auto const &it : store) {
    PieceUnderlyingType piece = it.first;
    it.second.each(color, [&cb, piece, color](cv::Mat const &img) {
      cb(static_cast<PieceUnderlyingType>(piece) | static_cast<PieceUnderlyingType>(color), img);
    });
  }
}

void PieceBook::update(Position const &position, cv::Mat const &board) {
  for (int y = 0; y < 9; y++) {
    for (int x = 0; x < 9; x++) {
      Piece piece = position.pieces[x][y];
      if (piece == 0) {
        continue;
      }
      auto roi = PieceROI(board, x, y).clone();
      PieceUnderlyingType p = RemoveColorFromPiece(piece);
      Color color = ColorFromPiece(piece);
      store[p].push(roi, color);
    }
  }
}

void Game::apply(Move const &mv) {
  if (mv.from) {
    position.pieces[mv.from->file][mv.from->rank] = 0;
  } else {
    PieceType type = PieceTypeFromPiece(mv.piece);
    bool ok = false;
    auto &hd = hand(mv.color);
    for (auto it = hd.begin(); it != hd.end(); it++) {
      if (*it == type) {
        hd.erase(it);
        ok = true;
        break;
      }
    }
    if (!ok) {
      std::cout << "存在しない持ち駒を打った" << std::endl;
    }
  }
  if (mv.promote_ == true && !IsPromotedPiece(mv.piece)) {
    position.pieces[mv.to.file][mv.to.rank] = Promote(mv.piece);
  } else {
    position.pieces[mv.to.file][mv.to.rank] = mv.piece;
  }
  if (mv.newHand) {
    if (mv.color == Color::Black) {
      handBlack.push_back(*mv.newHand);
    } else {
      handWhite.push_back(*mv.newHand);
    }
  }
  std::cout << "========================" << std::endl;
  std::cout << (char const *)DebugStringFromPosition(position).c_str();
  std::cout << "------------------------" << std::endl;
}

std::u8string DebugStringFromPosition(Position const &p) {
  using namespace std;
  u8string ret;
  for (int y = 0; y < 9; y++) {
    for (int x = 0; x < 9; x++) {
      auto piece = p.pieces[x][y];
      if (ColorFromPiece(piece) == Color::White) {
        ret += u8"v";
      } else {
        ret += u8" ";
      }
      ret += ShortStringFromPieceTypeAndStatus(RemoveColorFromPiece(piece));
    }
    ret += u8"\n";
  }
  return ret;
}

Status::Status() {
}

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

    auto s = std::make_shared<Status>();
    s->stableBoardMaxSimilarity = BoardImage::kStableBoardMaxSimilarity;
    s->stableBoardThreshold = BoardImage::kStableBoardThreshold;
    FindContours(frame, *s);
    FindBoard(frame, *s);
    FindPieces(frame, *s);
    stat.update(*s);
    CreateWarpedBoard(frame, *s, stat);
    stat.push(s->boardWarped, *s, game);
    s->game = game;
    if (!stat.stableBoardHistory.empty()) {
      s->stableBoard = stat.stableBoardHistory.back()[2].image;
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
