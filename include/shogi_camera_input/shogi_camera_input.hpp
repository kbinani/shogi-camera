#pragma once

#include <opencv2/core/core.hpp>

#include <deque>
#include <map>
#include <string>
#include <thread>

namespace sci {

using PieceUnderlyingType = uint32_t;

// 駒の種類
enum class PieceType : PieceUnderlyingType {
  Empty = 0, // 升目に駒が無いことを表す時に使う
  King = 0b0001,
  Rook = 0b0010,
  Bishop = 0b0011,
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

using Piece = PieceUnderlyingType; // PieceType | PieceStatus | Color;

inline Piece MakePiece(Color color, PieceType type, PieceStatus status = PieceStatus::Default) {
  return static_cast<PieceUnderlyingType>(color) | static_cast<PieceUnderlyingType>(type) | static_cast<PieceUnderlyingType>(status);
}

// 盤面
struct Position {
  Piece pieces[9][9]; // [筋][段]
};

// ハンデ
enum class Handicap {
  None,
};

inline Position MakePosition(Handicap h) {
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
  p.pieces[1][7] = MakePiece(Color::Black, PieceType::Rook);
  p.pieces[7][7] = MakePiece(Color::Black, PieceType::Bishop);
  return p;
}

// 持ち駒
struct Hand {
  std::deque<PieceType> pieces;
};
#if 0
// 局面
struct Board {
  Position position;
  Hand black;
  Hand white;
  uint32_t step;
};

inline Board MakeBoard(Handicap h) {
  Board b;
  b.position = MakePosition(h);
  b.step = 0;
  return b;
}
#endif
// 筋. 右が File1, 左が File9
enum File : int32_t {
  File1 = 0,
  File2,
  File3,
  File4,
  File5,
  File6,
  File7,
  File8,
  File9,
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
};

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
      f = static_cast<File>(it.second);
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

inline double Distance(cv::Point const &a, cv::Point const &b) {
  double dx = a.x - b.x;
  double dy = a.y - b.y;
  return sqrt(dx * dx + dy * dy);
}

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

// 駒のような形をした Contour. points[0] が駒の頂点, points[2] => points[3] が底辺
struct PieceContour {
  std::vector<cv::Point2f> points;
  double area;
  cv::Point2f direction;
  // 底辺の長さ/(頂点と底辺の中点を結ぶ線分の長さ)
  double aspectRatio;

  cv::Point2f mean() const {
    double x = 0;
    double y = 0;
    for (auto const &p : points) {
      x += p.x;
      y += p.y;
    }
    return cv::Point2f(x / points.size(), y / points.size());
  }

  Contour toContour() const {
    Contour c;
    c.points = points;
    c.area = area;
    return c;
  }

  static std::optional<PieceContour> Make(std::vector<cv::Point2f> const &points);
};

struct Status {
  Status();

  std::vector<Contour> contours;
  std::vector<Contour> squares;
  std::vector<PieceContour> pieces;
  int width;
  int height;
  // 升目の面積
  float squareArea;
  // マス目のアスペクト比. 横長の将棋盤は存在しないと仮定して, 幅/高さ.
  float aspectRatio;
  // 盤面の向き. 後手番の対局者の座る向き. 手番がまだ不明の場合, boardDirection 回転後に画像の原点に近い側に居る対局者を後手番として扱う.
  float boardDirection = 0;
  // 駒・升の中心に外接する矩形. 中心なので面積は駒面積の約 8x8 = 64 倍の範囲. 座標系は入力画像の座標系.
  Contour outline;
  // bx, by: "9一" の中心座標. 値は入力画像を -boardDirection 回転した座標系での値.
  float bx;
  float by;
  // bwidth, bheight: [bx + bwidth, by + bheight] が "1九" の中心座標. 値は入力画像を -boardDirection 回転した座標系での値.
  float bwidth;
  float bheight;

  // 検出された駒、または升目. squares または pieces の中から最も近かったものが代入される. 検出できなかった場合 nullopt になる.
  std::array<std::array<std::optional<Contour>, 9>, 9> detected;

  // より正確な盤面のアウトライン.
  std::optional<Contour> preciseOutline;

  Position position;
};

class Session {
public:
  Session();
  ~Session();
  void push(cv::Mat const &frame);

  Status status() {
    auto cp = s;
    return *cp;
  }

private:
  void run();

private:
  std::thread th;
  std::atomic<bool> stop;
  std::condition_variable cv;
  std::mutex mut;
  std::deque<cv::Mat> queue;
  std::shared_ptr<Status> s;
};

class Utility {
  Utility() = delete;

public:
#if defined(__APPLE__)
  static cv::Mat MatFromUIImage(void *ptr);
  static void *UIImageFromMat(cv::Mat const &m);
#endif // defined(__APPLE__)
};

class SessionWrapper {
public:
  SessionWrapper() : ptr(std::make_shared<Session>()) {}

  void push(cv::Mat const &frame) {
    ptr->push(frame);
  }

  Status status() const {
    return ptr->status();
  }

private:
  std::shared_ptr<Session> ptr;
};

} // namespace sci
