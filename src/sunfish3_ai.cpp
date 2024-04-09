#if SHOGI_CAMERA_ENABLE_SUNFISH
#include "core/record/Record.h"
#include "searcher/Searcher.h"
#endif

#include <shogi_camera/shogi_camera.hpp>

#include <thread>

using namespace std;

namespace sci {

#if SHOGI_CAMERA_ENABLE_SUNFISH
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

} // namespace

struct Sunfish3AI::Impl {
  Impl() {
    random_device seed_gen;
    engine = make_unique<mt19937_64>(seed_gen());

    sunfish::Searcher::Config config = searcher.getConfig();
    config.limitSeconds = kLimitSeconds;
    config.maxDepth = kMaxDepth;
    searcher.setConfig(config);
  }

  ~Impl() {
    searcher.forceInterrupt();
    unique_lock<mutex> lock(mut);
    searcher.wait(lock, chrono::duration<double>::max(), [&]() { return !searcher.isRunning(); });
    lock.unlock();
  }

  optional<Move> next(Position const &p, Color color, deque<Move> const &, deque<PieceType> const &hand, deque<PieceType> const &handEnemy) {
    sunfish::Record record;
    record.init(
        SunfishBoardFromPositionAndHand(
            p,
            color == Color::Black ? hand : handEnemy,
            color == Color::Black ? handEnemy : hand,
            color));
    searcher.forceInterrupt();
    unique_lock<mutex> lock(mut);
    bool stopped = searcher.wait(lock, chrono::duration<double>(kLimitSeconds + 1), [&]() {
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

  static constexpr float kLimitSeconds = 60;
  static constexpr int kMaxDepth = 64;
};
#else
struct Sunfish3AI::Impl {
  Impl() {}

  optional<Move> next(Position const &p, Color next, deque<Move> const &moves, deque<PieceType> const &hand, deque<PieceType> const &handEnemy) {
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

optional<Move> Sunfish3AI::next(Position const &p, Color next, deque<Move> const &moves, deque<PieceType> const &hand, deque<PieceType> const &handEnemy) {
  return impl->next(p, next, moves, hand, handEnemy);
}

void Sunfish3AI::stop() {
  impl->stop();
}

} // namespace sci
