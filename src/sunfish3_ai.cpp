#if SHOGI_CAMERA_DEBUG
#include "core/record/Record.h"
#include "searcher/Searcher.h"
#endif

#include <shogi_camera/shogi_camera.hpp>

#include <thread>

using namespace std;

namespace sci {

#if SHOGI_CAMERA_DEBUG
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

  ~Impl() {
    searcher.forceInterrupt();
  }

  static void RunTests() {
    Test2();
  }

  static void Test2() {
    u8string kif = u8R"(▲６八玉(59)
△３二金(41)
▲１八香(19)
△８四歩(83)
▲３八金(49)
△８五歩(84)
▲３六歩(37)
△８六歩(85)
▲同歩(87)
△同飛(82)
▲４八金(38)
△８七歩打
▲７八金(69)
△３六飛(86)
▲３七歩打
△８六飛(36)
▲８七金(78)
△同飛成(86)
▲４九金(48)
△８二龍(87)
▲６九玉(68)
△８七歩打
▲８三歩打
△同龍(82)
▲４六歩(47)
△８八歩成(87)
▲同飛(28)
△４七角打
▲７八玉(69)
△８六歩打
▲３八銀(39)
△８七金打
▲６八玉(78)
△１四角成(47)
▲８五歩打)";
    u8string::size_type offset = 0;
    optional<Square> last;
    Position p = MakePosition(Handicap::平手);
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
      } else if (line.starts_with(u8"△")) {
        color = Color::White;
        line = line.substr(u8string(u8"△").size());
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
      bool ok = searcher.idsearch(record.getBoard(), move);
      searcher.clearRecord();
      if (!ok) {
        cout << "idsearchが失敗" << endl;
        break;
      }
      last = to;
      offset = found + 1;
    }
  }

  static sunfish::Board SunfishBoardFromPositionAndHand(Position const &position, deque<PieceType> const &handBlack, deque<PieceType> const &handWhite, Color next) {
    sunfish::CompactBoard cb;
    int index = 0;
    for (int y = 0; y < 9; y++) {
      for (int x = 0; x < 9; x++) {
        Piece p = position.pieces[x][y];
        if (p == 0) {
          continue;
        }
        sunfish::Square sq = SunfishSquareFromSquare(MakeSquare(x, y));
        sunfish::Piece sp = SunfishPieceFromPiece(p);
        uint16_t c = static_cast<uint16_t>(sp.operator uint8_t()) << sunfish::CompactBoard::PieceShift;
        uint16_t s = static_cast<uint16_t>(sq.index());
        cb.buf[index++] = c | s;
      }
    }
    for (auto const &p : handBlack) {
      sunfish::Piece sp = SunfishPieceFromPiece(MakePiece(Color::Black, p));
      uint16_t c = static_cast<uint16_t>(sp.black().index()) << sunfish::CompactBoard::PieceShift;
      cb.buf[index++] = c | sunfish::CompactBoard::Hand;
    }
    for (auto const &p : handWhite) {
      sunfish::Piece sp = SunfishPieceFromPiece(MakePiece(Color::White, p));
      uint16_t c = static_cast<uint16_t>(sp.white().index()) << sunfish::CompactBoard::PieceShift;
      cb.buf[index++] = c | sunfish::CompactBoard::Hand;
    }
    cb.buf[index] = sunfish::CompactBoard::End;
    if (next == Color::Black) {
      cb.buf[index] |= sunfish::CompactBoard::Black;
    }
    return sunfish::Board(cb);
  }

  optional<Move> next(Position const &p, Color color, vector<Move> const &, deque<PieceType> const &hand, deque<PieceType> const &handEnemy) {
    sunfish::Record record;
    record.init(
        SunfishBoardFromPositionAndHand(
            p,
            color == Color::Black ? hand : handEnemy,
            color == Color::Black ? handEnemy : hand,
            color));
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
    bool ok = searcher.idsearch(record.getBoard(), move);
    searcher.clearRecord();
    if (!ok) {
      // idsearch が失敗したら有効手の中からランダムな手を選ぶ.
      cout << "idsearch が false を返した" << endl;
      return random(p, color, hand, handEnemy);
    }
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

  void stop() {
    searcher.forceInterrupt();
  }

  sunfish::Searcher searcher;
  unique_ptr<mt19937_64> engine;
  mutex mut;
};
#else
struct Sunfish3AI::Impl {
  Impl() {}

  optional<Move> next(Position const &p, Color next, vector<Move> const &moves, deque<PieceType> const &hand, deque<PieceType> const &handEnemy) {
    return nullopt;
  }

  void stop() {
  }

  static void RunTests() {}
};
#endif

Sunfish3AI::Sunfish3AI() : impl(make_unique<Impl>()) {
}

Sunfish3AI::~Sunfish3AI() {}

optional<Move> Sunfish3AI::next(Position const &p, Color next, vector<Move> const &moves, deque<PieceType> const &hand, deque<PieceType> const &handEnemy) {
  return impl->next(p, next, moves, hand, handEnemy);
}

void Sunfish3AI::stop() {
  impl->stop();
}

void Sunfish3AI::RunTests() {
  Impl::RunTests();
}

} // namespace sci
