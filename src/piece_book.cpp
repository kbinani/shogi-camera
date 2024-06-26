#include <shogi_camera/shogi_camera.hpp>

#include "base64.hpp"
#include <opencv2/imgproc.hpp>

#include <iostream>
#include <numbers>

using namespace std;

namespace sci {

namespace {

int SizeRank(PieceUnderlyingType type) {
  auto t = PieceTypeFromPiece(type);
  switch (t) {
  case PieceType::Pawn:
  case PieceType::Lance:
  case PieceType::Knight:
    return 0;
  case PieceType::Silver:
  case PieceType::Gold:
    return 1;
  case PieceType::Bishop:
  case PieceType::Rook:
  case PieceType::King:
    return 2;
  }
}

} // namespace

void PieceBook::Entry::each(Color color, function<void(cv::Mat const &, optional<PieceShape> shape, bool cut)> cb) const {
  cv::Mat img;
  auto shape = this->shape();
  int total = 0;
  int cut = 0;
  total += images.size();
  for (auto const &it : images) {
    if (it.cut) {
      cut++;
    }
  }
  bool onlyCut = cut >= (total - cut) && total > 2;

  if (color == Color::Black) {
    for (auto const &entry : images) {
      if (!onlyCut || entry.cut) {
        cb(entry.mat.clone(), shape, entry.cut);
      }
    }
  } else {
    for (auto const &entry : images) {
      if (!onlyCut || entry.cut) {
        cv::rotate(entry.mat, img, cv::ROTATE_180);
        cb(img, shape, entry.cut);
      }
    }
  }
}

void PieceBook::Entry::push(cv::Mat const &mat, optional<PieceShape> shape) {
  if (shape) {
    sumCount++;
    sumApex += cv::Point2d(shape->apex);
    sumPoint1 += cv::Point2d(shape->point1);
    sumPoint2 += cv::Point2d(shape->point2);
  }
  if (images.size() >= kMaxNumImages) {
    if (!shape) {
      return;
    }
    for (auto it = images.begin(); it != images.end(); it++) {
      if (!it->cut) {
        images.erase(it);
        break;
      }
    }
  }
  Image img;
  img.cut = shape != nullopt;
  img.rect = cv::Rect(0, 0, mat.size().width, mat.size().height);
  img.mat = mat;
  images.push_back(img);
}

std::optional<PieceShape> PieceBook::Entry::shape() const {
  if (sumCount == 0) {
    return std::nullopt;
  }
  PieceShape ps;
  ps.apex = sumApex / (float)sumCount;
  ps.point1 = sumPoint1 / (float)sumCount;
  ps.point2 = sumPoint2 / (float)sumCount;
  return ps;
}

void PieceBook::each(Color color, function<void(Piece, cv::Mat const &, optional<PieceShape> shape, bool cut)> cb) const {
  for (auto const &it : store) {
    PieceUnderlyingType piece = it.first;
    it.second.each(color, [&cb, piece, color](cv::Mat const &img, optional<PieceShape> shape, bool cut) {
      cb(static_cast<PieceUnderlyingType>(piece) | static_cast<PieceUnderlyingType>(color), img, shape, cut);
    });
  }
}

void PieceBook::update(Position const &position, cv::Mat const &board, Status const &s) {
  for (int y = 0; y < 9; y++) {
    for (int x = 0; x < 9; x++) {
      Piece piece = position.pieces[x][y];
      if (piece == 0) {
        continue;
      }
      // pieces の中心が rect の中にありかつ最も近い物を選ぶ.
      cv::Rect2f rect = Img::PieceROIRect(board.size(), x, y);
      cv::Point2f center(rect.x + rect.width * 0.5f, rect.y + rect.height * 0.5f);
      shared_ptr<PieceContour> nearest;
      float minDistance = numeric_limits<float>::max();
      for (auto const &piece : s.pieces) {
        auto wPiece = PerspectiveTransform(piece, s.perspectiveTransform, s.rotate, board.size().width, board.size().height);
        if (!wPiece) {
          continue;
        }
        cv::Point2f wpCenter = wPiece->center();
        if (!rect.contains(wpCenter)) {
          continue;
        }
        float distance = cv::norm(center - wpCenter);
        if (minDistance > distance) {
          minDistance = distance;
          nearest = wPiece;
        }
      }
      if (nearest) {
        cv::Point2f center = nearest->center();
        double direction = atan2(nearest->direction.y, nearest->direction.x) * 180 / numbers::pi;
        cv::Mat rot = cv::getRotationMatrix2D(center, direction - 90 + 180, 1);
        cv::Mat rotated;
        cv::warpAffine(board, rotated, rot, board.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT);
        Img::Bin(rotated, rotated);

        cv::Mat mask(board.size(), board.type(), cv::Scalar::all(0));
        vector<cv::Point> points;
        for (auto const &p : nearest->points) {
          cv::Point2f pp = WarpAffine(p, rot);
          points.push_back(cv::Point((int)round(pp.x), (int)round(pp.y)));
        }
        cv::fillConvexPoly(mask, points, cv::Scalar::all(255));
        cv::polylines(mask, points, true, cv::Scalar::all(0), kEdgeLineWidth);

        cv::Mat masked = cv::Mat::zeros(board.size(), board.type());
        cv::bitwise_and(rotated, mask, masked);

        cv::Mat part(cv::Size(rect.width, rect.height), board.type());
        cv::Rect bounds(center.x - rect.width / 2, center.y - rect.height / 2, rect.width, rect.height);
        Img::Bitblt(masked, part, -bounds.x, -bounds.y);
        PieceUnderlyingType p = RemoveColorFromPiece(piece);
        Color color = ColorFromPiece(piece);
        store[p].push(part.clone(), nearest->toShape());
      } else {
        auto roi = Img::PieceROI(board, x, y);
        auto rect = Img::PieceROIRect(board.size(), x, y);
        PieceUnderlyingType p = RemoveColorFromPiece(piece);
        optional<cv::Mat> mask;
        if (auto hint = this->hint(p); hint) {
          mask = cv::Mat::zeros(roi.size(), roi.type());
          int cx = (int)round(rect.width * 0.5);
          int cy = (int)round(rect.height * 0.5);
          vector<cv::Point> points;
          hint->poly(cv::Point(cx, cy), points, Color::Black);
          cv::fillConvexPoly(*mask, points, cv::Scalar::all(255));
        }
        Color color = ColorFromPiece(piece);
        cv::Mat tmp;
        if (color == Color::White) {
          cv::rotate(roi, tmp, cv::ROTATE_180);
        } else {
          tmp = roi.clone();
        }
        if (mask) {
          cv::bitwise_and(tmp, *mask, tmp);
          Img::Bin(tmp, tmp);
          cv::bitwise_and(tmp, *mask, tmp);
        } else {
          Img::Bin(tmp, tmp);
        }
        store[p].push(tmp, nullopt);
      }
    }
  }

  // 駒画像のサイズを揃える
  int width = 0;
  int height = 0;
  for (auto const &i : store) {
    for (auto const &j : i.second.images) {
      width = std::max(width, j.mat.size().width);
      height = std::max(height, j.mat.size().height);
    }
  }
  if (width > 0 && height > 0) {
    for (auto &i : store) {
      i.second.resize(width, height);
    }
  }

  for (auto &i : store) {
    i.second.gc();
  }
}

optional<PieceShape> PieceBook::hint(PieceUnderlyingType type) const {
  auto found = store.find(type);
  if (found != store.end()) {
    if (auto shape = found->second.shape(); shape) {
      return *shape;
    }
  }
  struct Item {
    int size;
    uint64_t count;
    PieceShape shape;
  };
  vector<Item> items;
  int size = SizeRank(type);
  for (auto const &it : store) {
    int s = SizeRank(it.first);
    if (size > s) {
      continue;
    }
    if (auto shape = it.second.shape(); shape) {
      Item item;
      item.size = s;
      item.count = it.second.sumCount;
      item.shape = *shape;
      items.push_back(item);
    }
  }
  if (items.empty()) {
    return nullopt;
  }
  if (items.size() == 1) {
    return items[0].shape;
  }
  sort(items.begin(), items.end(), [size](Item const &a, Item const &b) -> bool {
    if (a.size == b.size) {
      return a.count > b.count;
    } else {
      return a.size < b.size;
    }
  });
  return items[0].shape;
}

void PieceBook::Entry::gc() {
  if (images.size() <= kMaxNumImages) {
    return;
  }
  for (int i = (int)images.size() - 1; i >= 0 && images.size() > kMaxNumImages; i--) {
    if (!images[i].cut) {
      images.erase(images.begin() + i);
    }
  }
  if (images.size() <= kMaxNumImages) {
    return;
  }
  vector<tuple<int, int, float>> sims;
  for (int i = 0; i < (int)images.size() - 1; i++) {
    cv::Mat im = images[i].mat;
    for (int j = i + 1; j < (int)images.size(); j++) {
      cv::Mat diffu8;
      cv::absdiff(im, images[j].mat, diffu8);
      cv::Mat diff;
      diffu8.convertTo(diff, CV_32F);
      cv::Scalar sum = cv::sum(diff);
      float sim = 1 - sum[0] / (im.size().width * im.size().height * 255.0f);
      sims.push_back(make_tuple(i, j, sim));
    }
  }
  vector<pair<int, float>> tscores;
  for (int k = 0; k < (int)images.size(); k++) {
    vector<float> values;
    float sum = 0;
    int count = 0;
    for (auto const &it : sims) {
      auto [i, j, sim] = it;
      if (i == k || j == k) {
        sum += sim;
        count++;
      } else {
        values.push_back(sim);
      }
    }
    float m = sum / count;
    // 自分(k)と他との類似度の平均値(m)が、自分以外の者同士の類似度からどの程度乖離しているかを調べる.
    cv::Scalar mean;
    cv::Scalar stddev;
    cv::meanStdDev(cv::Mat(values), mean, stddev);
    float t = 10 * (m - mean[0]) / stddev[0] + 50;
    // 自分と, 他の駒との類似度が低い場合, t は小さくなるはず
    tscores.push_back(make_pair(k, t));
  }
  sort(tscores.begin(), tscores.end(), [](auto const &a, auto const &b) {
    return a.second < b.second;
  });
#if 0
  cv::Mat all(images[0].mat.size().height, images[0].mat.size().width * images.size(), CV_8UC3, cv::Scalar::all(255));
  for (int i = 0; i < tscores.size(); i++) {
    int index = tscores[i].first;
    Img::Bitblt(images[index].mat, all, i * images[0].mat.size().width, 0);
  }
  cout << "b64png(sorted):" << base64::to_base64(Img::EncodeToPng(all)) << endl;
#endif
  vector<int> erasing;
  for (int i = 0; i < tscores.size() && i < (int)images.size() - kMaxNumImages; i++) {
    erasing.push_back(tscores[i].first);
  }
  sort(erasing.begin(), erasing.end(), [](int a, int b) { return a > b; });
  for (auto i : erasing) {
    images.erase(images.begin() + i);
  }
}

void PieceBook::Entry::resize(int width, int height) {
  for (auto &i : images) {
    i.resize(width, height);
  }
}

void PieceBook::Image::resize(int width, int height) {
  if (mat.size().width == width && mat.size().height == height) {
    return;
  }
  int w = mat.size().width;
  int h = mat.size().height;
  cv::Mat bu = mat;
  cv::Mat tmp = cv::Mat::zeros(cv::Size(width, height), mat.type());
  int dx = width / 2 - (rect.x + rect.width / 2);
  int dy = height / 2 - (rect.y + rect.height / 2);
  rect = cv::Rect(rect.x + dx, rect.y + dy, rect.width, rect.height);
  bu(cv::Rect(dx, dy, w - dx, h - dy)).copyTo(tmp(cv::Rect(0, 0, w - dx, h - dy)));
  mat = tmp;
}

string PieceBook::toPng() const {
  int rows = (int)store.size();
  int w = 0;
  int h = 0;
  map<Piece, int> count;
  each(Color::Black, [&](Piece piece, cv::Mat const &img, optional<PieceShape> shape, bool cut) {
    w = std::max(w, img.size().width);
    h = std::max(h, img.size().height);
    count[piece] += 1;
  });
  if (w == 0 || h == 0) {
    return "";
  }
  int columns = 0;
  for (auto const &it : count) {
    columns = std::max(columns, it.second);
  }
  if (columns == 0) {
    return "";
  }
  int width = w * columns;
  int height = rows * h;
  cv::Mat all(height, width, CV_8UC3, cv::Scalar::all(255));
  int row = 0;
  for (auto const &it : store) {
    int column = 0;
    it.second.each(Color::Black, [&](cv::Mat const &img, optional<PieceShape> shape, bool cut) {
      int x = column * w;
      int y = row * h;
      cv::Mat color;
      cv::cvtColor(img, color, cv::COLOR_GRAY2BGR);
      Img::Bitblt(color, all, x, y);
      cv::rectangle(all,
                    cv::Point(x + 1, y + 1), cv::Point(x + w - 2, y + h - 2),
                    cut ? cv::Scalar(255, 0, 0) : cv::Scalar(0, 0, 255));
      column++;
    });
    row++;
  }
  return Img::EncodeToPng(all);
}

} // namespace sci
