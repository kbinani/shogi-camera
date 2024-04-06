#include <shogi_camera/shogi_camera.hpp>

#include <iostream>

using namespace std;

namespace sci {

bool Position::isInCheck(Color color) const {
  Piece search = MakePiece(color, PieceType::King);
  optional<Square> king;
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

bool Position::apply(Move const &mv, deque<PieceType> &handBlack, deque<PieceType> &handWhite) {
  auto to = pieces[mv.to.file][mv.to.rank];
  if (mv.captured) {
    if (to == 0) {
      // 駒が居ないマスの駒を取ろうとしている
      return false;
    }
    if (ColorFromPiece(to) == mv.color) {
      // 自軍の駒を取っている
      return false;
    }
    if (RemoveColorFromPiece(to) != *mv.captured) {
      // 取った駒と実際に居る駒が矛盾している
      return false;
    }
  } else {
    if (to != 0) {
      // 駒を取ったわけじゃないのに駒のいるマスに進んでいる
      return false;
    }
  }
  auto &hand = mv.color == Color::Black ? handBlack : handWhite;
  if (mv.from) {
    pieces[mv.from->file][mv.from->rank] = 0;
  } else {
    if (pieces[mv.to.file][mv.to.rank] != 0) {
      // 既に駒の居るマスに打った
      return false;
    }
    if (IsPromotedPiece(mv.piece)) {
      // 成駒を打った
      return false;
    }
    PieceType type = PieceTypeFromPiece(mv.piece);
    if (type == PieceType::King) {
      // 玉は持ち駒にできない
      return false;
    }
    bool ok = false;
    for (auto it = hand.begin(); it != hand.end(); it++) {
      if (*it == type) {
        hand.erase(it);
        ok = true;
        break;
      }
    }
    if (!ok) {
      // 持ち駒に無い駒を打った
      return false;
    }
    Rank minRank = Rank::Rank1;
    Rank maxRank = Rank::Rank9;
    switch (type) {
    case PieceType::Pawn:
    case PieceType::Lance:
      if (mv.color == Color::Black) {
        minRank = Rank::Rank2;
      } else {
        maxRank = Rank::Rank8;
      }
      break;
    case PieceType::Knight:
      if (mv.color == Color::Black) {
        minRank = Rank::Rank3;
      } else {
        maxRank = Rank::Rank7;
      }
      break;
    default:
      break;
    }
    if (mv.to.rank < minRank || maxRank < mv.to.rank) {
      // 反則となる駒打ち
      return false;
    }
    if (type == PieceType::Pawn) {
      for (int y = 0; y < 9; y++) {
        if (pieces[mv.to.file][y] == MakePiece(mv.color, PieceType::Pawn)) {
          // 二歩
          return false;
        }
      }
    }
  }
  if (mv.promote == 1 && !IsPromotedPiece(mv.piece)) {
    pieces[mv.to.file][mv.to.rank] = Promote(mv.piece);
  } else {
    pieces[mv.to.file][mv.to.rank] = mv.piece;
  }
  if (mv.captured) {
    hand.push_back(PieceTypeFromPiece(Unpromote(*mv.captured)));
  }
  if (isInCheck(mv.color)) {
    // 王手放置
    return false;
  }
  return true;
}

u8string Position::debugString() const {
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
