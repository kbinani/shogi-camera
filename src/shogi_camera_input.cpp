#include <opencv2/core/core.hpp>
#include <opencv2/core/types_c.h>
#include <opencv2/imgcodecs/ios.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgproc/types_c.h>

#include <iostream>
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

void FindContours(cv::Mat const &image, Session::Status &s) {
  s.contours.clear();
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

      if (contour.points.size() != 4 && contour.points.size() != 5) {
        continue;
      }

      if (!isContourConvex(cv::Mat(contour.points))) {
        continue;
      }

      contour.area = fabs(contourArea(cv::Mat(contour.points)));
      if (area / 324.0 < contour.area && contour.area < area / 81.0) {
        s.contours.push_back(contour);
      }
    }
  }
}

void FindBoard(Session::Status &s) {
  std::vector<Contour> squares;
  for (auto const &contour : s.contours) {
    if (contour.points.size() != 4) {
      continue;
    }
    squares.push_back(contour);
  }
  if (squares.empty()) {
    return;
  }
  // 四角形の面積の中央値を升目の面積(squareArea)とする.
  std::sort(squares.begin(), squares.end(), [](Contour const &a, Contour const &b) { return a.area < b.area; });
  size_t mid = squares.size() / 2;
  if (squares.size() % 2 == 0) {
    s.squareArea = (squares[mid - 1].area + squares[mid].area) * 0.5;
  } else {
    s.squareArea = squares[mid].area;
  }
}
} // namespace

#if defined(__APPLE__)
cv::Mat Session::MatFromUIImage(void *ptr) {
  cv::Mat image;
  UIImageToMat((__bridge UIImage *)ptr, image, true);
  return image;
}

void *Session::UIImageFromMat(cv::Mat const &m) {
  return (__bridge_retained void *)MatToUIImage(m);
}
#endif // defined(__APPLE__)

Session::Session() {
}

void Session::push(cv::Mat const &frame) {
  FindContours(frame, s);
  FindBoard(s);
}

} // namespace sci
