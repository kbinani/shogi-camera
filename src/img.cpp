#include <shogi_camera/shogi_camera.hpp>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "base64.hpp"
#include <iostream>

using namespace std;

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

void Img::DetectPiece(cv::Mat const &img, uint8_t board[9][9], double similarity[9][9]) {
  int const inset = 5;
  if (img.size().width <= 9 * inset * 2) {
    return;
  }
  if (img.size().height <= 9 * inset * 2) {
    return;
  }
  double sim[9][9];
  double minimum = numeric_limits<double>::max();
  double maximum = numeric_limits<double>::lowest();
  for (int y = 0; y < 9; y++) {
    for (int x = 0; x < 9; x++) {
      board[x][y] = 0;
      cv::Mat roi;
      Img::PieceROI(img, x, y).convertTo(roi, CV_32F);
      int w = roi.size().width - 2 * inset;
      int h = roi.size().height - 2 * inset;
      cv::Mat part = roi(cv::Rect(inset, inset, w, h));
      cv::Mat vsum = cv::Mat::zeros(cv::Size(w, h), CV_32F);
      cv::Mat diff = cv::Mat::zeros(cv::Size(w, h), CV_32F);
      float weightSum = 0;
      for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
          if (dx == 0 && dy == 0) {
            continue;
          }
          cv::absdiff(part, roi(cv::Rect(inset + dx, inset + dy, w, h)), diff);
          vsum += diff;
          weightSum++;
        }
      }
      double sum = cv::sum(vsum)[0];
      double s = sum / (255.0 * weightSum * w * h);
      sim[x][y] = s;
      minimum = std::min(minimum, s);
      maximum = std::max(maximum, s);
    }
  }
  double minPositive, maxPositive, minNegative, maxNegative;
  minPositive = minNegative = numeric_limits<double>::max();
  maxPositive = maxNegative = numeric_limits<double>::lowest();
  for (int y = 0; y < 9; y++) {
    for (int x = 0; x < 9; x++) {
      double s = sim[x][y];
      double v = (s - minimum) / (maximum - minimum);
      if (similarity) {
        similarity[x][y] = v;
      }
      if (v > BoardImage::kPieceDetectThreshold) {
        board[x][y] = 1;
        minPositive = std::min(minPositive, v);
        maxPositive = std::max(maxPositive, v);
      } else {
        minNegative = std::min(minNegative, v);
        maxNegative = std::max(maxNegative, v);
      }
    }
  }
#if 0
  cout << "positive = [" << minPositive << ", " << maxPositive << "], negative = [" << minNegative << ", " << maxNegative << "]" << endl;
#endif
}

void Img::DetectBoardChange(BoardImage const &before, BoardImage const &after, CvPointSet &buffer, double similarity[9][9]) {
  // 2 枚の盤面画像を比較する. 変動が検出された升目を buffer に格納する.

  buffer.clear();

  uint8_t boardBefore[9][9];
  uint8_t boardAfter[9][9];
  DetectPiece(before.blurGray, boardBefore, nullptr);
  DetectPiece(after.blurGray, boardAfter, similarity);
  for (int y = 0; y < 9; y++) {
    for (int x = 0; x < 9; x++) {
      if (boardBefore[x][y] != boardAfter[x][y]) {
        buffer.insert(cv::Point(x, y));
      }
    }
  }
}

pair<cv::Mat, cv::Mat> Img::Equalize(cv::Mat const &a, cv::Mat const &b) {
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

double Img::Similarity(cv::Mat const &before, cv::Mat const &after, int x, int y) {
  constexpr int degrees = 5;
  constexpr float translationRatio = 0.1f;

  cv::Mat a = Img::PieceROI(after, x, y).clone();
  Bin(a, a);

  int w = a.size().width;
  int h = a.size().height;

  float cx = w / 2.0f;
  float cy = h / 2.0f;
  int dx = (int)round(w * translationRatio);
  int dy = (int)round(h * translationRatio);
  float maxSim = numeric_limits<float>::lowest();
  int maxDegrees = 99;
  int maxDx = 99;
  int maxDy = 99;
  for (int t = -degrees; t <= degrees; t++) {
    for (int iy = -dy; iy <= dy; iy++) {
      for (int ix = -dx; ix <= dx; ix++) {
        float cx = before.size().width / 9.0f * (x + 0.5f) + ix;
        float cy = before.size().height / 9.0f * (y + 0.5f) + iy;
        cv::Mat m = cv::getRotationMatrix2D(cv::Point2f(cx, cy), t, 1);
        m.at<double>(0, 2) -= (cx - w / 2);
        m.at<double>(1, 2) -= (cy - h / 2);
        cv::Mat b;
        cv::warpAffine(before, b, m, cv::Size(w, h), cv::INTER_LINEAR, cv::BORDER_CONSTANT);
        Bin(b, b);

        cv::Mat mSum = cv::Mat::zeros(h, w, CV_32F);
        cv::Mat mDiffU8;
        cv::absdiff(b, a, mDiffU8);
        cv::Mat mDiff;
        mDiffU8.convertTo(mDiff, CV_32F);
        cv::Mat mSq;
        cv::multiply(mDiff, mDiff, mSq);
        cv::add(mSq, mSum, mSum);

        cv::Scalar sSum = cv::sum(mSum);
        float sim = 1 - sSum[0] / (w * h * 255.f * 255.f);
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

pair<double, cv::Mat> Img::ComparePiece(cv::Mat const &board,
                                        int x, int y,
                                        cv::Mat const &tmpl,
                                        Color targetColor,
                                        optional<PieceShape> shape,
                                        hwm::task_queue &pool,
                                        ComparePieceCache &cache) {
  constexpr int degrees = 10;
  constexpr float translationRatio = 0.2f;

  int width = board.size().width;
  int height = board.size().height;

  int w = tmpl.size().width;
  int h = tmpl.size().height;
  int dx = (int)round(w * translationRatio);
  int dy = (int)round(h * translationRatio);
  float maxSim = numeric_limits<float>::lowest();
  cv::Mat maxImg;
  cv::Mat mask;
  int count = w * h;
  if (shape) {
    vector<cv::Point2f> outline;
    shape->poly(cv::Point2f(w * 0.5f, h * 0.5f), outline, targetColor);
    vector<cv::Point> points;
    for (auto const &p : outline) {
      points.push_back(p);
    }
    mask = cv::Mat::zeros(h, w, CV_8U);
    cv::fillConvexPoly(mask, points, cv::Scalar::all(255));
    cv::polylines(mask, points, true, cv::Scalar::all(0), PieceBook::kEdgeLineWidth);
    count = cv::countNonZero(mask);
  } else {
    mask = cv::Mat(h, w, CV_8U, cv::Scalar::all(255));
  }
  mutex mut;

  for (int t = -degrees; t <= degrees; t += 2) {
    deque<future<pair<float, cv::Mat>>> futures;
    for (int iy = -dy; iy <= dy; iy++) {
      for (int ix = -dx; ix <= dx; ix++) {
        futures.push_back(pool.enqueue(
            [width, height, x, y, w, h, count, &board, &mask, &tmpl, &cache, &mut](int t, int ix, int iy) {
              cv::Mat rotated;
              tuple<int, int, int> key(t, ix, iy);
              {
                lock_guard<mutex> lock(mut);
                auto found = cache.images.find(key);
                if (found != cache.images.end()) {
                  rotated = found->second;
                }
              }

              if (rotated.empty()) {
                float cx = width / 9.0f * (x + 0.5f) + ix;
                float cy = height / 9.0f * (y + 0.5f) + iy;
                cv::Mat m = cv::getRotationMatrix2D(cv::Point2f(cx, cy), t, 1);
                m.at<double>(0, 2) -= (cx - w / 2);
                m.at<double>(1, 2) -= (cy - h / 2);
                cv::warpAffine(board, rotated, m, tmpl.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT);
                cv::bitwise_and(rotated, mask, rotated);
                Bin(rotated, rotated);
                cv::bitwise_and(rotated, mask, rotated);

                lock_guard<mutex> lock(mut);
                cache.images[key] = rotated;
              }

              cv::Mat mSum = cv::Mat::zeros(h, w, CV_32F);
              cv::Mat mDiffU8;
              cv::absdiff(rotated, tmpl, mDiffU8);
              cv::Mat mDiff;
              mDiffU8.convertTo(mDiff, CV_32F);
              cv::Mat mSq;
              cv::multiply(mDiff, mDiff, mSq);
              cv::add(mSq, mSum, mSum, mask);

              cv::Scalar sSum = cv::sum(mSum);
              float sim = 1 - sSum[0] / (count * 255.0f * 2550.f);
              return make_pair(sim, rotated);
            },
            t, ix, iy));
      }
    }
    for (auto &f : futures) {
      auto [sim, img] = f.get();
      if (maxSim < sim) {
        maxSim = sim;
        maxImg = img;
      }
    }
    futures.clear();
  }
  return make_pair(maxSim, maxImg);
}

string Img::EncodeToPng(cv::Mat const &image) {
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

void Img::FindContours(cv::Mat const &image,
                       vector<shared_ptr<Contour>> &contours,
                       vector<shared_ptr<Contour>> &squares,
                       vector<shared_ptr<PieceContour>> &pieces) {
  int const N = 11;
  int const thresh = 50;

  contours.clear();
  squares.clear();
  pieces.clear();

  cv::Size size = image.size();

  cv::Mat gray0(image.size(), CV_8U), gray;

  vector<vector<cv::Point>> all;
  double area = size.width * size.height;
  double maxSquareArea = area / 81.0;

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

      switch (contour->points.size()) {
      case 4: {
        if (maxSquareArea <= contour->area) {
          continue;
        }
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
      case 5:
      case 6:
      case 7:
      case 8:
      case 9: {
        if (auto pc = PieceContour::Make(contour->points); pc && pc->aspectRatio >= 0.6 && pc->area < maxSquareArea) {
          pieces.push_back(pc);
        }
        break;
      }
      }
    }
  }
}

void Img::Bin(cv::Mat const &input, cv::Mat &output) {
  cv::adaptiveThreshold(input, output, 255, cv::THRESH_BINARY, cv::ADAPTIVE_THRESH_GAUSSIAN_C, 3, 0);
}

void Img::LUVFromBGR(cv::Mat const &input, cv::Mat &output) {
  cv::Mat luv;
  cv::cvtColor(input, luv, cv::COLOR_BGR2Luv);
  vector<cv::Mat> channels;
  cv::split(luv, channels);
  channels[0].convertTo(channels[0], CV_32F, 100.0f / 255.0f, 0.0f);
  channels[1].convertTo(channels[1], CV_32F, 354.0f / 255.0f, -134.0f);
  channels[2].convertTo(channels[2], CV_32F, 256.0f / 255.0f, -140.0f);
  cv::Mat out(input.size(), CV_32FC3);
  cv::merge(channels, out);
  output = out;
}

cv::Scalar Img::LUVFromBGR(cv::Scalar input) {
  cv::Mat tmp(cv::Size(1, 1), CV_8UC3, input);
  cv::Mat out;
  LUVFromBGR(tmp, out);
  auto v = out.at<cv::Vec3f>(0, 0);
  return cv::Scalar(v[0], v[1], v[2]);
}

static double SimilarityAgainstVermillion(cv::Mat const &bgr) {
  cv::Mat luv;
  Img::LUVFromBGR(bgr, luv);

  // vermillion: RGB(233, 81, 78)[0, 255], L*u*v(55.6863[0, 100], 115.882[-134, 220], 22.6353[-140, 122])

  cv::Mat diff;
  cv::absdiff(luv, cv::Scalar(55.6863f, 115.882f, 22.6353f), diff);
  cv::multiply(diff, diff, diff);
  vector<cv::Mat> channels;
  cv::split(diff, channels);
  cv::Mat out(bgr.size(), CV_32F, cv::Scalar::all(0));
  out += channels[1]; // u
  out += channels[2]; // v
  cv::sqrt(out, out);
  out = out / sqrt(256.0f * 256.0f + 354.0f * 354.0f) * -1 + 1;
  double min = 0.75, max = 0.83;
  out = (out - min) / (max - min);
  cv::Mat gray;
  out.convertTo(gray, CV_8U, 255);
  cv::threshold(gray, gray, 127, 255, cv::THRESH_BINARY);
  double average = cv::sum(gray)[0] / bgr.size().area();
#if 0
  static int cnt = 0;
  cnt++;
  LogPng(gray) << "sample_" << cnt << "_" << average;
#endif
  return average;
}

bool Img::Vermillion(cv::Mat const &before, cv::Mat const &after) {
  double simBefore = SimilarityAgainstVermillion(before);
  double simAfter = SimilarityAgainstVermillion(after);
#if 0
  cout << __FUNCTION__ << ", " << simBefore << " => " << simAfter << endl;
#endif
  double constexpr threshold = 8;
  return (simBefore < threshold && simAfter > threshold);
}

} // namespace sci
