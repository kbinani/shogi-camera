#include <shogi_camera_input/shogi_camera_input.hpp>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "base64.hpp"
#include <iostream>

namespace sci {

cv::Mat Img::PieceROI(cv::Mat const &board, int x, int y, float shrink) {
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

void Img::Compare(BoardImage const &before, BoardImage const &after, CvPointSet &buffer, std::array<std::array<double, 9>, 9> *similarity) {
  using namespace std;

  // 2 枚の盤面画像を比較する. 変動が検出された升目を buffer に格納する.

  buffer.clear();
  auto [b, a] = Equalize(before.image, after.image);
  double sim[9][9];
  for (int y = 0; y < 9; y++) {
    for (int x = 0; x < 9; x++) {
      cv::Mat pb = Img::PieceROI(b, x, y).clone();
      cv::Mat pa = Img::PieceROI(a, x, y).clone();
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

std::pair<cv::Mat, cv::Mat> Img::Equalize(cv::Mat const &a, cv::Mat const &b) {
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

double Img::Similarity(cv::Mat const &left, cv::Mat const &right, int degrees, float translationRatio) {
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

std::string Img::EncodeToBase64(cv::Mat const &image) {
  using namespace std;
  vector<uchar> buffer;
  cv::imencode(".png", image, buffer);
  string cbuffer;
  copy(buffer.begin(), buffer.end(), back_inserter(cbuffer));
  return base64::to_base64(cbuffer);
}

void Img::Bitblt(cv::Mat const &src, cv::Mat &dst, int x, int y) {
  float t[2][3] = {{1, 0, 0}, {0, 1, 0}};
  t[0][2] = x;
  t[1][2] = y;
  cv::warpAffine(src, dst, cv::Mat(2, 3, CV_32F, t), dst.size(), cv::INTER_LINEAR, cv::BORDER_TRANSPARENT);
}

} // namespace sci