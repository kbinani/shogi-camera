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
    // hack: use Canny instead of zero threshold level.
    // Canny helps to catch squares with gradient shading
    if (l == 0) {
      // apply Canny. Take the upper threshold from slider
      // and set the lower to 0 (which forces edges merging)
      Canny(gray0, gray, 5, thresh, 5);
      // dilate canny output to remove potential
      // holes between edge segments
      dilate(gray, gray, cv::Mat(), cv::Point(-1, -1));
    } else {
      // apply threshold if l!=0:
      //     tgray(x,y) = gray(x,y) < (l+1)*255/N ? 255 : 0
      gray = gray0 >= (l + 1) * 255 / N;
    }

    // find contours and store them all as a list
    findContours(gray, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

    std::vector<cv::Point> approx;

    // test each contour
    for (size_t i = 0; i < contours.size(); i++) {
      // approximate contour with accuracy proportional
      // to the contour perimeter
      approxPolyDP(cv::Mat(contours[i]), approx, arcLength(cv::Mat(contours[i]), true) * 0.02, true);

      if (!isContourConvex(cv::Mat(approx))) {
        continue;
      }

      Shape shape;
      shape.points = approx;
      shape.match = false;

      // square contours should have 4 vertices after approximation
      // relatively large area (to filter out noisy contours)
      // and be convex.
      // Note: absolute value of an area is used because
      // area may be positive or negative - in accordance with the
      // contour orientation
      shape.area = fabs(contourArea(cv::Mat(approx)));
      if (area / 324.0 < shape.area && shape.area < area / 81.0) {
        double maxCosine = 0;

        for (int j = 2; j < 5; j++) {
          // find the maximum cosine of the angle between joint edges
          double cosine = fabs(angle(approx[j % 4], approx[j - 2], approx[j - 1]));
          maxCosine = MAX(maxCosine, cosine);
        }

        // if cosines of all angles are small
        // (all angles are ~90 degree) then write quandrange
        // vertices to resultant sequence
        if (maxCosine < 0.3) {
          shape.match = true;
        }
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
