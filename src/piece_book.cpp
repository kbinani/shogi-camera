#include <shogi_camera/shogi_camera.hpp>

#include <opencv2/imgproc.hpp>

namespace sci {

void PieceBook::Entry::each(Color color, std::function<void(cv::Mat const &, bool)> cb) const {
  cv::Mat img;
  if (color == Color::Black) {
    if (blackInit) {
      cb(blackInit->mat_.clone(), true);
    }
    if (whiteInit) {
      cv::rotate(whiteInit->mat_, img, cv::ROTATE_180);
      cb(img, true);
    }
    for (auto const &entry : blackLast) {
      cb(entry.mat_.clone(), true);
    }
    for (auto const &entry : whiteLast) {
      cv::rotate(entry.mat_, img, cv::ROTATE_180);
      cb(img, true);
    }
  } else {
    if (whiteInit) {
      cb(whiteInit->mat_.clone(), true);
    }
    if (blackInit) {
      cv::rotate(blackInit->mat_, img, cv::ROTATE_180);
      cb(img, true);
    }
    for (auto const &entry : whiteLast) {
      cb(entry.mat_.clone(), true);
    }
    for (auto const &entry : blackLast) {
      cv::rotate(entry.mat_, img, cv::ROTATE_180);
      cb(img, true);
    }
  }
}

void PieceBook::Entry::push(PieceBook::Image const &img, Color color) {
  if (color == Color::Black) {
    if (!blackInit) {
      blackInit = img;
      return;
    }
    if (blackLast.size() >= kMaxLastImageCount) {
      blackLast.pop_front();
    }
    blackLast.push_back(img);
  } else {
    PieceBook::Image tmp;
    tmp.cut = img.cut;
    cv::rotate(img.mat_, tmp.mat_, cv::ROTATE_180);
    if (!whiteInit) {
      whiteInit = tmp;
      return;
    }
    if (whiteLast.size() >= kMaxLastImageCount) {
      whiteLast.pop_front();
    }
    whiteLast.push_back(tmp);
  }
}

void PieceBook::each(Color color, std::function<void(Piece, cv::Mat const &, bool)> cb) const {
  for (auto const &it : store) {
    PieceUnderlyingType piece = it.first;
    it.second.each(color, [&cb, piece, color](cv::Mat const &img, bool) {
      cb(static_cast<PieceUnderlyingType>(piece) | static_cast<PieceUnderlyingType>(color), img, true);
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
      auto roi = Img::PieceROI(board, x, y);
      PieceUnderlyingType p = RemoveColorFromPiece(piece);
      Color color = ColorFromPiece(piece);
      PieceBook::Image tmp;
      tmp.cut = false;
      if (color == Color::White) {
        cv::rotate(roi, tmp.mat_, cv::ROTATE_180);
      } else {
        tmp.mat_ = roi.clone();
      }
      cv::adaptiveThreshold(tmp.mat_, tmp.mat_, 255, cv::THRESH_BINARY, cv::ADAPTIVE_THRESH_GAUSSIAN_C, 5, 0);
      store[p].push(tmp, color);
    }
  }
}

std::string PieceBook::toPng() const {
  int rows = (int)store.size();
  int w = 0;
  int h = 0;
  std::map<Piece, int> count;
  each(Color::Black, [&](Piece piece, cv::Mat const &img, bool) {
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
  cv::Mat all(cv::Size(width, height), CV_8UC3, cv::Scalar(255, 255, 255, 255));
  int row = 0;
  for (auto const &it : store) {
    int column = 0;
    it.second.each(Color::Black, [&](cv::Mat const &img, bool) {
      int x = column * w;
      int y = row * h;
      Img::Bitblt(img, all, x, y);
      column++;
    });
    row++;
  }
  return Img::EncodeToPng(all);
}

} // namespace sci
