#include <shogi_camera_input/shogi_camera_input.hpp>

#include <iostream>

namespace sci {

void Move::decideSuffix(Position const &p) {
  using namespace std;
  // this->to に効いている自軍の this->piece の一覧.
  vector<Square> candidates;
  Piece search = promote == 1 ? RemoveStatusFromPiece(piece) : piece;
  for (int y = 0; y < 9; y++) {
    for (int x = 0; x < 9; x++) {
      if (p.pieces[x][y] != search) {
        continue;
      }
      Square sq = MakeSquare(x, y);
      if (CanMove(p, sq, to)) {
        candidates.push_back(sq);
      }
    }
  }
  if (candidates.size() < 2) {
    suffix = static_cast<SuffixUnderlyingType>(SuffixType::None);
  } else if (!from) {
    // candidates.size によらず, drop を指定するだけで手を特定できる
    suffix = static_cast<SuffixUnderlyingType>(SuffixType::Drop);
  } else {
    SquareSet up;
    SquareSet down;
    SquareSet sideway;
    // piece 以外の candiate が piece に対して左右どちらに居るか.
    SquareSet left;
    SquareSet right;
    for (auto const &candidate : candidates) {
      if (candidate.rank > to.rank) {
        if (color == Color::Black) {
          up.insert(candidate);
        } else {
          down.insert(candidate);
        }
      } else if (candidate.rank < to.rank) {
        if (color == Color::Black) {
          down.insert(candidate);
        } else {
          up.insert(candidate);
        }
      }
      if (candidate.file != to.file && candidate.rank == to.rank) {
        sideway.insert(candidate);
      }
      if (candidate.file != from->file || candidate.rank != from->rank) {
        int dx = candidate.file - from->file;
        if (color == Color::White) {
          dx = -dx;
        }
        if (dx > 0) {
          right.insert(candidate);
        } else {
          left.insert(candidate);
        }
      }
    }
    // "直" 判定の時の dy
    int nearestDy = color == Color::Black ? -1 : 1;

    size_t const numUp = up.count(*from);
    size_t const numDown = down.count(*from);
    size_t const numSideway = sideway.count(*from);
    if (numUp == 1 && up.size() == 1) {
      suffix = static_cast<SuffixUnderlyingType>(SuffixType::Up);
      return;
    } else if (numDown == 1 && down.size() == 1) {
      suffix = static_cast<SuffixUnderlyingType>(SuffixType::Down);
      return;
    } else if (numSideway == 1 && sideway.size() == 1) {
      suffix = static_cast<SuffixUnderlyingType>(SuffixType::Sideway);
      return;
    } else if (from->file == to.file && from->rank + nearestDy == to.rank && search != MakePiece(color, PieceType::Rook, PieceStatus::Promoted) && search != MakePiece(color, PieceType::Bishop, PieceStatus::Promoted)) {
      // "直ぐ", と表現できるならそのようにする. ただし竜と馬には "直ぐ" は使わない.
      suffix = static_cast<SuffixUnderlyingType>(SuffixType::Nearest);
      return;
    } else if (numUp == 1) {
      if (right.size() == 0) {
        suffix = static_cast<SuffixUnderlyingType>(SuffixType::Right);
        return;
      } else if (left.size() == 0) {
        suffix = static_cast<SuffixUnderlyingType>(SuffixType::Left);
        return;
      }
      for (auto const &candidate : candidates) {
        if (up.count(candidate) == 0) {
          right.erase(candidate);
          left.erase(candidate);
        }
      }
      if (right.size() == 0) {
        suffix = static_cast<SuffixUnderlyingType>(SuffixType::Right) | static_cast<SuffixUnderlyingType>(SuffixType::Up);
        return;
      } else if (left.size() == 0) {
        suffix = static_cast<SuffixUnderlyingType>(SuffixType::Left) | static_cast<SuffixUnderlyingType>(SuffixType::Up);
        return;
      }
    } else if (numDown == 1) {
      if (left.size() == 0) {
        suffix = static_cast<SuffixUnderlyingType>(SuffixType::Left);
        return;
      } else if (right.size() == 0) {
        suffix = static_cast<SuffixUnderlyingType>(SuffixType::Right);
        return;
      }
      for (auto const &candidate : candidates) {
        if (down.count(candidate) == 0) {
          right.erase(candidate);
          left.erase(candidate);
        }
      }
      if (right.size() == 0) {
        suffix = static_cast<SuffixUnderlyingType>(SuffixType::Right) | static_cast<SuffixUnderlyingType>(SuffixType::Down);
        return;
      } else if (left.size() == 0) {
        suffix = static_cast<SuffixUnderlyingType>(SuffixType::Left) | static_cast<SuffixUnderlyingType>(SuffixType::Down);
        return;
      }
    } else if (numSideway == 1) {
      if (left.size() == 0) {
        suffix = static_cast<SuffixUnderlyingType>(SuffixType::Left);
        return;
      } else if (right.size() == 0) {
        suffix = static_cast<SuffixUnderlyingType>(SuffixType::Right);
        return;
      }
      for (auto const &candidate : candidates) {
        if (sideway.count(candidate) == 0) {
          right.erase(candidate);
          left.erase(candidate);
        }
      }
      if (right.size() == 0) {
        suffix = static_cast<SuffixUnderlyingType>(SuffixType::Right) | static_cast<SuffixUnderlyingType>(SuffixType::Sideway);
        return;
      } else if (left.size() == 0) {
        suffix = static_cast<SuffixUnderlyingType>(SuffixType::Left) | static_cast<SuffixUnderlyingType>(SuffixType::Sideway);
        return;
      }
    }
  }
}

bool Move::CanMove(Position const &position, Square from, Square to) {
  using namespace std;
  Piece pieceFrom = position.pieces[from.file][from.rank];
  if (pieceFrom == 0) {
    return false;
  }
  Color color = ColorFromPiece(pieceFrom);
  Piece pieceTo = position.pieces[to.file][to.rank];
  if (pieceTo != 0 && ColorFromPiece(pieceTo) == color) {
    return false;
  }

  struct Step {
    Step(int x, int y, int count = 1) : x(x), y(y), count(count) {}
    int x;
    int y;
    int count;
  };
  static map<Piece, vector<Step>> const sSteps = {
      {MakePiece(Color::Black, PieceType::King), {Step{-1, -1}, Step{0, -1}, Step{1, -1}, Step{-1, 0}, Step{1, 0}, Step{-1, 1}, Step{0, 1}, Step{1, 1}}},
      {MakePiece(Color::Black, PieceType::Rook), {Step{0, -1, 8}, Step{1, 0, 8}, Step{0, 1, 8}, Step{-1, 0, 8}}},
      {MakePiece(Color::Black, PieceType::Bishop), {Step{-1, -1, 8}, Step{1, -1, 8}, Step{1, 1, 8}, Step{-1, 1, 8}}},
      {MakePiece(Color::Black, PieceType::Gold), {Step{-1, -1}, Step{0, -1}, Step{1, -1}, Step{-1, 0}, Step{1, 0}, Step{0, 1}}},
      {MakePiece(Color::Black, PieceType::Silver), {Step{-1, -1}, Step{0, -1}, Step{1, -1}, Step{-1, 1}, Step{1, 1}}},
      {MakePiece(Color::Black, PieceType::Knight), {Step{-1, -2}, Step{1, -2}}},
      {MakePiece(Color::Black, PieceType::Lance), {Step{0, -1, 8}}},
      {MakePiece(Color::Black, PieceType::Pawn), {Step{0, -1}}},
      {MakePiece(Color::Black, PieceType::Rook, PieceStatus::Promoted), {Step{0, -1, 8}, Step{1, 0, 8}, Step{0, 1, 8}, Step{-1, 0, 8}, Step{-1, -1}, Step{1, -1}, Step{-1, 1}, Step{1, 1}}},
      {MakePiece(Color::Black, PieceType::Bishop, PieceStatus::Promoted), {Step{-1, -1, 8}, Step{1, -1, 8}, Step{1, 1, 8}, Step{-1, 1, 8}, Step{0, -1}, Step{-1, 0}, Step{1, 0}, Step{0, 1}}},
      {MakePiece(Color::Black, PieceType::Silver, PieceStatus::Promoted), {Step{-1, -1}, Step{0, -1}, Step{1, -1}, Step{-1, 0}, Step{1, 0}, Step{0, 1}}},
      {MakePiece(Color::Black, PieceType::Knight, PieceStatus::Promoted), {Step{-1, -1}, Step{0, -1}, Step{1, -1}, Step{-1, 0}, Step{1, 0}, Step{0, 1}}},
      {MakePiece(Color::Black, PieceType::Lance, PieceStatus::Promoted), {Step{-1, -1}, Step{0, -1}, Step{1, -1}, Step{-1, 0}, Step{1, 0}, Step{0, 1}}},
      {MakePiece(Color::Black, PieceType::Pawn, PieceStatus::Promoted), {Step{-1, -1}, Step{0, -1}, Step{1, -1}, Step{-1, 0}, Step{1, 0}, Step{0, 1}}},

      {MakePiece(Color::White, PieceType::King), {Step{-1, -1}, Step{0, -1}, Step{1, -1}, Step{-1, 0}, Step{1, 0}, Step{-1, 1}, Step{0, 1}, Step{1, 1}}},
      {MakePiece(Color::White, PieceType::Rook), {Step{0, -1, 8}, Step{1, 0, 8}, Step{0, 1, 8}, Step{-1, 0, 8}}},
      {MakePiece(Color::White, PieceType::Bishop), {Step{-1, -1, 8}, Step{1, -1, 8}, Step{1, 1, 8}, Step{-1, 1, 8}}},
      {MakePiece(Color::White, PieceType::Gold), {Step{1, 1}, Step{0, 1}, Step{-1, 1}, Step{1, 0}, Step{-1, 0}, Step{0, -1}}},
      {MakePiece(Color::White, PieceType::Silver), {Step{1, 1}, Step{0, 1}, Step{-1, 1}, Step{1, -1}, Step{-1, -1}}},
      {MakePiece(Color::White, PieceType::Knight), {Step{1, 2}, Step{-1, 2}}},
      {MakePiece(Color::White, PieceType::Lance), {Step{0, 1, 8}}},
      {MakePiece(Color::White, PieceType::Pawn), {Step{0, 1}}},
      {MakePiece(Color::White, PieceType::Rook, PieceStatus::Promoted), {Step{0, 1, 8}, Step{-1, 0, 8}, Step{0, -1, 8}, Step{1, 0, 8}, Step{1, 1}, Step{-1, 1}, Step{1, -1}, Step{-1, -1}}},
      {MakePiece(Color::White, PieceType::Bishop, PieceStatus::Promoted), {Step{1, 1, 8}, Step{-1, 1, 8}, Step{-1, -1, 8}, Step{1, -1, 8}, Step{0, 1}, Step{1, 0}, Step{-1, 0}, Step{0, -1}}},
      {MakePiece(Color::White, PieceType::Silver, PieceStatus::Promoted), {Step{1, 1}, Step{0, 1}, Step{-1, 1}, Step{1, 0}, Step{-1, 0}, Step{0, -1}}},
      {MakePiece(Color::White, PieceType::Knight, PieceStatus::Promoted), {Step{1, 1}, Step{0, 1}, Step{-1, 1}, Step{1, 0}, Step{-1, 0}, Step{0, -1}}},
      {MakePiece(Color::White, PieceType::Lance, PieceStatus::Promoted), {Step{1, 1}, Step{0, 1}, Step{-1, 1}, Step{1, 0}, Step{-1, 0}, Step{0, -1}}},
      {MakePiece(Color::White, PieceType::Pawn, PieceStatus::Promoted), {Step{1, 1}, Step{0, 1}, Step{-1, 1}, Step{1, 0}, Step{-1, 0}, Step{0, -1}}},
  };
  auto found = sSteps.find(pieceFrom);
  if (found == sSteps.end()) {
    return false;
  }
  vector<Step> const &steps = found->second;
  for (Step const &step : steps) {
    for (int i = 1; i <= step.count; i++) {
      int x = from.file + step.x * i;
      int y = from.rank + step.y * i;
      if (x < 0 || 9 <= x || y < 0 || 9 <= y) {
        break;
      }
      Piece t = position.pieces[x][y];
      if (t == 0) {
        if (x == to.file && y == to.rank) {
          return true;
        }
        continue;
      } else if (ColorFromPiece(t) == color) {
        break;
      } else if (x == to.file && y == to.rank) {
        return true;
      } else {
        break;
      }
    }
  }
  return false;
}

} // namespace sci
