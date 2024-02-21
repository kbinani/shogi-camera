#include "shogi_camera_input.cpp"
#include <shogi_camera_input/shogi_camera_input.hpp>

#include <colormap/colormap.h>
#include <iostream>
#include <opencv2/highgui.hpp>

using namespace std;
using namespace sci;

namespace {
cv::Scalar ScalarFromColor(colormap::Color const &c) {
  cv::Scalar ret;
  ret.val[0] = c.b * 255;
  ret.val[1] = c.g * 255;
  ret.val[2] = c.r * 255;
  ret.val[3] = c.a * 255;
  return ret;
}
} // namespace

int main(int argc, const char *argv[]) {
  cv::Mat before = cv::imread(argv[1], cv::IMREAD_GRAYSCALE);
  cv::Mat after = cv::imread(argv[2], cv::IMREAD_GRAYSCALE);
  auto [a, b] = Equalize(after, before);

  int x = 7;
  int y = 1;
  auto bp = PieceROI(b, 7, 1).clone();
  auto ap = PieceROI(a, 7, 1).clone();
  double sim = Similarity(bp, ap, 10, 0.5f);
  cout << sim << endl;
  cv::adaptiveThreshold(ap, ap, 255, cv::THRESH_BINARY, cv::ADAPTIVE_THRESH_GAUSSIAN_C, 5, 0);
  cv::adaptiveThreshold(bp, bp, 255, cv::THRESH_BINARY, cv::ADAPTIVE_THRESH_GAUSSIAN_C, 5, 0);

  int width = a.size().width;
  int height = a.size().height;
  cv::Mat all(cv::Size(width * 3, height), CV_8UC4, cv::Scalar(255, 255, 255, 255));
  Bitblt(b, all, 0, 0);
  Bitblt(a, all, width * 2, 0);

  Bitblt(bp, all, width, 0);
  Bitblt(ap, all, width + bp.size().width, 0);
  //  {
  //    double theta = 30.0;// 180.0f * std::numbers::pi;
  //    float t[2][3] = {{1, 0, 0}, {0, 1, 0}};
  //    t[0][0] = cos(theta);
  //    t[0][1] =
  //    t[0][2] = 0;
  //    t[1][2] = 0;
  //    cv::Mat m = cv::getRotationMatrix2D(cv::Point2f(a71.size().width * 0.5f, a71.size().height * 0.5f), theta, 1);
  //    cout << "tx=" << m.at<double>(0, 2) << "; ty=" << m.at<double>(1, 2) << endl;
  //    cv::Mat rotated;
  ////    cv::warpAffine(a71, rotated, cv::Mat(2, 3, CV_32F, t), rotated.size(), cv::INTER_LINEAR, cv::BORDER_TRANSPARENT);
  //    cv::warpAffine(a71, rotated, m, a71 .size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT);
  //    Bitblt(rotated, all, width, 0);
  //    Bitblt(a71, all, width + a71.size().width, 0);
  //  }
  cv::cvtColor(all, all, cv::COLOR_GRAY2RGB);
  cv::imshow("preview", all);
  cv::waitKey();
  return 0;
}
