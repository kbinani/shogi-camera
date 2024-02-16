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

void FindContours(cv::Mat const &image, Status &s) {
  s.contours.clear();
  s.squares.clear();
  s.pieces.clear();

  cv::Mat timg(image);
  cv::cvtColor(image, timg, CV_RGB2GRAY);

  cv::Size size = image.size();
  s.width = size.width;
  s.height = size.height;

  cv::Mat gray0(image.size(), CV_8U), gray;

  std::vector<std::vector<cv::Point>> contours;
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

void FindBoard(Status &s) {
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
  // 5 度単位でヒストグラムを作り, 最頻値を調べる. angle は [0, 90) に限定しているので index は [0, 17]
  array<int, 17> count;
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
  s.boardDirection = acos(sumCosAngle / countAngle);
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
    FindBoard(*s);
    this->s = s;
  }
  std::cout << 1 << std::endl;
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
