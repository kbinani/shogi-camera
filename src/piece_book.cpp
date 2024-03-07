#include <shogi_camera/shogi_camera.hpp>

#include "base64.hpp"
#include <opencv2/imgproc.hpp>

#include <iostream>
#include <numbers>

namespace sci {

void PieceBook::Entry::each(Color color, std::function<void(cv::Mat const &, std::optional<PieceShape> shape)> cb) const {
  cv::Mat img;
  std::optional<PieceShape> shape;
  if (sumCount > 0) {
    PieceShape ps;
    ps.apex = sumApex / (float)sumCount;
    ps.point1 = sumPoint1 / (float)sumCount;
    ps.point2 = sumPoint2 / (float)sumCount;
    shape = ps;
  }
  int total = 0;
  int cut = 0;
  total += black.size();
  for (auto const &it : black) {
    if (it.cut) {
      cut++;
    }
  }
  total += white.size();
  for (auto const &it : white) {
    if (it.cut) {
      cut++;
    }
  }
  bool onlyCut = cut >= (total - cut) && total > 2;

  if (color == Color::Black) {
    for (auto const &entry : black) {
      if (!onlyCut || entry.cut) {
        cb(entry.mat.clone(), shape);
      }
    }
    for (auto const &entry : white) {
      if (!onlyCut || entry.cut) {
        cv::rotate(entry.mat, img, cv::ROTATE_180);
        cb(img, shape);
      }
    }
  } else {
    for (auto const &entry : white) {
      if (!onlyCut || entry.cut) {
        cb(entry.mat.clone(), shape);
      }
    }
    for (auto const &entry : black) {
      if (!onlyCut || entry.cut) {
        cv::rotate(entry.mat, img, cv::ROTATE_180);
        cb(img, shape);
      }
    }
  }
}

void PieceBook::Entry::push(cv::Mat const &mat, Color color, std::optional<PieceShape> shape) {
  if (shape) {
    sumCount++;
    sumApex += cv::Point2d(shape->apex);
    sumPoint1 += cv::Point2d(shape->point1);
    sumPoint2 += cv::Point2d(shape->point2);
  }
  if (color == Color::Black) {
    if (black.size() >= kMaxLastImageCount) {
      if (!shape) {
        return;
      }
      bool ok = false;
      for (auto it = black.begin(); it != black.end(); it++) {
        if (!it->cut) {
          black.erase(it);
          ok = true;
          break;
        }
      }
      if (!ok) {
        black.pop_front();
      }
    }
    Image img;
    img.mat = mat;
    img.cut = shape != std::nullopt;
    img.rect = cv::Rect(0, 0, mat.size().width, mat.size().height);
    black.push_back(img);
  } else {
    Image tmp;
    tmp.cut = shape != std::nullopt;
    tmp.rect = cv::Rect(0, 0, mat.size().width, mat.size().height);
    cv::rotate(mat, tmp.mat, cv::ROTATE_180);
    if (white.size() >= kMaxLastImageCount) {
      if (!shape) {
        return;
      }
      bool ok = false;
      for (auto it = white.begin(); it != white.end(); it++) {
        if (!shape) {
          white.erase(it);
          ok = true;
          break;
        }
      }
      if (!ok) {
        white.pop_front();
      }
    }
    white.push_back(tmp);
  }
}

void PieceBook::each(Color color, std::function<void(Piece, cv::Mat const &, std::optional<PieceShape> shape)> cb) const {
  for (auto const &it : store) {
    PieceUnderlyingType piece = it.first;
    it.second.each(color, [&cb, piece, color](cv::Mat const &img, std::optional<PieceShape> shape) {
      cb(static_cast<PieceUnderlyingType>(piece) | static_cast<PieceUnderlyingType>(color), img, shape);
    });
  }
}

void PieceBook::update(Position const &position, cv::Mat const &board, Status const &s) {
  using namespace std;
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
        store[p].push(part.clone(), color, nearest->toShape());
      } else {
        auto roi = Img::PieceROI(board, x, y);
        PieceUnderlyingType p = RemoveColorFromPiece(piece);
        Color color = ColorFromPiece(piece);
        cv::Mat tmp;
        if (color == Color::White) {
          cv::rotate(roi, tmp, cv::ROTATE_180);
          Img::Bin(tmp, tmp);
        } else {
          Img::Bin(roi, tmp);
        }
        store[p].push(tmp, color, nullopt);
      }
    }
  }

  // 駒画像のサイズを揃える
  int width = 0;
  int height = 0;
  for (auto const &i : store) {
    for (auto const &j : i.second.black) {
      width = std::max(width, j.mat.size().width);
      height = std::max(height, j.mat.size().height);
    }
    for (auto const &j : i.second.white) {
      width = std::max(width, j.mat.size().width);
      height = std::max(height, j.mat.size().height);
    }
  }
  if (width > 0 && height > 0) {
    for (auto &i : store) {
      i.second.resize(width, height);
    }
  }
}

void PieceBook::Entry::resize(int width, int height) {
  for (auto &i : black) {
    i.resize(width, height);
  }
  for (auto &i : white) {
    i.resize(width, height);
  }
}

void PieceBook::Image::resize(int width, int height) {
  if (mat.size().width == width && mat.size().height == height) {
    return;
  }
  cv::Mat bu = mat;
  cv::Mat tmp = cv::Mat::zeros(height, width, mat.type());
  int dx = width / 2 - (rect.x + rect.width / 2);
  int dy = height / 2 - (rect.y + rect.height / 2);
  rect = cv::Rect(rect.x + dx, rect.y + dy, rect.width, rect.height);
  for (int x = 0; x < rect.width; x++) {
    for (int y = 0; y < rect.height; y++) {
      tmp.at<cv::Vec3b>(x - dx, y - dy) = bu.at<cv::Vec3b>(x, y);
    }
  }
  mat = tmp;
}

std::string PieceBook::toPng() const {
  int rows = (int)store.size();
  int w = 0;
  int h = 0;
  std::map<Piece, int> count;
  each(Color::Black, [&](Piece piece, cv::Mat const &img, std::optional<PieceShape> shape) {
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
    it.second.each(Color::Black, [&](cv::Mat const &img, std::optional<PieceShape> shape) {
      int x = column * w;
      int y = row * h;
      cv::Mat color;
      cv::cvtColor(img, color, cv::COLOR_GRAY2BGR);
      Img::Bitblt(color, all, x, y);
      cv::rectangle(all,
                    cv::Point(x + 1, y + 1), cv::Point(x + w - 2, y + h - 2),
                    shape ? cv::Scalar(255, 0, 0) : cv::Scalar(0, 0, 255));
      column++;
    });
    row++;
  }
  return Img::EncodeToPng(all);
}

} // namespace sci
