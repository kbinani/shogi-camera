#include <shogi_camera/shogi_camera.hpp>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "base64.hpp"
#include <iostream>

namespace sci {

namespace {

double Angle(cv::Point pt1, cv::Point pt2, cv::Point pt0) {
  double dx1 = pt1.x - pt0.x;
  double dy1 = pt1.y - pt0.y;
  double dx2 = pt2.x - pt0.x;
  double dy2 = pt2.y - pt0.y;
  return (dx1 * dx2 + dy1 * dy2) / sqrt((dx1 * dx1 + dy1 * dy1) * (dx2 * dx2 + dy2 * dy2) + 1e-10);
}

} // namespace

cv::Rect Img::PieceROIRect(cv::Size const &size, int x, int y) {
  int w = size.width;
  int h = size.height;
  double sw = w / 9.0;
  double sh = h / 9.0;
  double cx = sw * (x + 0.5);
  double cy = sh * (y + 0.5);
  int x0 = (int)round(cx - sw * 0.5);
  int y0 = (int)round(cy - sh * 0.5);
  int x1 = std::min(w, (int)round(cx + sw * 0.5));
  int y1 = std::min(h, (int)round(cy + sh * 0.5));
  return cv::Rect(x0, y0, x1 - x0, y1 - y0);
}

cv::Mat Img::PieceROI(cv::Mat const &board, int x, int y) {
  return cv::Mat(board, PieceROIRect(board.size(), x, y));
}

void Img::Compare(BoardImage const &before, BoardImage const &after, CvPointSet &buffer, double similarity[9][9]) {
  using namespace std;

  // 2 枚の盤面画像を比較する. 変動が検出された升目を buffer に格納する.

  buffer.clear();
  auto [b, a] = Equalize(before.image, after.image);
  double sim[9][9];
  for (int y = 0; y < 9; y++) {
    for (int x = 0; x < 9; x++) {
      cv::Mat pb = Img::PieceROI(b, x, y).clone();
      cv::Mat pa = Img::PieceROI(a, x, y).clone();
      Bin(pb, pb);
      Bin(pa, pa);
      double s = cv::matchShapes(pb, pa, cv::CONTOURS_MATCH_I1, 0);
      sim[x][y] = s;
      if (similarity) {
        similarity[x][y] = s;
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

double Img::Similarity(cv::Mat const &left_, cv::Mat const &right, int degrees, float translationRatio) {
  using namespace std;
  int w = right.size().width;
  int h = right.size().height;
  cv::Mat left;
  if (left_.size() != right.size()) {
    cv::resize(left_, left, right.size());
  } else {
    left = left_;
  }

  float cx = w / 2.0f;
  float cy = h / 2.0f;
  int dx = (int)round(w * translationRatio);
  int dy = (int)round(h * translationRatio);
  float maxSim = numeric_limits<float>::lowest();
  int maxDegrees = 99;
  int maxDx = 99;
  int maxDy = 99;
  for (int t = -degrees; t <= degrees; t++) {
    cv::Mat m = cv::getRotationMatrix2D(cv::Point2f(cx, cy), t, 1);
    cv::Mat rotated(h, w, left.type(), cv::Scalar(0, 0, 0));
    cv::warpAffine(left, rotated, m, left.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT);

    for (int iy = -dy; iy <= dy; iy++) {
      for (int ix = -dx; ix <= dx; ix++) {
        float sum = 0;
        int count = 0;
        for (int j = 0; j < h; j++) {
          for (int i = 0; i < w; i++) {
            if (0 <= i + ix && i + ix < w && 0 <= j + iy && j + iy < h) {
              float diff = (float)left.at<uint8_t>(i, j) - (float)rotated.at<uint8_t>(i + ix, j + iy);
              sum += diff * diff;
              count++;
            }
          }
        }
        float sim = 1 - sum / (count * 255.f * 255.f);
        if (maxSim < sim) {
          maxSim = sim;
          maxDx = ix;
          maxDy = iy;
          maxDegrees = t;
        }
      }
    }
  }
  return maxSim;
}

double Img::ComparePiece(cv::Mat const &left_, cv::Mat const &right_, Color targetColor, std::optional<PieceShape> shape, int degrees, float translationRatio) {
  using namespace std;
  cv::Mat left;
  cv::Mat right = right_;
  if (left_.size() != right_.size()) {
    cv::resize(left_, left, right_.size());
  } else {
    left = left_;
  }

  int w = right_.size().width;
  int h = right_.size().height;
  float cx = w / 2.0f;
  float cy = h / 2.0f;
  int dx = (int)round(w * translationRatio);
  int dy = (int)round(h * translationRatio);
  float maxSim = numeric_limits<float>::lowest();
  float minSim = numeric_limits<float>::max();
  int maxDegrees = 99;
  int maxDx = 99;
  int maxDy = 99;
  vector<cv::Point2f> outline;
  if (shape) {
    shape->poly(cv::Point2f(cx, cy), outline, targetColor);
  }
  cv::Mat maxImg;
  cv::Mat minImg;
  for (int t = -degrees; t <= degrees; t++) {
    for (int iy = -dy; iy <= dy; iy++) {
      for (int ix = -dx; ix <= dx; ix++) {
        cv::Mat m = cv::getRotationMatrix2D(cv::Point2f(cx + ix, cy + iy), t, 1);
        m.at<double>(0, 2) -= ix;
        m.at<double>(1, 2) -= iy;
        cv::Mat rotated(h, w, left.type(), cv::Scalar(0, 0, 0));
        cv::warpAffine(left, rotated, m, left.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT);
        Bin(rotated, rotated);

        cv::Mat dbg;
        cv::cvtColor(rotated, dbg, cv::COLOR_GRAY2BGR);

        float sum = 0;
        int count = 0;
        for (int j = 0; j < h; j++) {
          for (int i = 0; i < w; i++) {
            if (!outline.empty() && cv::pointPolygonTest(outline, cv::Point2f(i, j), false) <= 0) {
              auto &c = dbg.at<cv::Vec3b>(j, i);
              c[0] = 0;
              c[1] = 0;
              c[2] = 255;
              continue;
            }
            float diff = (float)rotated.at<uint8_t>(i, j) - (float)right.at<uint8_t>(i, j);
            sum += diff * diff;
            count++;
          }
        }
        float sim = 1 - sum / (count * 255.f * 255.f);
        //        cout << "b64png(similarity_t=" << t << "_ix=" << ix << "_iy=" << iy << "_sim=" << sim << "):" << base64::to_base64(EncodeToPng(dbg)) << endl;
        if (maxSim < sim) {
          maxSim = sim;
          maxDx = ix;
          maxDy = iy;
          maxDegrees = t;
          maxImg = dbg.clone();
        }
        if (minSim > sim) {
          minSim = sim;
          minImg = dbg.clone();
        }
      }
    }
  }
  return maxSim;
}

std::string Img::EncodeToPng(cv::Mat const &image) {
  using namespace std;
  vector<uchar> buffer;
  cv::imencode(".png", image, buffer);
  string cbuffer;
  copy(buffer.begin(), buffer.end(), back_inserter(cbuffer));
  return cbuffer;
}

void Img::Bitblt(cv::Mat const &src, cv::Mat &dst, int x, int y) {
  float t[2][3] = {{1, 0, 0}, {0, 1, 0}};
  t[0][2] = x;
  t[1][2] = y;
  cv::warpAffine(src, dst, cv::Mat(2, 3, CV_32F, t), dst.size(), cv::INTER_LINEAR, cv::BORDER_TRANSPARENT);
}

void Img::FindContours(cv::Mat const &image, std::vector<std::shared_ptr<Contour>> &contours, std::vector<std::shared_ptr<Contour>> &squares, std::vector<std::shared_ptr<PieceContour>> &pieces) {
  using namespace std;
  int const N = 11;
  int const thresh = 50;

  contours.clear();
  squares.clear();
  pieces.clear();

  cv::Size size = image.size();

  cv::Mat gray0(image.size(), CV_8U), gray;

  vector<vector<cv::Point>> all;
  double area = size.width * size.height;

  int ch[] = {0, 0};
  mixChannels(&image, 1, &gray0, 1, ch, 1);

  for (int l = 0; l < N; l++) {
    if (l == 0) {
      Canny(gray0, gray, 5, thresh, 5);
    } else {
      gray = gray0 >= (l + 1) * 255 / N;
    }

    findContours(gray, all, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

    for (size_t i = 0; i < all.size(); i++) {
      auto contour = make_shared<Contour>();
      approxPolyDP(cv::Mat(all[i]), contour->points, arcLength(cv::Mat(all[i]), true) * 0.02, true);

      if (!isContourConvex(cv::Mat(contour->points))) {
        continue;
      }
      contour->area = fabs(contourArea(cv::Mat(contour->points)));

      if (contour->area <= area / 648.0) {
        continue;
      }
      contours.push_back(contour);

      if (area / 81.0 <= contour->area) {
        continue;
      }
      switch (contour->points.size()) {
      case 4: {
        // アスペクト比が 0.6 未満の四角形を除去
        if (contour->aspectRatio() < 0.6) {
          break;
        }
        double maxCosine = 0;

        for (int j = 2; j < 5; j++) {
          // find the maximum cosine of the angle between joint edges
          double cosine = fabs(Angle(contour->points[j % 4], contour->points[j - 2], contour->points[j - 1]));
          maxCosine = std::max(maxCosine, cosine);
        }

        // if cosines of all angles are small
        // (all angles are ~90 degree) then write quandrange
        // vertices to resultant sequence
        if (maxCosine >= 0.3) {
          break;
        }
        squares.push_back(contour);
        break;
      }
      case 5: {
        if (auto pc = PieceContour::Make(contour->points); pc && pc->aspectRatio >= 0.6) {
          pieces.push_back(pc);
        }
        break;
      }
      }
    }
  }
}

void Img::Bin(cv::Mat const &input, cv::Mat &output) {
  cv::adaptiveThreshold(input, output, 255, cv::THRESH_BINARY, cv::ADAPTIVE_THRESH_GAUSSIAN_C, 5, 0);
}

} // namespace sci
