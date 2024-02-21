#include <shogi_camera_input/shogi_camera_input.hpp>

namespace sci {

void PieceBook::Entry::each(Color color, std::function<void(cv::Mat const &)> cb) const {
  cv::Mat img;
  if (color == Color::Black) {
    if (blackInit) {
      cb(blackInit->clone());
    }
    if (whiteInit) {
      cv::rotate(*whiteInit, img, cv::ROTATE_180);
      cb(img);
    }
    if (blackLast) {
      cb(blackLast->clone());
    }
    if (whiteLast) {
      cv::rotate(*whiteLast, img, cv::ROTATE_180);
      cb(img);
    }
  } else {
    if (whiteInit) {
      cb(whiteInit->clone());
    }
    if (blackInit) {
      cv::rotate(*blackInit, img, cv::ROTATE_180);
      cb(img);
    }
    if (whiteLast) {
      cb(whiteLast->clone());
    }
    if (blackLast) {
      cv::rotate(*blackLast, img, cv::ROTATE_180);
      cb(img);
    }
  }
}

void PieceBook::Entry::push(cv::Mat const &img, Color color) {
  if (color == Color::Black) {
    if (!blackInit) {
      blackInit = img;
      return;
    }
    blackLast = img;
  } else {
    cv::Mat tmp;
    cv::rotate(img, tmp, cv::ROTATE_180);
    if (!whiteInit) {
      whiteInit = tmp;
      return;
    }
    whiteLast = tmp;
  }
}

void PieceBook::each(Color color, std::function<void(Piece, cv::Mat const &)> cb) const {
  for (auto const &it : store) {
    PieceUnderlyingType piece = it.first;
    it.second.each(color, [&cb, piece, color](cv::Mat const &img) {
      cb(static_cast<PieceUnderlyingType>(piece) | static_cast<PieceUnderlyingType>(color), img);
    });
  }
}

void PieceBook::update(Position const &position, cv::Mat const &board) {
  for (int y = 0; y < 9; y++) {
    for (int x = 0; x < 9; x++) {
      Piece piece = position.pieces[x][y];
      if (piece == 0) {
        continue;
      }
      auto roi = Img::PieceROI(board, x, y).clone();
      PieceUnderlyingType p = RemoveColorFromPiece(piece);
      Color color = ColorFromPiece(piece);
      store[p].push(roi, color);
    }
  }
}

} // namespace sci
