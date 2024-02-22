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
  cv::Mat img = cv::imread(argv[1], cv::IMREAD_GRAYSCALE);
  int x0 = atoi(argv[2]);
  int y0 = atoi(argv[3]);
  int x1 = atoi(argv[4]);
  int y1 = atoi(argv[5]);
  int x2 = atoi(argv[6]);
  int y2 = atoi(argv[7]);

  auto aroi = Img::PieceROI(img, x0, y0);
  auto broi = Img::PieceROI(img, x1, y1);
  auto croi = Img::PieceROI(img, x2, y2);
  int w = std::max({aroi.size().width, broi.size().width, croi.size().width});
  int h = std::max({aroi.size().height, broi.size().height, croi.size().height});
  cv::Mat a, b, c;
  cv::resize(aroi, a, cv::Size(w, h));
  cv::resize(broi, b, cv::Size(w, h));
  cv::resize(croi, c, cv::Size(w, h));

  cv::Mat all(cv::Size(w * 3, h), CV_8UC3, cv::Scalar(255, 255, 255, 255));
  vector<cv::Mat> images = {a, b, c};
  for (int i = 0; i < images.size(); i++) {
    for (int j = 0; j < images.size(); j++) {
      auto sim = Img::Similarity(images[i], images[j]);
      cout << "(" << i << ", " << j << ") " << sim << endl;
    }
  }

  cv::adaptiveThreshold(a, a, 255, cv::THRESH_BINARY, cv::ADAPTIVE_THRESH_GAUSSIAN_C, 5, 0);
  cv::adaptiveThreshold(b, b, 255, cv::THRESH_BINARY, cv::ADAPTIVE_THRESH_GAUSSIAN_C, 5, 0);
  cv::adaptiveThreshold(c, c, 255, cv::THRESH_BINARY, cv::ADAPTIVE_THRESH_GAUSSIAN_C, 5, 0);

  Img::Bitblt(a, all, 0, 0);
  Img::Bitblt(b, all, w, 0);
  Img::Bitblt(c, all, w * 2, 0);
  //  cv::Mat after = cv::imread(argv[2], cv::IMREAD_GRAYSCALE);
  //  auto [a, b] = Img::Equalize(after, before);
  //
  //  int x = 7;
  //  int y = 1;
  //  auto bp = Img::PieceROI(b, 7, 1).clone();
  //  auto ap = Img::PieceROI(a, 7, 1).clone();
  //  double sim = Img::Similarity(bp, ap, 10, 0.5f);
  //  cout << sim << endl;
  //  cv::adaptiveThreshold(ap, ap, 255, cv::THRESH_BINARY, cv::ADAPTIVE_THRESH_GAUSSIAN_C, 5, 0);
  //  cv::adaptiveThreshold(bp, bp, 255, cv::THRESH_BINARY, cv::ADAPTIVE_THRESH_GAUSSIAN_C, 5, 0);
  //  {
  //    int w = ap.size().width;
  //    int h = ap.size().height;
  //    float rate = 0.2f;
  //    int dw = (int)floor(w * rate);
  //    int dh = (int)floor(h * rate);
  //    cv::Scalar fill(0, 0, 0);
  //    cv::rectangle(ap, cv::Rect(0, 0, w, dh), fill, -1);
  //    cv::rectangle(ap, cv::Rect(0, h - dh, w, dh), fill, -1);
  //    cv::rectangle(ap, cv::Rect(0, 0, dw, h), fill, -1);
  //    cv::rectangle(ap, cv::Rect(w - dw, 0, dw, h), fill, -1);
  //  }
  //
  //  int width = a.size().width;
  //  int height = a.size().height;
  //  cv::Mat all(cv::Size(width * 3, height), CV_8UC4, cv::Scalar(255, 255, 255, 255));
  //  Img::Bitblt(b, all, 0, 0);
  //  Img::Bitblt(a, all, width * 2, 0);
  //
  //  Img::Bitblt(bp, all, width, 0);
  //  Img::Bitblt(ap, all, width + bp.size().width, 0);
  //  //  {
  //  //    double theta = 30.0;// 180.0f * std::numbers::pi;
  //  //    float t[2][3] = {{1, 0, 0}, {0, 1, 0}};
  //  //    t[0][0] = cos(theta);
  //  //    t[0][1] =
  //  //    t[0][2] = 0;
  //  //    t[1][2] = 0;
  //  //    cv::Mat m = cv::getRotationMatrix2D(cv::Point2f(a71.size().width * 0.5f, a71.size().height * 0.5f), theta, 1);
  //  //    cout << "tx=" << m.at<double>(0, 2) << "; ty=" << m.at<double>(1, 2) << endl;
  //  //    cv::Mat rotated;
  //  ////    cv::warpAffine(a71, rotated, cv::Mat(2, 3, CV_32F, t), rotated.size(), cv::INTER_LINEAR, cv::BORDER_TRANSPARENT);
  //  //    cv::warpAffine(a71, rotated, m, a71 .size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT);
  //  //    Bitblt(rotated, all, width, 0);
  //  //    Bitblt(a71, all, width + a71.size().width, 0);
  //  //  }
  //  cv::cvtColor(all, all, cv::COLOR_GRAY2RGB);
  cv::imshow("preview", all);
  cv::waitKey();
  return 0;
}
