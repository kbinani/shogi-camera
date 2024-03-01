#include "core/record/Record.h"
#include "searcher/Searcher.h"

#include <shogi_camera/shogi_camera.hpp>

#include <thread>

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
    if (move.captured().isPromoted()) {
      mv.captured = static_cast<PieceUnderlyingType>(*captured) | static_cast<PieceUnderlyingType>(PieceStatus::Promoted);
    } else {
      mv.captured = static_cast<PieceUnderlyingType>(*captured);
    }
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
  if (auto hand = move.captured; hand) {
    auto kind = SunfishPieceKindFromPieceType(PieceTypeFromPiece(*hand));
    if (IsPromotedPiece(*hand)) {
      mv.setCaptured(sunfish::Piece(kind | sunfish::Piece::Promotion));
    } else {
      mv.setCaptured(sunfish::Piece(kind));
    }
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

    sunfish::Searcher::Config config = searcher.getConfig();
    config.maxDepth = 2;
    config.limitSeconds = 3;
    searcher.setConfig(config);
  }

  static void RunTests() {
    Test2();
  }

  static void Test2() {
    u8string kif = u8R"(▲２六歩(27)
▽７二飛(82)
▲２五歩(26)
▽４二飛(72)
▲２四歩(25)
▽同歩(23)
▲同飛(28)
▽３二金(41)
▲２三歩打
▽同金(32)
▲同飛成(24)
▽６二玉(51)
▲２八龍(23)
▽５二金(61)
▲７六歩(77)
▽３二飛(42)
▲６八銀(79)
▽７二玉(62)
▲７七銀(68)
▽３四歩(33)
▲４八銀(39)
▽６四歩(63)
▲６六歩(67)
▽３五歩(34)
▲４六歩(47)
▽３六歩(35)
▲同歩(37)
▽同飛(32)
▲３七銀(48)
▽３二飛(36)
▲７八金(69)
▽３四歩打
▲９六歩(97)
▽５一金(52)
▲９七角(88)
▽６二飛(32)
▲２三歩打
▽４四角(22)
▲４五歩(46)
▽３五角(44)
▲５八金(49)
▽６一金(51)
▲６九玉(59)
▽２二歩打
▲同歩成(23)
▽同銀(31)
▲３六歩打
▽２七歩打
▲同龍(28)
▽２六歩打
▲２八龍(27)
▽２四角(35)
▲１六歩(17)
▽４二角(24)
▲２六銀(37)
▽２四角(42)
▲３七銀(26)
▽２三銀(22)
▲２五歩打
▽５一角(24)
▲４六銀(37)
▽８四歩(83)
▲３五歩(36)
▽同歩(34)
▲同銀(46)
▽６三玉(72)
▲１五歩(16)
▽１二香(11)
▲２四歩(25)
▽２七歩打
▲同龍(28)
▽３二銀(23)
▲２三歩不成(24)
▽３三銀(32)
▲３四歩打
▽４二銀(33)
▲２二歩不成(23)
▽１四歩(13)
▲２一歩成(22)
▽１五歩(14)
▲２三龍(27)
▽２八歩打
▲同龍(23)
▽９二飛(62)
▲２三龍(28)
▽６二金(61)
▲１二龍(23)
▽６五歩(64)
▲８三香打
▽７二玉(63)
▲６五歩(66)
▽８三玉(72)
▲３二龍(12)
▽６三香打
▲６四歩(65)
▽同香(63)
▲同角(97)
▽６三歩打
▲４六角(64)
▽７四歩(73)
▲３一と金(21)
▽同銀(42)
▲同龍(32)
▽６一金(62)
▲３三歩不成(34)
▽３八歩打
▲２三歩打
▽３九歩成(38)
▲３七桂(29)
▽７三桂(81)
▲３四銀(35)
▽６五桂(73)
▲６六銀(77)
▽５七桂成(65)
▲同銀(66)
▽３六歩打
▲２五桂(37)
▽３七歩成(36)
▲同角(46)
▽３八と金(39)
▲４六角(37)
▽７五歩(74)
▲４三銀成(34)
▽７六歩(75)
▲３二歩成(33)
▽２六歩打
▲４二と金(32)
▽７七歩成(76)
▲同桂(89)
▽４二飛(92)
▲同成銀(43)
▽同角(51)
▲同龍(31)
▽６四歩(63)
▲５三龍(42)
▽７三歩打
▲９五歩(96)
▽２七歩成(26)
▲５六角打
▽７二玉(83)
▲８三銀打
▽８一玉(72)
▲４四歩(45)
▽３六銀打
▲６四角(46)
▽２五銀(36)
▲７三角成(64)
▽８五桂打
▲６三馬(73)
)";
    u8string::size_type offset = 0;
    optional<Square> last;
    Position p = MakePosition(Handicap::None);
    deque<PieceType> handBlack;
    deque<PieceType> handWhite;

    sunfish::Record record;
    record.init(sunfish::Board::Handicap::Even);

    sunfish::Searcher searcher;
    sunfish::Searcher::Config config = searcher.getConfig();
    config.maxDepth = 2;
    config.limitSeconds = 3;
    searcher.setConfig(config);

    vector<sunfish::Move> smoves;

    while (offset != u8string::npos) {
      auto found = kif.find(u8"\n", offset);
      if (found == string::npos) {
        break;
      }
      auto line = kif.substr(offset, found - offset);
      auto backup = line;
      Color color;
      if (line.starts_with(u8"▲")) {
        color = Color::Black;
        line = line.substr(u8string(u8"▲").size());
      } else if (line.starts_with(u8"▽")) {
        color = Color::White;
        line = line.substr(u8string(u8"▽").size());
      } else {
        cout << "手番が不明" << endl;
        break;
      }
      Square to;
      if (line.starts_with(u8"同")) {
        if (!last) {
          cout << "初手なのに'同'" << endl;
          break;
        }
        to = *last;
        line = line.substr(u8string(u8"同").size());
      } else {
        auto read = TrimSquarePartFromString(line);
        if (!read) {
          cout << "マス目を読み取れない" << endl;
          break;
        } else {
          to = *read;
        }
      }
      auto pieceTypeAndStatus = TrimPieceTypeAndStatusPartFromString(line);
      if (!pieceTypeAndStatus) {
        cout << "駒の種類を読み取れない" << endl;
        break;
      }
      Move mv;
      mv.color = color;
      mv.to = to;
      bool fromHand = false;
      if (line.starts_with(u8"打")) {
        line = line.substr(u8string(u8"打").size());
        mv.from = nullopt;
        fromHand = true;
      }
      if (line.starts_with(u8"成")) {
        line = line.substr(u8string(u8"成").size());
        mv.promote = 1;
      } else if (line.starts_with(u8"不成")) {
        line = line.substr(u8string(u8"不成").size());
        mv.promote = -1;
      } else {
        mv.promote = 0;
      }
      mv.piece = MakePiece(color, static_cast<PieceType>(RemoveStatusFromPiece(*pieceTypeAndStatus)), IsPromotedPiece(*pieceTypeAndStatus) ? PieceStatus::Promoted : PieceStatus::Default);
      auto captured = p.pieces[mv.to.file][mv.to.rank];
      if (captured != 0) {
        mv.captured = RemoveColorFromPiece(captured);
      }
      if (!fromHand) {
        auto index = line.find(u8"(");
        if (index != u8string::npos) {
          auto fromStr = line.substr(index + 1);
          auto from = TrimSquarePartFromString(fromStr);
          if (from) {
            mv.from = *from;
          } else {
            cout << "TODO: 末尾に()書きで移動元が書いてない場合もparseできるようにする" << endl;
            break;
          }
        }
      }
      mv.decideSuffix(p);
      p.apply(mv, handBlack, handWhite);
      cout << (char const *)backup.c_str() << " => " << (char const *)StringFromMoveWithOptionalLast(mv, last).c_str() << endl;
      auto smove = SunfishMoveFromMove(mv);
      if (!smove) {
        cout << "sunfish 形式に変換できない" << endl;
        break;
      }
      smoves.push_back(*smove);
      if (!record.makeMove(*smove)) {
        cout << "makeMove が失敗" << endl;
        break;
      }

      searcher.setRecord(record);
      sunfish::Move move = sunfish::Move::empty();
      if (!searcher.idsearch(record.getBoard(), move)) {
        cout << "idsearchが失敗" << endl;
        break;
      }
      searcher.clearRecord();
      last = to;
      offset = found + 1;
    }
  }

  optional<Move> next(Position const &p, vector<Move> const &moves, deque<PieceType> const &hand, deque<PieceType> const &handEnemy) {
    Color color = moves.size() % 2 == 0 ? Color::Black : Color::White;

    sunfish::Record record;
    record.init(sunfish::Board::Handicap::Even);
    for (auto const &m : moves) {
      auto sm = SunfishMoveFromMove(m);
      if (!sm) {
        cout << "move を sunfish 形式に変換できなかった" << endl;
        return random(p, color, hand, handEnemy);
      }
      cout << sm->toString() << endl;
      if (!record.makeMove(*sm)) {
        cout << "move を record に適用できなかった" << endl;
        return random(p, color, hand, handEnemy);
      }
    }
    searcher.forceInterrupt();
    unique_lock<mutex> lock(mut);
    bool stopped = searcher.wait(lock, chrono::duration<double>(10), [&]() {
      return !searcher.isRunning();
    });
    lock.unlock();
    if (!stopped) {
      cout << "searcher が停止しなかった" << endl;
      return random(p, color, hand, handEnemy);
    }
    searcher.setRecord(record);
    sunfish::Move move = sunfish::Move::empty();
    if (!searcher.idsearch(record.getBoard(), move)) {
      searcher.clearRecord();

      // idsearch が失敗したら有効手の中からランダムな手を選ぶ.
      cout << "idsearch が false を返した" << endl;
      return random(p, color, hand, handEnemy);
    }
    searcher.clearRecord();
    if (auto m = MoveFromSunfishMove(move, color); m) {
      return m;
    } else {
      return random(p, color, hand, handEnemy);
    }
  }

  optional<Move> random(Position const &p, Color color, deque<PieceType> const &hand, deque<PieceType> const &handEnemy) {
    deque<Move> candidates;
    Game::Generate(p, color, color == Color::Black ? hand : handEnemy, color == Color::Black ? handEnemy : hand, candidates, true);
    if (candidates.empty()) {
      return nullopt;
    } else if (candidates.size() == 1) {
      return candidates[0];
    } else {
      uniform_int_distribution<int> dist(0, (int)candidates.size() - 1);
      int index = dist(*engine);
      return candidates[index];
    }
  }

  sunfish::Searcher searcher;
  unique_ptr<mt19937_64> engine;
  std::mutex mut;
};

Sunfish3AI::Sunfish3AI() : impl(make_unique<Impl>()) {
}

Sunfish3AI::~Sunfish3AI() {}

optional<Move> Sunfish3AI::next(Position const &p, vector<Move> const &moves, deque<PieceType> const &hand, deque<PieceType> const &handEnemy) {
  return impl->next(p, moves, hand, handEnemy);
}

void Sunfish3AI::RunTests() {
  Impl::RunTests();
}

} // namespace sci
