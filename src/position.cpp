#include <shogi_camera_input/shogi_camera_input.hpp>

#include <iostream>

namespace sci {

bool Position::isInCheck(Color color) const {
  Piece search = MakePiece(color, PieceType::King);
  std::optional<Square> king;
  for (int y = 0; y < 9; y++) {
    for (int x = 0; x < 9; x++) {
      if (search == pieces[x][y]) {
        king = MakeSquare(x, y);
        break;
      }
    }
  }
  if (!king) {
    // 玉が居なければ王手は掛からない(?).
    return false;
  }
  for (int y = 0; y < 9; y++) {
    for (int x = 0; x < 9; x++) {
      Piece p = pieces[x][y];
      if (p == 0 || ColorFromPiece(p) == color) {
        continue;
      }
      if (Move::CanMove(*this, MakeSquare(x, y), *king)) {
        return true;
      }
    }
  }
  return false;
}

void Position::apply(Move const &mv, std::deque<PieceType> &handBlack, std::deque<PieceType> &handWhite) {
  auto &hand = mv.color == Color::Black ? handBlack : handWhite;
  if (mv.from) {
    pieces[mv.from->file][mv.from->rank] = 0;
  } else {
    PieceType type = PieceTypeFromPiece(mv.piece);
    bool ok = false;
    for (auto it = hand.begin(); it != hand.end(); it++) {
      if (*it == type) {
        hand.erase(it);
        ok = true;
        break;
      }
    }
    if (!ok) {
      std::cout << "存在しない持ち駒を打った" << std::endl;
    }
  }
  if (mv.promote == 1 && !IsPromotedPiece(mv.piece)) {
    pieces[mv.to.file][mv.to.rank] = Promote(mv.piece);
  } else {
    pieces[mv.to.file][mv.to.rank] = mv.piece;
  }
  if (mv.newHand_) {
    hand.push_back(PieceTypeFromPiece(Unpromote(*mv.newHand_)));
  }
}

std::u8string Position::debugString() const {
  using namespace std;
  u8string ret;
  for (int y = 0; y < 9; y++) {
    for (int x = 0; x < 9; x++) {
      auto piece = pieces[x][y];
      if (ColorFromPiece(piece) == Color::White) {
        ret += u8"v";
      } else {
        ret += u8" ";
      }
      ret += ShortStringFromPieceTypeAndStatus(RemoveColorFromPiece(piece));
    }
    ret += u8"\n";
  }
  return ret;
}

} // namespace sci
