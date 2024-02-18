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
  auto [b, a] = Equalize(before, after);
  double sim[9][9];
  double minSim = numeric_limits<double>::max();
  double maxSim = numeric_limits<double>::lowest();
  for (int y = 0; y < 9; y++) {
    for (int x = 0; x < 9; x++) {
      auto pb = PieceROI(before, x, y).clone();
      auto pa = PieceROI(after, x, y).clone();
      auto [rpb, rpa] = Equalize(pb, pa);
      double minB, maxB;
      cv::minMaxLoc(rpb, &minB, &maxB);
      double minA, maxA;
      cv::minMaxLoc(rpa, &minA, &maxA);
      double minimum = std::min(minA, minB);
      double maximum = std::max(maxA, maxB);
      rpa = (rpa - minimum) / (maximum - minimum);
      rpb = (rpb - minimum) / (maximum - minimum);
      cv::Mat diff;
      cv::absdiff(rpa, rpb, diff);
      auto sum = cv::sum(diff);
      double s = sum.val[0] / double(rpb.size().area());
      sim[x][y] = s;
      minSim = std::min(minSim, s);
      maxSim = std::max(maxSim, s);
    }
  }
  int width = b.size().width;
  int height = b.size().height;
  cv::Mat all(cv::Size(width * 3, height), CV_8UC3, cv::Scalar(255, 255, 255, 255));
  Bitblt(b, all, 0, 0);
  Bitblt(a, all, width * 2, 0);
  cv::cvtColor(all, all, cv::COLOR_GRAY2RGB);
  colormap::transform::Seismic cmap;
  for (int y = 0; y < 9; y++) {
    for (int x = 0; x < 9; x++) {
      double s = sim[x][y];
      double v = (s - minSim) / (maxSim - minSim);
      auto color = cmap.getColor(v);
      int x0 = x * width / 9;
      int x1 = (x + 1) * width / 9;
      int y0 = y * height / 9;
      int y1 = (y + 1) * height / 9;
      cv::rectangle(all, cv::Rect(width + x0, y0, x1 - x0, y1 - y0), ScalarFromColor(color), -1);
    }
  }
  cv::imshow("preview", all);
  cv::waitKey();
  return 0;
}
