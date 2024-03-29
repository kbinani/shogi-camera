#pragma once

#include <hwm/task/task_queue.hpp>
#include <opencv2/core.hpp>

#include <deque>
#include <map>
#include <random>
#include <set>
#include <string>
#include <thread>

#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace sci {

using PieceUnderlyingType = uint32_t;

// 駒の種類
enum class PieceType : PieceUnderlyingType {
  Empty = 0, // 升目に駒が無いことを表す時に使う
  King = 0b0001,
  Rook = 0b0010,   // 飛車
  Bishop = 0b0011, // 角
  Gold = 0b0100,
  Silver = 0b0101,
  Knight = 0b0110,
  Lance = 0b0111,
  Pawn = 0b1000,
};

// 成駒かどうか
enum class PieceStatus : PieceUnderlyingType {
  Default = 0b00000,
  Promoted = 0b10000,
};

enum class Color : PieceUnderlyingType {
  Black = 0b000000, // 先手
  White = 0b100000, // 後手
};

inline Color OpponentColor(Color color) {
  if (color == Color::Black) {
    return Color::White;
  } else {
    return Color::Black;
  }
}

// 0 を先手の最初の手として, i 番目の手がどちらの手番か.
inline Color ColorFromIndex(size_t i) {
  return i % 2 == 0 ? Color::Black : Color::White;
}

using Piece = PieceUnderlyingType; // PieceType | PieceStatus | Color;

inline Piece MakePiece(Color color, PieceType type, PieceStatus status = PieceStatus::Default) {
  return static_cast<PieceUnderlyingType>(color) | static_cast<PieceUnderlyingType>(type) | static_cast<PieceUnderlyingType>(status);
}

inline Color ColorFromPiece(Piece p) {
  return static_cast<Color>(p & 0b100000);
}

inline PieceUnderlyingType RemoveColorFromPiece(Piece p) {
  return p & 0b11111;
}

inline Piece RemoveStatusFromPiece(Piece p) {
  return p & 0b101111;
}

inline PieceType PieceTypeFromPiece(Piece p) {
  return static_cast<PieceType>(p & 0b1111);
}

inline bool IsPromotedPiece(Piece p) {
  return (p & 0b10000) == 0b10000;
}

inline Piece Promote(Piece p) {
  return p | static_cast<PieceUnderlyingType>(PieceStatus::Promoted);
}

inline Piece Unpromote(Piece p) {
  return p & ~static_cast<PieceUnderlyingType>(PieceStatus::Promoted);
}

inline bool CanPromote(Piece p) {
  if (IsPromotedPiece(p)) {
    return false;
  }
  PieceType type = PieceTypeFromPiece(p);
  return type != PieceType::King && type != PieceType::Gold;
}

inline std::u8string ShortStringFromPieceTypeAndStatus(PieceUnderlyingType p) {
  auto i = RemoveColorFromPiece(p);
  switch (i) {
  case static_cast<PieceUnderlyingType>(PieceType::Empty):
    return u8"　";
  case static_cast<PieceUnderlyingType>(PieceType::King):
    return u8"玉";
  case static_cast<PieceUnderlyingType>(PieceType::Rook):
    return u8"飛";
  case static_cast<PieceUnderlyingType>(PieceType::Bishop):
    return u8"角";
  case static_cast<PieceUnderlyingType>(PieceType::Gold):
    return u8"金";
  case static_cast<PieceUnderlyingType>(PieceType::Silver):
    return u8"銀";
  case static_cast<PieceUnderlyingType>(PieceType::Knight):
    return u8"桂";
  case static_cast<PieceUnderlyingType>(PieceType::Lance):
    return u8"香";
  case static_cast<PieceUnderlyingType>(PieceType::Pawn):
    return u8"歩";
  case static_cast<PieceUnderlyingType>(PieceType::Rook) | static_cast<PieceUnderlyingType>(PieceStatus::Promoted):
    return u8"龍";
  case static_cast<PieceUnderlyingType>(PieceType::Bishop) | static_cast<PieceUnderlyingType>(PieceStatus::Promoted):
    return u8"馬";
  case static_cast<PieceUnderlyingType>(PieceType::Silver) | static_cast<PieceUnderlyingType>(PieceStatus::Promoted):
    return u8"全";
  case static_cast<PieceUnderlyingType>(PieceType::Knight) | static_cast<PieceUnderlyingType>(PieceStatus::Promoted):
    return u8"圭";
  case static_cast<PieceUnderlyingType>(PieceType::Lance) | static_cast<PieceUnderlyingType>(PieceStatus::Promoted):
    return u8"杏";
  case static_cast<PieceUnderlyingType>(PieceType::Pawn) | static_cast<PieceUnderlyingType>(PieceStatus::Promoted):
    return u8"と";
  }
  return u8"？";
}

inline std::optional<PieceUnderlyingType> TrimPieceTypeAndStatusPartFromString(std::u8string &inout) {
  using namespace std;
  if (inout.starts_with(u8"玉")) {
    inout = inout.substr(u8string(u8"玉").size());
    return static_cast<PieceUnderlyingType>(PieceType::King);
  } else if (inout.starts_with(u8"飛")) {
    inout = inout.substr(u8string(u8"飛").size());
    return static_cast<PieceUnderlyingType>(PieceType::Rook);
  } else if (inout.starts_with(u8"角")) {
    inout = inout.substr(u8string(u8"角").size());
    return static_cast<PieceUnderlyingType>(PieceType::Bishop);
  } else if (inout.starts_with(u8"金")) {
    inout = inout.substr(u8string(u8"金").size());
    return static_cast<PieceUnderlyingType>(PieceType::Gold);
  } else if (inout.starts_with(u8"銀")) {
    inout = inout.substr(u8string(u8"銀").size());
    return static_cast<PieceUnderlyingType>(PieceType::Silver);
  } else if (inout.starts_with(u8"桂")) {
    inout = inout.substr(u8string(u8"桂").size());
    return static_cast<PieceUnderlyingType>(PieceType::Knight);
  } else if (inout.starts_with(u8"香")) {
    inout = inout.substr(u8string(u8"香").size());
    return static_cast<PieceUnderlyingType>(PieceType::Lance);
  } else if (inout.starts_with(u8"歩")) {
    inout = inout.substr(u8string(u8"歩").size());
    return static_cast<PieceUnderlyingType>(PieceType::Pawn);
  } else if (inout.starts_with(u8"龍")) {
    inout = inout.substr(u8string(u8"龍").size());
    return static_cast<PieceUnderlyingType>(PieceType::Rook) | static_cast<PieceUnderlyingType>(PieceStatus::Promoted);
  } else if (inout.starts_with(u8"馬")) {
    inout = inout.substr(u8string(u8"馬").size());
    return static_cast<PieceUnderlyingType>(PieceType::Bishop) | static_cast<PieceUnderlyingType>(PieceStatus::Promoted);
  } else if (inout.starts_with(u8"成銀")) {
    inout = inout.substr(u8string(u8"成銀").size());
    return static_cast<PieceUnderlyingType>(PieceType::Silver) | static_cast<PieceUnderlyingType>(PieceStatus::Promoted);
  } else if (inout.starts_with(u8"全")) {
    inout = inout.substr(u8string(u8"全").size());
    return static_cast<PieceUnderlyingType>(PieceType::Silver) | static_cast<PieceUnderlyingType>(PieceStatus::Promoted);
  } else if (inout.starts_with(u8"成桂")) {
    inout = inout.substr(u8string(u8"成桂").size());
    return static_cast<PieceUnderlyingType>(PieceType::Knight) | static_cast<PieceUnderlyingType>(PieceStatus::Promoted);
  } else if (inout.starts_with(u8"圭")) {
    inout = inout.substr(u8string(u8"圭").size());
    return static_cast<PieceUnderlyingType>(PieceType::Knight) | static_cast<PieceUnderlyingType>(PieceStatus::Promoted);
  } else if (inout.starts_with(u8"成香")) {
    inout = inout.substr(u8string(u8"成香").size());
    return static_cast<PieceUnderlyingType>(PieceType::Lance) | static_cast<PieceUnderlyingType>(PieceStatus::Promoted);
  } else if (inout.starts_with(u8"杏")) {
    inout = inout.substr(u8string(u8"杏").size());
    return static_cast<PieceUnderlyingType>(PieceType::Lance) | static_cast<PieceUnderlyingType>(PieceStatus::Promoted);
  } else if (inout.starts_with(u8"と金")) {
    inout = inout.substr(u8string(u8"と金").size());
    return static_cast<PieceUnderlyingType>(PieceType::Pawn) | static_cast<PieceUnderlyingType>(PieceStatus::Promoted);
  } else if (inout.starts_with(u8"と")) {
    inout = inout.substr(u8string(u8"と").size());
    return static_cast<PieceUnderlyingType>(PieceType::Pawn) | static_cast<PieceUnderlyingType>(PieceStatus::Promoted);
  } else {
    return std::nullopt;
  }
}

inline std::u8string LongStringFromPieceTypeAndStatus(PieceUnderlyingType p) {
  auto i = RemoveColorFromPiece(p);
  switch (i) {
  case static_cast<PieceUnderlyingType>(PieceType::Empty):
    return u8"　";
  case static_cast<PieceUnderlyingType>(PieceType::King):
    return u8"玉";
  case static_cast<PieceUnderlyingType>(PieceType::Rook):
    return u8"飛";
  case static_cast<PieceUnderlyingType>(PieceType::Bishop):
    return u8"角";
  case static_cast<PieceUnderlyingType>(PieceType::Gold):
    return u8"金";
  case static_cast<PieceUnderlyingType>(PieceType::Silver):
    return u8"銀";
  case static_cast<PieceUnderlyingType>(PieceType::Knight):
    return u8"桂";
  case static_cast<PieceUnderlyingType>(PieceType::Lance):
    return u8"香";
  case static_cast<PieceUnderlyingType>(PieceType::Pawn):
    return u8"歩";
  case static_cast<PieceUnderlyingType>(PieceType::Rook) | static_cast<PieceUnderlyingType>(PieceStatus::Promoted):
    return u8"龍";
  case static_cast<PieceUnderlyingType>(PieceType::Bishop) | static_cast<PieceUnderlyingType>(PieceStatus::Promoted):
    return u8"馬";
  case static_cast<PieceUnderlyingType>(PieceType::Silver) | static_cast<PieceUnderlyingType>(PieceStatus::Promoted):
    return u8"成銀";
  case static_cast<PieceUnderlyingType>(PieceType::Knight) | static_cast<PieceUnderlyingType>(PieceStatus::Promoted):
    return u8"成桂";
  case static_cast<PieceUnderlyingType>(PieceType::Lance) | static_cast<PieceUnderlyingType>(PieceStatus::Promoted):
    return u8"成香";
  case static_cast<PieceUnderlyingType>(PieceType::Pawn) | static_cast<PieceUnderlyingType>(PieceStatus::Promoted):
    return u8"と金";
  }
  return u8"？";
}

struct Move;

// 盤面
struct Position {
  Piece pieces[9][9]; // [筋][段]

  // 手番 color の玉に王手がかかっているかどうかを判定
  bool isInCheck(Color color) const;

  bool apply(Move const &, std::deque<PieceType> &handBlack, std::deque<PieceType> &handWhite);
  std::u8string debugString() const;
};

struct LessPosition {
  constexpr bool operator()(Position const &a, Position const &b) const {
    for (int x = 0; x < 9; x++) {
      for (int y = 0; y < 9; y++) {
        if (a.pieces[x][y] == b.pieces[x][y]) {
          continue;
        }
        return a.pieces[x][y] < b.pieces[x][y];
      }
    }
    return false;
  }
};

inline bool operator==(Position const &a, Position const &b) {
  for (int x = 0; x < 9; x++) {
    for (int y = 0; y < 9; y++) {
      if (a.pieces[x][y] != b.pieces[x][y]) {
        return false;
      }
    }
  }
  return true;
}

// ハンデ
enum class Handicap {
  平手,
  香落ち,
  右香落ち,
  角落ち,
  飛車落ち,
  飛香落ち,
  二枚落ち,
  三枚落ち,
  四枚落ち,
  五枚落ち左桂,
  五枚落ち右桂,
  六枚落ち,
  七枚落ち左銀,
  七枚落ち右銀,
  八枚落ち,
  トンボ,
  九枚落ち左金,
  九枚落ち右金,
  十枚落ち,
  青空将棋,
};

inline std::u8string StringFromHandicap(Handicap h) {
#define RET(s)      \
  case Handicap::s: \
    return u8## #s;
  switch (h) {
    RET(平手)
    RET(香落ち)
    RET(右香落ち)
    RET(角落ち)
    RET(飛車落ち)
    RET(飛香落ち)
    RET(二枚落ち)
    RET(三枚落ち)
    RET(四枚落ち)
    RET(五枚落ち左桂)
    RET(五枚落ち右桂)
    RET(六枚落ち)
    RET(七枚落ち左銀)
    RET(七枚落ち右銀)
    RET(八枚落ち)
    RET(トンボ)
    RET(九枚落ち左金)
    RET(九枚落ち右金)
    RET(十枚落ち)
    RET(青空将棋)
  default:
    assert(false);
    return u8"";
  }
#undef RET
}

inline std::optional<std::u8string> KifStringFromHandicap(Handicap h) {
  switch (h) {
  case Handicap::平手:
    return u8"平手";
  case Handicap::香落ち:
    return u8"香落ち";
  case Handicap::右香落ち:
    return u8"右香落ち";
  case Handicap::角落ち:
    return u8"角落ち";
  case Handicap::飛車落ち:
    return u8"飛車落ち";
  case Handicap::飛香落ち:
    return u8"飛香落ち";
  case Handicap::二枚落ち:
    return u8"二枚落ち";
  case Handicap::三枚落ち:
    return u8"三枚落ち";
  case Handicap::四枚落ち:
    return u8"四枚落ち";
  case Handicap::五枚落ち右桂:
    return u8"五枚落ち";
  case Handicap::五枚落ち左桂:
    return u8"左五枚落ち";
  case Handicap::六枚落ち:
    return u8"六枚落ち";
  case Handicap::七枚落ち左銀:
    return u8"左七枚落ち";
  case Handicap::七枚落ち右銀:
    return u8"右七枚落ち";
  case Handicap::八枚落ち:
    return u8"八枚落ち";
  case Handicap::十枚落ち:
    return u8"十枚落ち";
  default:
    return std::nullopt;
  }
}

// whiteHand を指定すると駒渡しになる
inline Position MakePosition(Handicap h, std::deque<PieceType> *whiteHand = nullptr) {
  Position p;
  for (int x = 0; x < 9; x++) {
    for (int y = 0; y < 9; y++) {
      p.pieces[x][y] = 0;
    }
  }
  // 後手
  for (int x = 0; x < 9; x++) {
    p.pieces[x][2] = MakePiece(Color::White, PieceType::Pawn);
  }
  p.pieces[0][0] = p.pieces[8][0] = MakePiece(Color::White, PieceType::Lance);
  p.pieces[1][0] = p.pieces[7][0] = MakePiece(Color::White, PieceType::Knight);
  p.pieces[2][0] = p.pieces[6][0] = MakePiece(Color::White, PieceType::Silver);
  p.pieces[3][0] = p.pieces[5][0] = MakePiece(Color::White, PieceType::Gold);
  p.pieces[4][0] = MakePiece(Color::White, PieceType::King);
  p.pieces[1][1] = MakePiece(Color::White, PieceType::Rook);
  p.pieces[7][1] = MakePiece(Color::White, PieceType::Bishop);
  // 先手
  for (int x = 0; x < 9; x++) {
    p.pieces[x][6] = MakePiece(Color::Black, PieceType::Pawn);
  }
  p.pieces[0][8] = p.pieces[8][8] = MakePiece(Color::Black, PieceType::Lance);
  p.pieces[1][8] = p.pieces[7][8] = MakePiece(Color::Black, PieceType::Knight);
  p.pieces[2][8] = p.pieces[6][8] = MakePiece(Color::Black, PieceType::Silver);
  p.pieces[3][8] = p.pieces[5][8] = MakePiece(Color::Black, PieceType::Gold);
  p.pieces[4][8] = MakePiece(Color::Black, PieceType::King);
  p.pieces[7][7] = MakePiece(Color::Black, PieceType::Rook);
  p.pieces[1][7] = MakePiece(Color::Black, PieceType::Bishop);
  std::deque<PieceType> tmp;
  std::deque<PieceType> *bin = whiteHand ? whiteHand : &tmp;
  auto drop = [&](int f, int r) {
    bin->push_back(PieceTypeFromPiece(p.pieces[9 - f][r - 1]));
    p.pieces[9 - (f)][(r)-1] = 0;
  };
  switch (h) {
  case Handicap::平手:
    break;
  case Handicap::香落ち:
    drop(1, 1);
    break;
  case Handicap::右香落ち:
    drop(9, 1);
    break;
  case Handicap::角落ち:
    drop(2, 2);
    break;
  case Handicap::飛車落ち:
    drop(8, 2);
    break;
  case Handicap::飛香落ち:
    drop(1, 1);
    drop(8, 2);
    break;
  case Handicap::二枚落ち:
    drop(2, 2);
    drop(8, 2);
    break;
  case Handicap::三枚落ち:
    drop(1, 1);
    drop(2, 2);
    drop(8, 2);
    break;
  case Handicap::四枚落ち:
    drop(2, 2);
    drop(8, 2);
    drop(1, 1);
    drop(9, 1);
    break;
  case Handicap::五枚落ち左桂:
    drop(2, 2);
    drop(8, 2);
    drop(1, 1);
    drop(9, 1);
    drop(2, 1);
    break;
  case Handicap::五枚落ち右桂:
    drop(2, 2);
    drop(8, 2);
    drop(1, 1);
    drop(9, 1);
    drop(8, 1);
    break;
  case Handicap::六枚落ち:
    drop(2, 2);
    drop(8, 2);
    drop(1, 1);
    drop(9, 1);
    drop(8, 1);
    drop(2, 1);
    break;
  case Handicap::七枚落ち左銀:
    drop(2, 2);
    drop(8, 2);
    drop(1, 1);
    drop(9, 1);
    drop(8, 1);
    drop(2, 1);
    drop(3, 1);
    break;
  case Handicap::七枚落ち右銀:
    drop(2, 2);
    drop(8, 2);
    drop(1, 1);
    drop(9, 1);
    drop(8, 1);
    drop(2, 1);
    drop(7, 1);
    break;
  case Handicap::八枚落ち:
    drop(2, 2);
    drop(8, 2);
    drop(1, 1);
    drop(9, 1);
    drop(8, 1);
    drop(2, 1);
    drop(3, 1);
    drop(7, 1);
    break;
  case Handicap::トンボ:
    drop(1, 1);
    drop(2, 1);
    drop(3, 1);
    drop(4, 1);
    drop(6, 1);
    drop(7, 1);
    drop(8, 1);
    drop(9, 1);
    break;
  case Handicap::九枚落ち左金:
    drop(1, 1);
    drop(2, 1);
    drop(3, 1);
    drop(4, 1);
    drop(7, 1);
    drop(8, 1);
    drop(9, 1);
    drop(2, 2);
    drop(8, 2);
    break;
  case Handicap::九枚落ち右金:
    drop(1, 1);
    drop(2, 1);
    drop(3, 1);
    drop(6, 1);
    drop(7, 1);
    drop(8, 1);
    drop(9, 1);
    drop(2, 2);
    drop(8, 2);
    break;
  case Handicap::十枚落ち:
    drop(1, 1);
    drop(2, 1);
    drop(3, 1);
    drop(4, 1);
    drop(6, 1);
    drop(7, 1);
    drop(8, 1);
    drop(9, 1);
    drop(2, 2);
    drop(8, 2);
    break;
  case Handicap::青空将棋:
    for (int x = 0; x < 9; x++) {
      p.pieces[x][2] = 0;
      p.pieces[x][6] = 0;
    }
    break;
  }
  return p;
}

// 持ち駒
struct Hand {
  std::deque<PieceType> pieces;
};

// 筋. 右が File1, 左が File9
enum File : int32_t {
  File9 = 0,
  File8,
  File7,
  File6,
  File5,
  File4,
  File3,
  File2,
  File1,
};

inline std::u8string StringFromFile(File f) {
  switch (f) {
  case File1:
    return u8"１";
  case File2:
    return u8"２";
  case File3:
    return u8"３";
  case File4:
    return u8"４";
  case File5:
    return u8"５";
  case File6:
    return u8"６";
  case File7:
    return u8"７";
  case File8:
    return u8"８";
  case File9:
    return u8"９";
  default:
    return u8"";
  }
}

// 段. 上が Rank1, 下が Rank9
enum Rank : int32_t {
  Rank1 = 0,
  Rank2,
  Rank3,
  Rank4,
  Rank5,
  Rank6,
  Rank7,
  Rank8,
  Rank9,
};

inline std::u8string StringFromRank(Rank r) {
  switch (r) {
  case Rank1:
    return u8"一";
  case Rank2:
    return u8"二";
  case Rank3:
    return u8"三";
  case Rank4:
    return u8"四";
  case Rank5:
    return u8"五";
  case Rank6:
    return u8"六";
  case Rank7:
    return u8"七";
  case Rank8:
    return u8"八";
  case Rank9:
    return u8"九";
  default:
    return u8"";
  }
}

struct Square {
  Square() = default;
  Square(File file, Rank rank) : file(file), rank(rank) {}
  File file;
  Rank rank;

  Square rotated() const {
    return Square(static_cast<File>(8 - file), static_cast<Rank>(8 - rank));
  }
};

// 左上(９一)を [0, 0], 右下(１九)を [8, 8] とした時の x, y を元に Square を作る.
inline Square MakeSquare(int x, int y) {
  return Square(static_cast<File>(x), static_cast<Rank>(y));
}

inline bool operator==(Square const &a, Square const &b) {
  return a.file == b.file && a.rank == b.rank;
}

struct LessSquare {
  constexpr bool operator()(Square const &a, Square const &b) const {
    if (a.file == b.file) {
      return a.rank < b.rank;
    } else {
      return a.file < b.file;
    }
  }
};

using SquareSet = std::set<Square, LessSquare>;

inline std::u8string StringFromSquare(Square s) {
  return StringFromFile(s.file) + StringFromRank(s.rank);
}

inline std::optional<Square> TrimSquarePartFromString(std::u8string &inout) {
  std::u8string s = inout;
  static std::map<std::u8string, int32_t> const sMap = {
      {u8"一", 0},
      {u8"１", 0},
      {u8"1", 0},
      {u8"二", 1},
      {u8"２", 1},
      {u8"2", 1},
      {u8"三", 2},
      {u8"３", 2},
      {u8"3", 2},
      {u8"四", 3},
      {u8"４", 3},
      {u8"4", 3},
      {u8"五", 4},
      {u8"５", 4},
      {u8"5", 4},
      {u8"六", 5},
      {u8"６", 5},
      {u8"6", 5},
      {u8"七", 6},
      {u8"７", 6},
      {u8"7", 6},
      {u8"八", 7},
      {u8"８", 7},
      {u8"8", 7},
      {u8"九", 8},
      {u8"９", 8},
      {u8"9", 8}};
  std::optional<File> f;
  for (auto const &it : sMap) {
    if (s.starts_with(it.first)) {
      f = static_cast<File>(8 - it.second);
      s = s.substr(it.first.size());
      break;
    }
  }
  if (!f) {
    return std::nullopt;
  }
  std::optional<Rank> r;
  for (auto const &it : sMap) {
    if (s.starts_with(it.first)) {
      r = static_cast<Rank>(it.second);
      s = s.substr(it.first.size());
      break;
    }
  }
  if (!r) {
    return std::nullopt;
  }
  inout.swap(s);
  return Square(*f, *r);
}

inline std::optional<Square> SquareFromString(std::u8string const &s) {
  std::u8string cp = s;
  return TrimSquarePartFromString(cp);
}

// 手番 color の駒が from から to に移動したとき, 成れる条件かどうか.
inline bool IsPromotableMove(Square from, Square to, Color color) {
  if (color == Color::Black) {
    return from.rank <= Rank::Rank3 || to.rank <= Rank::Rank3;
  } else {
    return from.rank >= Rank::Rank7 || to.rank >= Rank::Rank7;
  }
}

// type の種類の駒が from から to に移動した時, 成らないと反則になる時 true を返す.
inline bool MustPromote(PieceType type, Square from, Square to, Color color) {
  if (!IsPromotableMove(from, to, color)) {
    return false;
  }
  switch (type) {
  case PieceType::Lance:
  case PieceType::Pawn:
    if (color == Color::Black) {
      return to.rank < Rank::Rank2;
    } else {
      return to.rank > Rank::Rank8;
    }
  case PieceType::Knight:
    if (color == Color::Black) {
      return to.rank < Rank::Rank3;
    } else {
      return to.rank > Rank::Rank7;
    }
  default:
    return false;
  }
}

inline bool CanDrop(Position const &p, Piece piece, Square sq) {
  if (IsPromotedPiece(piece)) {
    return false;
  }
  if (p.pieces[sq.file][sq.rank] != 0) {
    return false;
  }
  auto type = PieceTypeFromPiece(piece);
  auto color = ColorFromPiece(piece);
  Rank minRank = Rank1;
  Rank maxRank = Rank9;
  if (type == PieceType::Pawn) {
    if (color == Color::Black) {
      minRank = Rank2;
    } else {
      maxRank = Rank8;
    }
  } else if (type == PieceType::Lance) {
    if (color == Color::Black) {
      minRank = Rank2;
    } else {
      maxRank = Rank8;
    }
  } else if (type == PieceType::Knight) {
    if (color == Color::Black) {
      minRank = Rank3;
    } else {
      maxRank = Rank7;
    }
  } else if (type == PieceType::King) {
    return false;
  }
  if (sq.rank < minRank || maxRank < sq.rank) {
    return false;
  }
  if (type == PieceType::Pawn) {
    for (int y = 0; y < 9; y++) {
      Piece pc = p.pieces[sq.file][y];
      if (pc == MakePiece(color, PieceType::Pawn)) {
        return false;
      }
    }
  }
  return true;
}

using SuffixUnderlyingType = uint32_t;

// 符号を読み上げるのに必要な追加情報.
enum class SuffixType : SuffixUnderlyingType {
  None = 0,
  // 位置を表す Suffix
  Right = 0b01,   // 右
  Left = 0b10,    // 左
  Nearest = 0b11, // 直
  // 動作を表す Suffix
  Up = 0b00100,      // 上
  Down = 0b01000,    // 引
  Sideway = 0b01100, // 寄
  Drop = 0b10000,    // 打ち

  MaskPosition = 0b000011,
  MaskAction = 0b11100,
};

using Suffix = SuffixUnderlyingType;

struct Move {
  Color color;
  // 移動した駒.
  Piece piece;
  // from == nullopt の場合は駒打ち.
  std::optional<Square> from;
  Square to;
  // 成る場合に 1, 不成の場合に -1, 変更なしの場合 0
  int promote = 0;
  // 相手の駒を取った場合, その駒の種類. 成駒を取った場合, promoted フラグは付けたまま, color フラグだけ落とされた状態とする.
  std::optional<PieceUnderlyingType> captured;
  Suffix suffix = static_cast<SuffixUnderlyingType>(SuffixType::None);

  // 盤面の情報から suffix を決める.
  void decideSuffix(Position const &p);

  // from に居る駒が to に効いているかどうかを調べる. from が空きマスだった場合は false を返す.
  static bool CanMove(Position const &p, Square from, Square to);
};

inline bool operator==(Move const &a, Move const &b) {
  if (a.color != b.color) {
    return false;
  }
  if ((bool)a.from != (bool)b.from) {
    return false;
  }
  if (a.from && b.from) {
    if (*a.from != *b.from) {
      return false;
    }
  }
  if (a.to != b.to) {
    return false;
  }
  return a.promote == b.promote;
}

inline std::u8string StringFromMoveWithLastPtr(Move const &mv, Square const *last) {
  std::u8string ret;
  if (mv.color == Color::Black) {
    ret += u8"▲";
  } else {
    ret += u8"△";
  }
  if (last && mv.to == *last) {
    ret += u8"同";
  } else {
    ret += StringFromSquare(mv.to);
  }
  if (mv.promote == 1) {
    ret += LongStringFromPieceTypeAndStatus(static_cast<PieceUnderlyingType>(PieceTypeFromPiece(mv.piece))) + u8"成";
  } else if (mv.promote == -1) {
    ret += LongStringFromPieceTypeAndStatus(mv.piece) + u8"不成";
  } else {
    ret += LongStringFromPieceTypeAndStatus(mv.piece);
  }
  if (mv.from) {
    ret += u8"(" + std::u8string((char8_t const *)std::to_string(9 - mv.from->file).c_str()) + std::u8string((char8_t const *)std::to_string(mv.from->rank + 1).c_str()) + u8")";
  } else {
    ret += u8"打";
  }
  return ret;
}

inline std::u8string StringFromMove(Move const &mv) {
  return StringFromMoveWithLastPtr(mv, nullptr);
}

inline std::u8string StringFromMove(Move const &mv, Square last) {
  return StringFromMoveWithLastPtr(mv, &last);
}

inline std::u8string StringFromMoveWithOptionalLast(Move const &mv, std::optional<Square> last) {
  Square l;
  if (last) {
    l = *last;
  }
  return StringFromMoveWithLastPtr(mv, last ? &l : nullptr);
}

inline double Distance(cv::Point const &a, cv::Point const &b) {
  double dx = a.x - b.x;
  double dy = a.y - b.y;
  return sqrt(dx * dx + dy * dy);
}

inline cv::Point2f PerspectiveTransform(cv::Point2f const &src, cv::Mat const &mtx, bool rotate180, int width, int height) {
  cv::Mat dst;
  std::vector<cv::Point2f> vp;
  vp.push_back(src);
  cv::perspectiveTransform(vp, dst, mtx);
  float x = dst.at<float>(0, 0);
  float y = dst.at<float>(0, 1);
  if (rotate180) {
    return cv::Point2f(-x + width, -y + height);
  } else {
    return cv::Point2f(x, y);
  }
}

inline cv::Point2f WarpAffine(cv::Point2f const &p, cv::Mat const &mtx) {
  double x = mtx.at<double>(0, 0) * p.x + mtx.at<double>(0, 1) * p.y + mtx.at<double>(0, 2);
  double y = mtx.at<double>(1, 0) * p.x + mtx.at<double>(1, 1) * p.y + mtx.at<double>(1, 2);
  return cv::Point2f(x, y);
}

inline void WarpAffine(std::vector<cv::Point2f> &buffer, cv::Mat const &mtx) {
  for (auto &p : buffer) {
    p = WarpAffine(p, mtx);
  }
}

struct RadianAverage {
  void push(double radian) {
    x += cos(radian);
    y += sin(radian);
    count++;
  }

  std::optional<double> get() const {
    if (count == 0) {
      return std::nullopt;
    } else {
      return atan2(y, x);
    }
  }

  double x = 0;
  double y = 0;
  size_t count = 0;
};

struct Contour {
  std::vector<cv::Point2f> points;
  double area;

  // 最も短い辺の長さ/最も長い辺の長さ
  double aspectRatio() const {
    if (points.empty()) {
      return 0;
    }
    double shortest = std::numeric_limits<double>::max();
    double longest = std::numeric_limits<double>::lowest();
    for (int i = 0; i < points.size() - 1; i++) {
      double distance = Distance(points[i], points[i + 1]);
      shortest = std::min(shortest, distance);
      longest = std::max(longest, distance);
    }
    return shortest / longest;
  }

  cv::Point2f mean() const {
    double x = 0;
    double y = 0;
    for (auto const &p : points) {
      x += p.x;
      y += p.y;
    }
    return cv::Point2f(x / points.size(), y / points.size());
  }

  std::optional<cv::Point2f> direction(float length = 1) const;
};

struct PieceContour;

// 駒の形を決めるパラメータ
struct PieceShape {
  // 駒の頂点. PieceContour#center() を原点とした時の PieceContour#points[0] と一致する
  cv::Point2f apex;
  // PieceContour#center() を原点とした時の PieceContour#points[1] と一致する
  cv::Point2f point1;
  // PieceContour#center() を原点とした時の PieceContour#points[2] と一致する
  cv::Point2f point2;

  // PieceContour#mean と center が一致するような頂点の列を計算する.
  void poly(cv::Point2f const &center, std::vector<cv::Point2f> &buffer, Color color) const;
};

// 駒のような形をした Contour. points[0] が駒の頂点, points[2] => points[3] が底辺
struct PieceContour {
  std::vector<cv::Point2f> points;
  double area;
  cv::Point2f direction;
  // 底辺の長さ/(頂点と底辺の中点を結ぶ線分の長さ)
  double aspectRatio;

  cv::Point2f center() const {
    cv::Point2f mid = (points[2] + points[3]) * 0.5f;
    cv::Point2f apex = points[0];
    return (mid + apex) * 0.5f;
  }

  Contour toContour() const {
    Contour c;
    c.points = points;
    c.area = area;
    return c;
  }

  static std::shared_ptr<PieceContour> Make(std::vector<cv::Point2f> const &points);

  PieceShape toShape() const;
};

using LatticeContent = std::variant<std::shared_ptr<Contour>, std::shared_ptr<PieceContour>>;

inline std::shared_ptr<Contour> ContourFromLatticeContent(LatticeContent lc) {
  return std::get<0>(lc);
}

inline std::shared_ptr<PieceContour> PieceContourFromLatticeContent(LatticeContent lc) {
  return std::get<1>(lc);
}

inline cv::Point2f CenterFromLatticeContent(LatticeContent lc) {
  if (lc.index() == 0) {
    return std::get<0>(lc)->mean();
  } else {
    return std::get<1>(lc)->center();
  }
}

template <class T>
struct WeakSet {
  using ConstIterator = std::deque<std::weak_ptr<T>>::const_iterator;
  using Iterator = std::deque<std::weak_ptr<T>>::iterator;

  bool contains(std::shared_ptr<T> const &v) const {
    for (auto const &it : store) {
      auto lk = it.lock();
      if (!lk) {
        continue;
      }
      if (lk == v) {
        return true;
      }
    }
    return false;
  }

  bool contains(std::weak_ptr<T> const &v) const {
    auto s = v.lock();
    if (!s) {
      return false;
    }
    return contains(s);
  }

  Iterator erase(Iterator it) {
    return store.erase(it);
  }

  void insert(std::shared_ptr<T> const &v) {
    if (contains(v)) {
      return;
    }
    std::weak_ptr<T> wk = v;
    store.push_back(wk);
  }

  void insert(std::weak_ptr<T> const &v) {
    if (contains(v)) {
      return;
    }
    store.push_back(v);
  }

  ConstIterator begin() const {
    return store.begin();
  }

  ConstIterator end() const {
    return store.end();
  }

  Iterator begin() {
    return store.begin();
  }

  Iterator end() {
    return store.end();
  }

  std::deque<std::weak_ptr<T>> store;
};

struct Lattice {
  Lattice() {
    Counter().fetch_add(1);
  }

  ~Lattice() {
    Counter().fetch_add(-1);
  }

  static std::atomic<int> &Counter() {
    static std::atomic<int> i = 0;
    return i;
  }

  std::shared_ptr<LatticeContent> content;
  // content と位置が重なっている物.
  std::set<std::shared_ptr<LatticeContent>> center;
  WeakSet<Lattice> adjacent;

  cv::Point2f meanCenter() const {
    cv::Point2f sum = CenterFromLatticeContent(*content);
    for (auto const &c : center) {
      sum += CenterFromLatticeContent(*c);
    }
    return sum / float(center.size() + 1);
  }
};

inline std::shared_ptr<PieceContour> PerspectiveTransform(std::shared_ptr<PieceContour> const &src, cv::Mat const &mtx, bool rotate180, int width, int height) {
  if (!src) {
    return nullptr;
  }
  std::vector<cv::Point2f> points;
  for (auto const &p : src->points) {
    points.push_back(PerspectiveTransform(p, mtx, rotate180, width, height));
  }
  return PieceContour::Make(points);
}

enum class GameResult {
  BlackWin,
  WhiteWin,
  Abort,
};

enum class GameResultReason {
  Resign,
  IllegalAction,
  Abort,
  Repetition,
  CheckRepetition,
};

class Game {
public:
  // 駒渡しにする時 hand = true
  Game(Handicap h, bool hand) : handicap(h), handicapHand(hand) {
    position = MakePosition(h, hand ? &handWhite : nullptr);
  }

  enum class ApplyResult {
    Ok,
    Illegal,
    Repetition,
    CheckRepetitionBlack,
    CheckRepetitionWhite,
  };

  ApplyResult apply(Move const &move);

  std::deque<PieceType> &hand(Color color) {
    if (color == Color::Black) {
      return handBlack;
    } else {
      return handWhite;
    }
  }

  std::deque<PieceType> const &hand(Color color) const {
    if (color == Color::Black) {
      return handBlack;
    } else {
      return handWhite;
    }
  }

  static void Generate(Position const &p, Color color, std::deque<PieceType> const &handBlack, std::deque<PieceType> const &handWhite, std::deque<Move> &moves, bool enablePawnCheckByDrop);
  void generate(std::deque<Move> &moves) const;

public:
  Position position;
  std::vector<Move> moves;
  std::deque<PieceType> handBlack;
  std::deque<PieceType> handWhite;
  Handicap handicap;
  bool handicapHand;

private:
  std::map<Position, size_t, LessPosition> history;
  std::map<Position, size_t, LessPosition> blackCheckHistory;
  std::map<Position, size_t, LessPosition> whiteCheckHistory;
};

class Player {
public:
  virtual ~Player() {}
  virtual std::optional<Move> next(Position const &p, std::vector<Move> const &moves, std::deque<PieceType> const &hand, std::deque<PieceType> const &handEnemy) = 0;
  virtual std::optional<std::u8string> name() = 0;
  virtual void stop() = 0;
};

class RandomAI : public Player {
public:
  RandomAI();
  std::optional<Move> next(Position const &p, std::vector<Move> const &moves, std::deque<PieceType> const &hand, std::deque<PieceType> const &handEnemy) override;
  std::optional<std::u8string> name() override {
    return u8"random";
  }
  void stop() override {}

private:
  std::unique_ptr<std::mt19937_64> engine;
};

class Sunfish3AI : public Player {
public:
  Sunfish3AI();
  ~Sunfish3AI();
  std::optional<Move> next(Position const &p, std::vector<Move> const &moves, std::deque<PieceType> const &hand, std::deque<PieceType> const &handEnemy) override;
  std::optional<std::u8string> name() override {
    return u8"sunfish3";
  }
  void stop() override;

  static void RunTests();

private:
  struct Impl;
  std::unique_ptr<Impl> impl;
};

struct CsaGameSummary {
  struct Receiver {
    std::optional<std::string> protocolVersion;
    std::optional<std::string> protocolMode;
    std::optional<std::string> format;
    std::optional<std::string> declaration;
    std::optional<std::string> gameId;
    std::optional<std::string> playerNameBlack;
    std::optional<std::string> playerNameWhite;
    std::optional<Color> yourTurn;
    std::optional<bool> rematchOnDraw;
    std::optional<Color> toMove;
    std::optional<int> maxMoves;

    std::optional<CsaGameSummary> validate() const;
  };

  std::string protocolVersion;
  std::optional<std::string> protocolMode;
  std::string format;
  std::optional<std::string> declaration;
  std::optional<std::string> gameId;
  std::string playerNameBlack;
  std::string playerNameWhite;
  Color yourTurn;
  std::optional<bool> rematchOnDraw;
  Color toMove;
  std::optional<int> maxMoves;
};

struct CsaPositionReceiver {
  std::optional<std::string> i;
  std::map<int, std::string> ranks;
  std::map<std::tuple<Color, int, int>, PieceUnderlyingType> pieces;
  std::optional<Color> next;
  bool error = false;

  std::optional<std::pair<Game, Color>> validate() const;
};

inline std::optional<PieceUnderlyingType> PieceTypeFromCsaString(std::string const &p) {
  if (p == "FU") {
    return static_cast<PieceUnderlyingType>(PieceType::Pawn);
  } else if (p == "KY") {
    return static_cast<PieceUnderlyingType>(PieceType::Lance);
  } else if (p == "KE") {
    return static_cast<PieceUnderlyingType>(PieceType::Knight);
  } else if (p == "GI") {
    return static_cast<PieceUnderlyingType>(PieceType::Silver);
  } else if (p == "KI") {
    return static_cast<PieceUnderlyingType>(PieceType::Gold);
  } else if (p == "HI") {
    return static_cast<PieceUnderlyingType>(PieceType::Rook);
  } else if (p == "KA") {
    return static_cast<PieceUnderlyingType>(PieceType::Bishop);
  } else if (p == "OU") {
    return static_cast<PieceUnderlyingType>(PieceType::King);
  } else if (p == "TO") {
    return static_cast<PieceUnderlyingType>(PieceType::Pawn) | static_cast<PieceUnderlyingType>(PieceStatus::Promoted);
  } else if (p == "NY") {
    return static_cast<PieceUnderlyingType>(PieceType::Lance) | static_cast<PieceUnderlyingType>(PieceStatus::Promoted);
  } else if (p == "NK") {
    return static_cast<PieceUnderlyingType>(PieceType::Knight) | static_cast<PieceUnderlyingType>(PieceStatus::Promoted);
  } else if (p == "NG") {
    return static_cast<PieceUnderlyingType>(PieceType::Silver) | static_cast<PieceUnderlyingType>(PieceStatus::Promoted);
  } else if (p == "UM") {
    return static_cast<PieceUnderlyingType>(PieceType::Bishop) | static_cast<PieceUnderlyingType>(PieceStatus::Promoted);
  } else if (p == "RY") {
    return static_cast<PieceUnderlyingType>(PieceType::Rook) | static_cast<PieceUnderlyingType>(PieceStatus::Promoted);
  } else {
    return std::nullopt;
  }
}

inline std::variant<Move, std::u8string> MoveFromCsaMove(std::string const &msg, Position const &position) {
  using namespace std;
  if (!msg.starts_with("+") && !msg.starts_with("-")) {
    return u8"指し手を読み取れませんでした";
  }
  if (msg.size() < 7) {
    return u8"指し手を読み取れませんでした";
  }
  Color color = Color::Black;
  if (msg.starts_with("-")) {
    color = Color::White;
  }
  auto fromFile = atoi(msg.substr(1, 1).c_str());
  auto fromRank = atoi(msg.substr(2, 1).c_str());
  auto toFile = atoi(msg.substr(3, 1).c_str());
  auto toRank = atoi(msg.substr(4, 1).c_str());
  auto piece = PieceTypeFromCsaString(msg.substr(5, 2));
  if (!piece) {
    return u8"指し手を読み取れませんでした";
  }
  if (!msg.substr(1).starts_with(to_string(fromFile) + to_string(fromRank) + to_string(toFile) + to_string(toRank))) {
    return u8"指し手を読み取れませんでした";
  }
  if (fromFile < 0 || 9 < fromFile || fromRank < 0 || 9 < fromRank || toFile < 1 || 9 < toFile || toRank < 1 || 9 < toRank) {
    return u8"指し手を読み取れませんでした";
  }
  if ((fromFile == 0 && fromRank != 0) || (fromFile != 0 && fromRank == 0)) {
    return u8"指し手を読み取れませんでした";
  }
  Move mv;
  mv.color = color;
  mv.piece = *piece | static_cast<PieceUnderlyingType>(color);
  mv.to = MakeSquare(9 - toFile, toRank - 1);
  if (fromFile != 0 && fromRank != 0) {
    Square from = MakeSquare(9 - fromFile, fromRank - 1);
    Piece existing = position.pieces[from.file][from.rank];
    if (existing == 0) {
      return u8"盤上にない駒の移動が指定されました";
    }
    if (existing == mv.piece) {
      if (CanPromote(mv.piece) && IsPromotableMove(from, mv.to, color)) {
        mv.promote = -1;
      }
    } else {
      if (Promote(existing) == mv.piece) {
        mv.promote = 1;
      } else {
        return u8"不正な指し手が指定されました";
      }
    }
    mv.from = from;
  }
  Piece captured = position.pieces[mv.to.file][mv.to.rank];
  if (captured != 0) {
    mv.captured = RemoveColorFromPiece(captured);
  }
  mv.decideSuffix(position);
  return mv;
}

std::optional<std::string> CsaStringFromPiece(Piece p, int promote) {
  bool promoted = IsPromotedPiece(p);
  auto type = PieceTypeFromPiece(p);
  if (type == PieceType::Pawn) {
    if (promoted || promote == 1) {
      return "TO";
    } else {
      return "FU";
    }
  } else if (type == PieceType::Lance) {
    if (promoted || promote == 1) {
      return "NY";
    } else {
      return "KY";
    }
  } else if (type == PieceType::Knight) {
    if (promoted || promote == 1) {
      return "NK";
    } else {
      return "KE";
    }
  } else if (type == PieceType::Silver) {
    if (promoted || promote == 1) {
      return "NG";
    } else {
      return "GI";
    }
  } else if (type == PieceType::Gold) {
    return "KI";
  } else if (type == PieceType::Bishop) {
    if (promoted || promote == 1) {
      return "UM";
    } else {
      return "KA";
    }
  } else if (type == PieceType::Rook) {
    if (promoted || promote == 1) {
      return "RY";
    } else {
      return "HI";
    }
  } else if (type == PieceType::King) {
    return "OU";
  }
  return std::nullopt;
}

class CsaServer {
public:
  class Peer {
  public:
    virtual ~Peer() {}
    virtual void onmessage(std::string const &line) = 0;
    virtual void send(std::string const &line) = 0;
    virtual std::string name() const = 0;
  };

  explicit CsaServer(int port);
  ~CsaServer();
  // 駒渡しにする場合に hand = true
  void setLocalPeer(std::shared_ptr<Peer> local, Color color, Handicap h, bool hand);
  void unsetLocalPeer();
  void send(std::string const &msg);
  std::optional<int> port() const;
  bool isGameReady() const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl;
};

struct CsaServerWrapper {
  std::shared_ptr<CsaServer> server;

  std::optional<int> port() const {
    if (server) {
      return server->port();
    } else {
      return false;
    }
  }

  bool isGameReady() const {
    if (server) {
      return server->isGameReady();
    } else {
      return false;
    }
  }

  void unsetLocalPeer() {
    if (server) {
      server->unsetLocalPeer();
    }
  }

  static CsaServerWrapper Create(int port);
};

class CsaAdapter : public Player, public CsaServer::Peer {
public:
  struct Delegate {
    virtual ~Delegate() {}
    virtual void csaAdapterDidProvidePosition(Game const &g) = 0;
    virtual void csaAdapterDidGetError(std::u8string const &what) = 0;
    virtual void csaAdapterDidFinishGame(GameResult, GameResultReason) = 0;
  };

  explicit CsaAdapter(std::weak_ptr<CsaServer> server);
  ~CsaAdapter();
  std::optional<Move> next(Position const &p, std::vector<Move> const &moves, std::deque<PieceType> const &hand, std::deque<PieceType> const &handEnemy) override;
  std::optional<std::u8string> name() override;
  void stop() override;
  std::string name() const override { return "Player"; }

  void onmessage(std::string const &) override;
  void send(std::string const &) override;
  void resign(Color color);

  bool aborted() const { return chudan; }

private:
  void error(std::u8string const &what);

public:
  std::weak_ptr<Delegate> delegate;
  std::optional<Color> color_;

private:
  std::weak_ptr<CsaServer> server;
  std::deque<std::string> stack;
  std::string current;
  bool started = false;
  std::atomic_bool finished = false;
  bool rejected = false;
  std::optional<GameResultReason> reason;
  std::atomic_bool chudan = false;
  std::map<Color, bool> resignSent;

  std::condition_variable cv;
  std::mutex mut;
  std::atomic_bool stopSignal;
  std::deque<Move> moves;
  Game game;

  CsaPositionReceiver positionReceiver;
  CsaGameSummary::Receiver summaryReceiver;
  std::optional<CsaGameSummary> summary;
  // 初期局面と手番
  std::optional<std::pair<Game, Color>> init;
};

struct Status;

// 駒の画像を集めたもの.
struct PieceBook {
  struct Image {
    cv::Mat mat;
    bool cut = false;
    cv::Rect rect;
    void resize(int width, int height);
  };
  struct Entry {
    // 先手向きで格納する.
    std::deque<Image> images;
    cv::Point2d sumApex;
    cv::Point2d sumPoint1;
    cv::Point2d sumPoint2;
    uint64_t sumCount = 0;

    static constexpr size_t kMaxNumImages = 32;

    void each(Color color, std::function<void(cv::Mat const &, std::optional<PieceShape>)> cb) const;
    void push(cv::Mat const &mat, std::optional<PieceShape> shape);
    void resize(int width, int height);
    void gc();
  };

  std::map<PieceUnderlyingType, Entry> store;

  void each(Color color, std::function<void(Piece, cv::Mat const &, std::optional<PieceShape> shape)> cb) const;
  void update(Position const &position, cv::Mat const &board, Status const &s);
  std::string toPng() const;
  static constexpr int kEdgeLineWidth = 2;
};

struct Status {
  Status();

  std::vector<std::shared_ptr<Contour>> contours;
  std::vector<std::shared_ptr<Contour>> squares;
  std::vector<std::shared_ptr<PieceContour>> pieces;
  int width;
  int height;
  // 升目の面積
  float squareArea;
  // マス目のアスペクト比. 横長の将棋盤は存在しないと仮定して, 幅/高さ.
  float aspectRatio;
  // 盤面の向き. 後手番の対局者の座る向き. 手番がまだ不明の場合, boardDirection 回転後に画像の原点に近い側に居る対局者を後手番として扱う.
  float boardDirection = 0;

  // 検出された駒、または升目. squares または pieces の中から最も近かったものが代入される. 検出できなかった場合 nullopt になる.
  std::array<std::array<std::shared_ptr<Contour>, 9>, 9> detected;

  // より正確な盤面のアウトライン.
  std::optional<Contour> preciseOutline;
  // 台形補正済みの盤面画像
  cv::Mat boardWarpedGray;
  cv::Mat boardWarpedColor;
  // 台形補正する際に使用した透視変換
  cv::Mat perspectiveTransform;
  bool rotate = false;
  int warpedWidth;
  int warpedHeight;

  // 試合の最新状況. Session.game のコピー.
  Game game;
  // 直前のフレームと比較した時の各マスの類似度
  double similarity[9][9];
  // 直前の stable board と比較した時の各マスの類似度
  double similarityAgainstStableBoard[9][9];
  double stableBoardThreshold;
  double stableBoardMaxSimilarity;
  // 最新の stable board
  std::optional<cv::Mat> stableBoard;
  bool blackResign = false;
  bool whiteResign = false;
  // session.detected.size() != game.moves.size() の時 true. AI の指し手の駒を人間が動かすのを待っている時に true.
  bool waitingMove = false;
  // 盤面の画像認識が準備完了となった時 true
  bool boardReady = false;
  // AI の示した手と違う手が指されている時に true
  bool wrongMove = false;
  std::optional<GameResultReason> reason;
  std::shared_ptr<PieceBook> book;
  std::deque<std::map<std::pair<int, int>, std::set<std::shared_ptr<Lattice>>>> clusters;
  std::optional<Color> yourTurn;
  bool aborted = false;
};

// 盤面画像.
struct BoardImage {
  cv::Mat gray;
  cv::Mat fullcolor;
  static constexpr double kStableBoardMaxSimilarity = 0.02;
  static constexpr double kStableBoardThreshold = kStableBoardMaxSimilarity * 0.5;

  void rotate() {
    cv::rotate(gray, gray, cv::ROTATE_180);
    cv::rotate(fullcolor, fullcolor, cv::ROTATE_180);
  }
};

struct LessCvPoint {
  constexpr bool operator()(cv::Point const &a, cv::Point const &b) const {
    if (a.x == b.x) {
      return a.y < b.y;
    } else {
      return a.x < b.x;
    }
  }
};

using CvPointSet = std::set<cv::Point, LessCvPoint>;

template <class T>
struct ClosedRange {
  ClosedRange() = default;
  ClosedRange(T minimum, T maximum) : minimum(minimum), maximum(maximum) {}
  T minimum;
  T maximum;
};

struct Statistics {
  Statistics();

  std::deque<float> squareAreaHistory;
  std::optional<float> squareArea;

  std::deque<float> aspectRatioHistory;
  std::optional<float> aspectRatio;

  void update(Status const &s);

  std::deque<BoardImage> boardHistory;
  std::deque<std::array<BoardImage, 3>> stableBoardHistory;
  void push(cv::Mat const &boardGray, cv::Mat const &boardFullcolor, Status &s, Game &g, std::vector<Move> &detected, bool detectMove);
  // 盤面画像を180度回転してから盤面認識処理すべき場合に true.
  bool rotate = false;

  std::deque<Move> moveCandidateHistory;

  static std::optional<Move> Detect(cv::Mat const &boardBeforeGray, cv::Mat const &boardBeforeColor,
                                    cv::Mat const &boardAfterGray, cv::Mat const &boardAfterColor,
                                    CvPointSet const &changes,
                                    Position const &position,
                                    std::vector<Move> const &moves,
                                    Color const &color,
                                    std::deque<PieceType> const &hand,
                                    PieceBook &book,
                                    std::optional<Move> hint,
                                    Status const &s,
                                    hwm::task_queue &pool);
  std::shared_ptr<PieceBook> book;
  // stableBoardHistory をリセットする処理で, 最初の stableBoardHistory との差分がデカいフレームがいくつ連続したかを数えるカウンター.
  // これが閾値を超えたら stableBoardHistory をリセットする.
  int stableBoardInitialResetCounter = 0;
  // stableBoardHistory を ready 判定とする閾値
  int stableBoardInitialReadyCounter = 0;
  std::unique_ptr<hwm::task_queue> pool;

  std::deque<cv::Point2f> outlineTL;
  std::deque<cv::Point2f> outlineTR;
  std::deque<cv::Point2f> outlineBR;
  std::deque<cv::Point2f> outlineBL;
  static int constexpr kOutlineMaxCount = 16;
  static int constexpr kOutlineMaxCountDuringGame = 1500;

  static int constexpr kStableBoardCounterThreshold = 5;
  static int constexpr kBoardArea = 74000;
};

struct PlayerConfig {
  struct Remote {
    std::shared_ptr<CsaAdapter> csa;
    std::shared_ptr<Player> local;
  };
  struct Local {
    std::shared_ptr<Player> black;
    std::shared_ptr<Player> white;
  };
  std::variant<Local, Remote> players;
};

struct Players {
  std::shared_ptr<Player> black;
  std::shared_ptr<Player> white;
};

struct GameStartParameter {
  Color userColor;
  Handicap handicap;
  bool hand;
  // 1~: sunfish
  // other: random
  int option;
  std::shared_ptr<CsaServer> server;
};

class Session : public CsaAdapter::Delegate, public std::enable_shared_from_this<Session> {
public:
  Session();
  ~Session();
  void push(cv::Mat const &frame);
  void setPlayerConfig(std::shared_ptr<PlayerConfig> config) {
    if (!game.moves.empty() || this->players) {
      return;
    }
    playerConfig = config;
  }
  Status status() {
    std::shared_ptr<Status> cp = s;
    return *cp;
  }
  void startGame(GameStartParameter parameter);
  void stopGame();
  void resign(Color color);
  std::optional<std::u8string> name(Color color);

  void csaAdapterDidProvidePosition(Game const &g) override;
  void csaAdapterDidGetError(std::u8string const &what) override;
  void csaAdapterDidFinishGame(GameResult, GameResultReason) override;

private:
  void run();

private:
  std::thread th;
  std::atomic<bool> stop;
  std::condition_variable cv;
  std::mutex mut;
  std::deque<cv::Mat> queue;
  std::shared_ptr<Status> s;
  Statistics stat;
  Game game;
  std::vector<Move> detected;
  std::shared_ptr<PlayerConfig> playerConfig;
  std::shared_ptr<Players> players;
  std::optional<std::future<std::pair<Color, std::optional<Move>>>> next;
};

class Img {
  Img() = delete;

public:
  static cv::Rect PieceROIRect(cv::Size const &size, int x, int y);
  static cv::Mat PieceROI(cv::Mat const &board, int x, int y);
  static void Compare(BoardImage const &before, BoardImage const &after, CvPointSet &buffer, double similarity[9][9] = nullptr);
  // 2 つの画像を同じサイズになるよう変形する
  static std::pair<cv::Mat, cv::Mat> Equalize(cv::Mat const &a, cv::Mat const &b);
  struct ComparePieceCache {
    std::map<std::tuple<int, int, int>, cv::Mat> images;
  };
  // 2 枚の画像を比較する. right を ±degrees 度, x と y 方向にそれぞれ ±width*translationRatio, ±height*translationRatio 移動して画像の一致度を計算し, 最大の一致度を返す.
  static std::pair<double, cv::Mat> ComparePiece(cv::Mat const &board,
                                                 int x, int y,
                                                 cv::Mat const &tmpl,
                                                 Color targetColor,
                                                 std::optional<PieceShape> shape,
                                                 hwm::task_queue &pool,
                                                 ComparePieceCache &cache);
  static double Similarity(cv::Mat const &before, cv::Mat const &after, int x, int y);
  static std::string EncodeToPng(cv::Mat const &image);
  static void Bitblt(cv::Mat const &src, cv::Mat &dst, int x, int y);
  static void FindContours(cv::Mat const &img, std::vector<std::shared_ptr<Contour>> &contours, std::vector<std::shared_ptr<Contour>> &squares, std::vector<std::shared_ptr<PieceContour>> &pieces);
  static void Bin(cv::Mat const &input, cv::Mat &output);
};

class Utility {
  Utility() = delete;

public:
#if defined(__APPLE__)
  static cv::Mat MatFromUIImage(void *ptr);
  static cv::Mat MatFromCGImage(void *ptr);
  static void *UIImageFromMat(cv::Mat const &m);
  static CFStringRef CFStringFromU8String(std::u8string const &s) {
    return CFStringCreateWithCString(kCFAllocatorDefault, (char const *)s.c_str(), kCFStringEncodingUTF8);
  }
#endif // defined(__APPLE__)
};

class SessionWrapper {
public:
  SessionWrapper();

  void push(cv::Mat const &frame) {
    ptr->push(frame);
  }

  Status status() const {
    return ptr->status();
  }

  void startGame(Color userColor, int option) {
    GameStartParameter p;
    p.userColor = userColor;
    p.handicap = Handicap::平手;
    p.hand = false;
    p.option = option;
    ptr->startGame(p);
  }

  void startGame(Color userColor, CsaServerWrapper server) {
    GameStartParameter p;
    p.userColor = userColor;
    p.handicap = Handicap::平手;
    p.hand = false;
    p.option = -1;
    p.server = server.server;
    ptr->startGame(p);
  }

  void startGame(Handicap h, bool hand, int option) {
    GameStartParameter p;
    p.userColor = Color::White;
    p.handicap = h;
    p.hand = hand;
    p.option = option;
    ptr->startGame(p);
  }

  void startGame(Handicap h, bool hand, CsaServerWrapper server) {
    GameStartParameter p;
    p.userColor = Color::White;
    p.handicap = h;
    p.hand = hand;
    p.option = -1;
    p.server = server.server;
    ptr->startGame(p);
  }

  void stopGame() {
    ptr->stopGame();
  }

  void resign(Color color) {
    ptr->resign(color);
  }

  std::optional<std::u8string> name(Color color) {
    return ptr->name(color);
  }

private:
  std::shared_ptr<Session> ptr;
};

void RunTests();

} // namespace sci
