#include <shogi_camera/shogi_camera.hpp>

#include <opencv2/imgproc.hpp>

#include <numbers>

namespace sci {

void PieceBook::Entry::each(Color color, std::function<void(cv::Mat const &, bool cut)> cb) const {
  cv::Mat img;
  int total = 0;
  int cut = 0;
  total += blackLast.size();
  for (auto const &it : blackLast) {
    if (it.cut) {
      cut++;
    }
  }
  total += whiteLast.size();
  for (auto const &it : whiteLast) {
    if (it.cut) {
      cut++;
    }
  }
  bool onlyCut = cut >= (total - cut) && total > 2;

  if (color == Color::Black) {
    for (auto const &entry : blackLast) {
      if (!onlyCut || entry.cut) {
        cb(entry.mat.clone(), entry.cut);
      }
    }
    for (auto const &entry : whiteLast) {
      if (!onlyCut || entry.cut) {
        cv::rotate(entry.mat, img, cv::ROTATE_180);
        cb(img, entry.cut);
      }
    }
  } else {
    for (auto const &entry : whiteLast) {
      if (!onlyCut || entry.cut) {
        cb(entry.mat.clone(), entry.cut);
      }
    }
    for (auto const &entry : blackLast) {
      if (!onlyCut || entry.cut) {
        cv::rotate(entry.mat, img, cv::ROTATE_180);
        cb(img, entry.cut);
      }
    }
  }
}

void PieceBook::Entry::push(PieceBook::Image const &img, Color color) {
  if (color == Color::Black) {
    if (blackLast.size() >= kMaxLastImageCount) {
      if (!img.cut) {
        return;
      }
      bool ok = false;
      for (auto it = blackLast.begin(); it != blackLast.end(); it++) {
        if (!it->cut) {
          blackLast.erase(it);
          ok = true;
          break;
        }
      }
      if (!ok) {
        blackLast.pop_front();
      }
    }
    blackLast.push_back(img);
  } else {
    PieceBook::Image tmp;
    tmp.cut = img.cut;
    cv::rotate(img.mat, tmp.mat, cv::ROTATE_180);
    if (whiteLast.size() >= kMaxLastImageCount) {
      if (!img.cut) {
        return;
      }
      bool ok = false;
      for (auto it = whiteLast.begin(); it != whiteLast.end(); it++) {
        if (!it->cut) {
          whiteLast.erase(it);
          ok = true;
          break;
        }
      }
      if (!ok) {
        whiteLast.pop_front();
      }
    }
    whiteLast.push_back(tmp);
  }
}

void PieceBook::each(Color color, std::function<void(Piece, cv::Mat const &)> cb) const {
  for (auto const &it : store) {
    PieceUnderlyingType piece = it.first;
    it.second.each(color, [&cb, piece, color](cv::Mat const &img, bool cut) {
      cb(static_cast<PieceUnderlyingType>(piece) | static_cast<PieceUnderlyingType>(color), img);
    });
  }
}

void PieceBook::update(Position const &position, cv::Mat const &board, Status const &s) {
  using namespace std;
  cv::Mat bin;
  Img::Bin(board, bin);

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
        cv::Point2f wpCenter = wPiece->mean();
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
        cv::Mat mask(bin.size(), bin.type(), cv::Scalar(0, 0, 0));
        vector<cv::Point> points;
        for (auto const &p : nearest->points) {
          points.push_back(cv::Point((int)round(p.x), (int)round(p.y)));
        }
        cv::fillConvexPoly(mask, points, cv::Scalar(255, 255, 255));
        cv::polylines(mask, points, true, cv::Scalar(0, 0, 0), 3);
        cv::Mat masked = cv::Mat::zeros(bin.size(), bin.type());
        cv::bitwise_and(bin, mask, masked);

        cv::Point2f center = nearest->mean();
        double direction = atan2(nearest->direction.y, nearest->direction.x) * 180 / numbers::pi;
        cv::Mat rot = cv::getRotationMatrix2D(center, direction - 90 + 180, 1);
        cv::Mat rotated;
        cv::warpAffine(masked, rotated, rot, board.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT);
        cv::Mat part(cv::Size(rect.width, rect.height), board.type());
        cv::Rect bounds(center.x - rect.width / 2, center.y - rect.height / 2, rect.width, rect.height);
        Img::Bitblt(rotated, part, -bounds.x, -bounds.y);
        PieceUnderlyingType p = RemoveColorFromPiece(piece);
        Color color = ColorFromPiece(piece);
        PieceBook::Image tmp;
        tmp.mat = part.clone();
        tmp.cut = true;
        store[p].push(tmp, color);
      } else {
        auto roi = Img::PieceROI(board, x, y);
        PieceUnderlyingType p = RemoveColorFromPiece(piece);
        Color color = ColorFromPiece(piece);
        PieceBook::Image tmp;
        tmp.cut = false;
        if (color == Color::White) {
          cv::rotate(roi, tmp.mat, cv::ROTATE_180);
        } else {
          tmp.mat = roi.clone();
        }
        Img::Bin(tmp.mat, tmp.mat);
        store[p].push(tmp, color);
      }
    }
  }
}

std::string PieceBook::toPng() const {
  int rows = (int)store.size();
  int w = 0;
  int h = 0;
  std::map<Piece, int> count;
  each(Color::Black, [&](Piece piece, cv::Mat const &img) {
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
  cv::Mat all(cv::Size(width, height), CV_8UC4, cv::Scalar(255, 255, 255, 255));
  int row = 0;
  for (auto const &it : store) {
    int column = 0;
    it.second.each(Color::Black, [&](cv::Mat const &img, bool cut) {
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
