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
  PieceType type = PieceTypeFromPiece(move.piece);
  Color color = ColorFromPiece(move.piece);
  PieceStatus status = IsPromotedPiece(move.piece) ? PieceStatus::Promoted : PieceStatus::Default;
  if (move.promote == 1) {
    status = PieceStatus::Default;
  }
  sunfish::Piece piece = SunfishPieceFromPiece(MakePiece(color, type, status));
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
  Impl() {
    random_device seed_gen;
    engine = make_unique<mt19937_64>(seed_gen());
    Test();
  }

  static void Test() {
    sunfish::Record record;
    record.init(sunfish::Board::Handicap::Even);

    static auto Make = [](Color color, optional<Square> from, Square to, Piece p, bool promote = false, optional<PieceType> newHand = nullopt) {
      Move m;
      m.color = color;
      m.from = from;
      m.to = to;
      m.piece = p;
      if (promote) {
        m.promote = 1;
      }
      m.newHand = newHand;
      return m;
    };

    vector<Move> moves = {
        // ▲７六歩(77)
        Make(Color::Black, MakeSquare(File::File7, Rank::Rank7), MakeSquare(File::File7, Rank::Rank6), MakePiece(Color::Black, PieceType::Pawn)),
        // ▽９四歩(93)
        Make(Color::White, MakeSquare(File::File9, Rank::Rank3), MakeSquare(File::File9, Rank::Rank4), MakePiece(Color::White, PieceType::Pawn)),
        // ▲９六歩(97)
        Make(Color::Black, MakeSquare(File::File9, Rank::Rank7), MakeSquare(File::File9, Rank::Rank6), MakePiece(Color::Black, PieceType::Pawn)),
        // ▽９三桂(81)
        Make(Color::White, MakeSquare(File::File8, Rank::Rank1), MakeSquare(File::File9, Rank::Rank3), MakePiece(Color::White, PieceType::Knight)),
        // ▲９七角(88)
        Make(Color::Black, MakeSquare(File::File8, Rank::Rank8), MakeSquare(File::File9, Rank::Rank7), MakePiece(Color::Black, PieceType::Bishop)),
        // ▽５二飛(82)
        Make(Color::White, MakeSquare(File::File8, Rank::Rank2), MakeSquare(File::File5, Rank::Rank2), MakePiece(Color::White, PieceType::Rook)),
        // ▲２六歩(27)
        Make(Color::Black, MakeSquare(File::File2, Rank::Rank7), MakeSquare(File::File2, Rank::Rank6), MakePiece(Color::Black, PieceType::Pawn)),
        // ▽４二玉(51)
        Make(Color::White, MakeSquare(File::File5, Rank::Rank1), MakeSquare(File::File4, Rank::Rank2), MakePiece(Color::White, PieceType::King)),
        // ▲２五歩(26)
        Make(Color::Black, MakeSquare(File::File2, Rank::Rank6), MakeSquare(File::File2, Rank::Rank5), MakePiece(Color::Black, PieceType::Pawn)),
        // ▽８二飛(52)
        Make(Color::White, MakeSquare(File::File5, Rank::Rank2), MakeSquare(File::File8, Rank::Rank2), MakePiece(Color::White, PieceType::Rook)),
        // ▲２四歩(25)
        Make(Color::Black, MakeSquare(File::File2, Rank::Rank5), MakeSquare(File::File2, Rank::Rank4), MakePiece(Color::Black, PieceType::Pawn)),
        // ▽３四歩(33)
        Make(Color::White, MakeSquare(File::File3, Rank::Rank3), MakeSquare(File::File3, Rank::Rank4), MakePiece(Color::White, PieceType::Pawn)),
        // ▲８八銀(79)
        Make(Color::Black, MakeSquare(File::File7, Rank::Rank9), MakeSquare(File::File8, Rank::Rank8), MakePiece(Color::Black, PieceType::Silver)),
        // ▽２四歩(23)
        Make(Color::White, MakeSquare(File::File2, Rank::Rank3), MakeSquare(File::File2, Rank::Rank4), MakePiece(Color::White, PieceType::Pawn), false, PieceType::Pawn),
        // ▲同飛(28)
        Make(Color::Black, MakeSquare(File::File2, Rank::Rank8), MakeSquare(File::File2, Rank::Rank4), MakePiece(Color::Black, PieceType::Rook), false, PieceType::Pawn),
        // ▽８五桂(93)
        Make(Color::White, MakeSquare(File::File9, Rank::Rank3), MakeSquare(File::File8, Rank::Rank5), MakePiece(Color::White, PieceType::Knight)),
        // ▲８六角(97)
        Make(Color::Black, MakeSquare(File::File9, Rank::Rank7), MakeSquare(File::File8, Rank::Rank6), MakePiece(Color::Black, PieceType::Bishop)),
        // ▽８八角成(22)
        Make(Color::White, MakeSquare(File::File2, Rank::Rank2), MakeSquare(File::File8, Rank::Rank8), MakePiece(Color::White, PieceType::Bishop), true, PieceType::Silver),
        // ▲２二歩打
        Make(Color::Black, nullopt, MakeSquare(File::File2, Rank::Rank2), MakePiece(Color::Black, PieceType::Pawn)),
        // ▽同馬(88)
        Make(Color::White, MakeSquare(File::File8, Rank::Rank8), MakeSquare(File::File2, Rank::Rank2), MakePiece(Color::White, PieceType::Bishop, PieceStatus::Promoted), false, PieceType::Pawn),
        // ▲３四飛(24)
        Make(Color::Black, MakeSquare(File::File2, Rank::Rank4), MakeSquare(File::File3, Rank::Rank4), MakePiece(Color::Black, PieceType::Rook), false, PieceType::Pawn),
    };
    for (int i = 0; i < moves.size(); i++) {
      Move move = moves[i];
      auto mv = SunfishMoveFromMove(move);
      if (!record.makeMove(*mv)) {
        cout << "ng" << endl;
      }
    }
  }

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
    sunfish::Searcher::Config config = searcher.getConfig();
    config.maxDepth = 2;
    config.limitSeconds = 3;
    searcher.setConfig(config);
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
