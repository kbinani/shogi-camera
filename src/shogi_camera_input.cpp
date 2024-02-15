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

double angle(cv::Point pt1, cv::Point pt2, cv::Point pt0) {
  double dx1 = pt1.x - pt0.x;
  double dy1 = pt1.y - pt0.y;
  double dx2 = pt2.x - pt0.x;
  double dy2 = pt2.y - pt0.y;
  return (dx1 * dx2 + dy1 * dy2) / sqrt((dx1 * dx1 + dy1 * dy1) * (dx2 * dx2 + dy2 * dy2) + 1e-10);
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

Session::Status Session::FindSquares(cv::Mat const &image) {
  Session::Status st;
  cv::Mat timg(image);
  cv::cvtColor(image, timg, CV_RGB2GRAY);

  cv::Size size = image.size();
  st.width = size.width;
  st.height = size.height;
  st.processed = timg.clone();

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
      Shape shape;
      approxPolyDP(cv::Mat(contours[i]), shape.points, arcLength(cv::Mat(contours[i]), true) * 0.02, true);

      if (shape.points.size() != 4 && shape.points.size() != 5) {
        continue;
      }

      if (!isContourConvex(cv::Mat(shape.points))) {
        continue;
      }

      shape.area = fabs(contourArea(cv::Mat(shape.points)));
      if (area / 324.0 < shape.area && shape.area < area / 81.0) {
        st.shapes.push_back(shape);
      }
    }
  }
  return st;
}

Session::Session() {
}

void Session::push(cv::Mat const &frame) {
}

} // namespace sci
