#include "core/record/Record.h"
#include "searcher/Searcher.h"

#include <shogi_camera_input/shogi_camera_input.hpp>

using namespace std;

namespace sci {

namespace {

optional<PieceType> PieceTypeFromSunfishPiece(sunfish::Piece p) {
  switch (sunfish::Piece::HandMask & static_cast<uint8_t>(p.kindOnly())) {
  case sunfish::Piece::Pawn:
    return PieceType::Pawn;
  case sunfish::Piece::Lance:
    return PieceType::Lance;
  case sunfish::Piece::Knight:
    return PieceType::Knight;
  case sunfish::Piece::Silver:
    return PieceType::Silver;
  case sunfish::Piece::Gold:
    return PieceType::Gold;
  case sunfish::Piece::Bishop:
    return PieceType::Bishop;
  case sunfish::Piece::Rook:
    return PieceType::Rook;
  case sunfish::Piece::King:
    return PieceType::King;
  default:
    return nullopt;
  }
}

Square SquareFromSunfishSquare(sunfish::Square sq) {
  return MakeSquare(9 - sq.getFile(), sq.getRank() - 1);
}

optional<Move> MoveFromSunfishMove(sunfish::Move const &move, Color color) {
  Move mv;
  mv.color = color;
  if (!move.isHand()) {
    mv.from = SquareFromSunfishSquare(move.from());
  }
  mv.to = SquareFromSunfishSquare(move.to());
  sunfish::Piece piece = move.piece();
  auto type = PieceTypeFromSunfishPiece(move.piece());
  if (!type) {
    cout << "PieceType を特定できなかった" << endl;
    return nullopt;
  }
  mv.piece = MakePiece(color, *type, piece.isPromoted() ? PieceStatus::Promoted : PieceStatus::Default);
  if (move.isCapturing()) {
    auto captured = PieceTypeFromSunfishPiece(move.captured());
    if (!captured) {
      cout << "取った駒の PieceType を特定できなかった" << endl;
      return nullopt;
    }
    mv.newHand = captured;
  }
  if (move.promote()) {
    mv.promote = 1;
  } else {
    if (mv.from && CanPromote(mv.piece) && !piece.isPromoted() && IsPromotableMove(*mv.from, mv.to, color)) {
      mv.promote = -1;
    }
  }
  return mv;
}

sunfish::Square SunfishSquareFromSquare(Square sq) {
  return sunfish::Square(9 - sq.file, sq.rank + 1);
}

uint8_t SunfishPieceKindFromPieceType(PieceType type) {
  switch (type) {
  case PieceType::Pawn:
    return sunfish::Piece::Pawn;
  case PieceType::Lance:
    return sunfish::Piece::Lance;
  case PieceType::Knight:
    return sunfish::Piece::Knight;
  case PieceType::Silver:
    return sunfish::Piece::Silver;
  case PieceType::Gold:
    return sunfish::Piece::Gold;
  case PieceType::Bishop:
    return sunfish::Piece::Bishop;
  case PieceType::Rook:
    return sunfish::Piece::Rook;
  case PieceType::King:
    return sunfish::Piece::King;
  case PieceType::Empty:
  default:
    return sunfish::Piece::Empty;
  }
}

sunfish::Piece SunfishPieceFromPiece(Piece p) {
  uint8_t index = 0;
  if (ColorFromPiece(p) == Color::White) {
    index |= sunfish::Piece::White;
  }
  index |= SunfishPieceKindFromPieceType(PieceTypeFromPiece(p));
  if (IsPromotedPiece(p)) {
    index |= sunfish::Piece::Promotion;
  }
  return Piece(index);
}

optional<sunfish::Move> SunfishMoveFromMove(Move const &move) {
  sunfish::Square from;
  sunfish::Piece piece = SunfishPieceFromPiece(move.piece);
  if (move.from) {
    from = SunfishSquareFromSquare(*move.from);
  } else {
    from = sunfish::Square::Invalid;
  }
  sunfish::Square to = SunfishSquareFromSquare(move.to);
  bool promote = move.promote == 1;
  sunfish::Move mv(piece, from, to, promote);
  if (auto hand = move.newHand; hand) {
    auto kind = SunfishPieceKindFromPieceType(*hand);
    mv.setCaptured(sunfish::Piece(kind));
  }
  if (mv.isEmpty()) {
    return nullopt;
  } else {
    return mv;
  }
}

} // namespace

struct Sunfish3AI::Impl {
  optional<Move> next(Position const &p, vector<Move> const &moves, deque<PieceType> const &hand, deque<PieceType> const &handEnemy) {
    Color color = moves.size() % 2 == 0 ? Color::Black : Color::White;
    sunfish::Move move = sunfish::Move::empty();
    sunfish::Record record;
    record.init(sunfish::Board::Handicap::Even);
    for (auto const &m : moves) {
      auto sm = SunfishMoveFromMove(m);
      if (!sm) {
        cout << "move を sunfish 形式に変換できなかった" << endl;
        return nullopt;
      }
      cout << sm->toString() << endl;
      if (!record.makeMove(*sm)) {
        cout << "move を record に適用できなかった" << endl;
        return nullopt;
      }
    }
    searcher.setRecord(record);
    if (!searcher.idsearch(record.getBoard(), move)) {
      cout << "idsearch が false を返した" << endl;
      return nullopt;
    }
    cout << "result: " << move.toString() << endl;
    searcher.clearRecord();
    cout << searcher.getInfoString() << endl;
    return MoveFromSunfishMove(move, color);
  }

  sunfish::Searcher searcher;
};

Sunfish3AI::Sunfish3AI() : impl(make_unique<Impl>()) {
}

Sunfish3AI::~Sunfish3AI() {}

optional<Move> Sunfish3AI::next(Position const &p, vector<Move> const &moves, deque<PieceType> const &hand, deque<PieceType> const &handEnemy) {
  return impl->next(p, moves, hand, handEnemy);
}

} // namespace sci
