#include <opencv2/calib3d.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/core/types_c.h>
#include <opencv2/imgcodecs/ios.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgproc/types_c.h>

#include <iostream>
#include <numbers>
#include <vector>

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

float MeanAngle(std::initializer_list<float> values) {
  double x = 0;
  double y = 0;
  for (float v : values) {
    x += cos(v);
    y += sin(v);
  }
  return atan2(y, x);
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
      dilate(gray, gray, cv::Mat(), cv::Point(-1, -1));
    } else {
      gray = gray0 >= (l + 1) * 255 / N;
    }

    findContours(gray, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

    for (size_t i = 0; i < contours.size(); i++) {
      Contour contour;
      approxPolyDP(cv::Mat(contours[i]), contour.points, arcLength(cv::Mat(contours[i]), true) * 0.02, true);

      if (!isContourConvex(cv::Mat(contour.points))) {
        continue;
      }
      contour.area = fabs(contourArea(cv::Mat(contour.points)));

      if (contour.area <= area / 648.0) {
        continue;
      }
      s.contours.push_back(contour);

      if (area / 81.0 <= contour.area) {
        continue;
      }
      switch (contour.points.size()) {
      case 4: {
        // アスペクト比が 0.6 未満の四角形を除去
        if (contour.aspectRatio() < 0.6) {
          break;
        }
        double maxCosine = 0;

        for (int j = 2; j < 5; j++) {
          // find the maximum cosine of the angle between joint edges
          double cosine = fabs(Angle(contour.points[j % 4], contour.points[j - 2], contour.points[j - 1]));
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
        s.pieces.push_back(contour);
        break;
      }
      }
    }
  }
}

void FindBoard(cv::Mat const &frame, Status &s) {
  using namespace std;
  vector<Contour> squares;
  for (auto const &square : s.squares) {
    if (square.points.size() != 4) {
      continue;
    }
    squares.push_back(square);
  }
  if (squares.empty()) {
    return;
  }
  // 四角形の面積の中央値を升目の面積(squareArea)とする.
  sort(squares.begin(), squares.end(), [](Contour const &a, Contour const &b) { return a.area < b.area; });
  size_t mid = squares.size() / 2;
  if (squares.size() % 2 == 0) {
    auto const &a = squares[mid - 1];
    auto const &b = squares[mid];
    s.squareArea = (a.area + b.area) * 0.5;
    s.aspectRatio = (a.aspectRatio() + b.aspectRatio()) * 0.5;
  } else {
    s.squareArea = squares[mid].area;
    s.aspectRatio = squares[mid].aspectRatio();
  }

  // squares の各辺の傾きから, 盤面の向きを推定する.
  vector<double> angles;
  for (auto const &square : s.squares) {
    for (size_t i = 0; i < 3; i++) {
      auto const &a = square.points[i];
      auto const &b = square.points[i + 1];
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

  {
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
    double sumCosAngle = 0;
    int countAngle = 0;
    for (double const &angle : angles) {
      if (targetAngle - 5 <= angle && angle <= targetAngle + 5) {
        sumCosAngle += cos(angle / 180.0 * numbers::pi);
        countAngle += 1;
      }
    }
    float direction = Normalize90To90(acosf(sumCosAngle / countAngle));
    s.boardDirection = direction;

    // Session.boardDirection は対局者の向きにしたい.
    map<bool, int> vote;
    // square の長手方向から, direction を 90 度回して訂正するか, そのまま採用するか投票する.
    for (auto const &square : s.squares) {
      if (auto d = SquareDirection(square.points); d) {
        bool shouldCorrect = fabs(cos(*d - direction)) < 0.25;
        vote[shouldCorrect] += 1;
      }
    }
    // piece の頂点の向きから, direction を 90 度回して訂正するか, そのまま採用するか投票する.
    for (auto const &piece : s.pieces) {
      if (auto d = PieceDirection(piece.points); d) {
        float a = Normalize90To90(Angle(*d));
        bool shouldCorrect = fabs(cos(a - direction)) < 0.25;
        vote[shouldCorrect] += 1;
      }
    }
    if (vote[true] > (vote[true] + vote[false]) * 2 / 3) {
      direction = Normalize90To90(direction + numbers::pi * 0.5);
      s.boardDirection = direction;
    }
  }

  {
    // squares と pieces を -1 * boardDirection 回転した状態で矩形を検出する.
    vector<cv::Point2d> centers;
    for (auto const &square : s.squares) {
      auto center = Rotate(square.mean(), -s.boardDirection);
      centers.push_back(center);
    }
    for (auto const &piece : s.pieces) {
      auto center = Rotate(piece.mean(), -s.boardDirection);
      centers.push_back(center);
    }
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
  }
}
} // namespace

#if defined(__APPLE__)
cv::Mat Utility::MatFromUIImage(void *ptr) {
  cv::Mat image;
  UIImageToMat((__bridge UIImage *)ptr, image, true);
  return image;
}

void *Utility::UIImageFromMat(cv::Mat const &m) {
  return (__bridge_retained void *)MatToUIImage(m);
}
#endif // defined(__APPLE__)

std::optional<cv::Point2f> Contour::direction(float length) const {
  if (auto d = PieceDirection(points); d) {
    double norm = cv::norm(*d);
    return *d / norm * length;
  } else {
    return std::nullopt;
  }
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
    FindContours(frame, *s);
    FindBoard(frame, *s);
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
