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
      auto pb = PieceROI(b, x, y);
      auto pa = PieceROI(a, x, y);
      //      if (x == 0 && y == 0) {
      cv::adaptiveThreshold(pb, pb, 255, cv::THRESH_BINARY, cv::ADAPTIVE_THRESH_GAUSSIAN_C, 5, 0);
      cv::adaptiveThreshold(pa, pa, 255, cv::THRESH_BINARY, cv::ADAPTIVE_THRESH_GAUSSIAN_C, 5, 0);
      //      }
      //      auto [rpb, rpa] = Equalize(pb, pa);
      //      Normalize(rpb);
      //      Normalize(rpa);
      double s = cv::matchShapes(pb, pa, cv::CONTOURS_MATCH_I1, 0);
      sim[x][y] = s;
      minSim = std::min(minSim, s);
      maxSim = std::max(maxSim, s);
    }
  }
  cout << "minSim=" << minSim << "; maxSim=" << maxSim << endl;

  int width = b.size().width;
  int height = b.size().height;
  //  cv::Mat mid(cv::Size(width, height), CV_8UC3, cv::Scalar(255, 255, 255, 255));
  //  vector<vector<cv::Point>> cBefore;
  //  FindContoursSimple(b, cBefore);
  //  vector<vector<cv::Point>> cAfter;
  //  FindContoursSimple(a, cAfter);
  //  cv::cvtColor(a, a, cv::COLOR_GRAY2RGB);
  //  cv::cvtColor(b, b, cv::COLOR_GRAY2RGB);
  //  cv::drawContours(b, cBefore, -1, cv::Scalar(255, 0, 0));
  //  cv::drawContours(a, cAfter, -1, cv::Scalar(0, 255, 0));

  cv::Mat all(cv::Size(width * 3, height), CV_8UC3, cv::Scalar(255, 255, 255, 255));
  //  cv::threshold(b, b, 0, 255, cv::THRESH_OTSU);
  //  cv::threshold(a, a, 0, 255, cv::THRESH_OTSU);
  Bitblt(b, all, 0, 0);
  Bitblt(a, all, width * 2, 0);
  cv::cvtColor(all, all, cv::COLOR_GRAY2RGB);
  colormap::MATLAB::Jet cmap;
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
