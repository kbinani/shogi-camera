#pragma once

#include <opencv2/core.hpp>

#include <deque>
#include <map>
#include <set>
#include <string>
#include <thread>

#if defined(__APPLE__)
#include <swift/bridging>
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

inline PieceType PieceTypeFromPiece(Piece p) {
  return static_cast<PieceType>(p & 0b1111);
}

inline bool IsPromotedPiece(Piece p) {
  return (p & 0b10000) == 0b10000;
}

inline Piece Promote(Piece p) {
  return p | static_cast<PieceUnderlyingType>(PieceStatus::Promoted);
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

// 盤面
struct Position {
  Piece pieces[9][9]; // [筋][段]
};

std::u8string DebugStringFromPosition(Position const &p);

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
  p.pieces[7][7] = MakePiece(Color::Black, PieceType::Rook);
  p.pieces[1][7] = MakePiece(Color::Black, PieceType::Bishop);
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

// from に居る駒が to に効いているかどうかを調べる. from が空きマスだった場合は false を返す.
bool CanMove(Position const &p, Square from, Square to);

struct Move {
  Color color;
  // 移動した駒.
  Piece piece;
  // from == nullopt の場合は駒打ち.
  std::optional<Square> from;
  Square to;
  // 成る場合に 1, 不成の場合に -1, 変更なしの場合 0
  int promote = 0;
  // 相手の駒を取った場合, その駒の種類
  std::optional<PieceType> newHand;
};

inline bool operator==(Move const &a, Move const &b) {
  if (a.color != b.color) {
    return false;
  }
  if ((bool)a.from != (bool)b.from) {
    return false;
  }
  if (a.from && b.from) {
    return *a.from == *b.from;
  }
  if (a.to != b.to) {
    return false;
  }
  return a.promote == b.promote;
}

inline std::u8string StringFromMove(Move const &mv, std::optional<Square> last) {
  std::u8string ret;
  if (mv.color == Color::Black) {
    ret += u8"▲";
  } else {
    ret += u8"▽";
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
  if (!mv.from) {
    ret += u8"打";
  }
  return ret;
}

inline std::u8string StringFromMove(Move const &mv) {
  return StringFromMove(mv, std::nullopt);
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

  static std::shared_ptr<PieceContour> Make(std::vector<cv::Point2f> const &points);
};

struct Game {
  Position position;
  std::vector<Move> moves;
  std::deque<PieceType> handBlack;
  std::deque<PieceType> handWhite;

  Game() {
    position = MakePosition(Handicap::None);
  }

  void apply(Move const &move);

  std::deque<PieceType> &hand(Color color) {
    if (color == Color::Black) {
      return handBlack;
    } else {
      return handWhite;
    }
  }
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
  // 駒・升の中心に外接する矩形. 中心なので面積は駒面積の約 8x8 = 64 倍の範囲. 座標系は入力画像の座標系.
  Contour outline;
  // bx, by: "9一" の中心座標. 値は入力画像を -boardDirection 回転した座標系での値.
  float bx;
  float by;
  // bwidth, bheight: [bx + bwidth, by + bheight] が "1九" の中心座標. 値は入力画像を -boardDirection 回転した座標系での値.
  float bwidth;
  float bheight;

  std::vector<cv::Vec4f> vgrids;
  std::vector<cv::Vec4f> hgrids;
  // 検出された駒、または升目. squares または pieces の中から最も近かったものが代入される. 検出できなかった場合 nullopt になる.
  std::array<std::array<std::shared_ptr<Contour>, 9>, 9> detected;

  // より正確な盤面のアウトライン.
  std::optional<Contour> preciseOutline;
  // 台形補正済みの盤面画像
  cv::Mat boardWarped;

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
};

// 盤面画像.
struct BoardImage {
  cv::Mat image;
  static constexpr double kStableBoardMaxSimilarity = 0.02;
  static constexpr double kStableBoardThreshold = kStableBoardMaxSimilarity * 0.5;
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

// 駒の画像を集めたもの.
struct PieceBook {
  struct Entry {
    std::optional<cv::Mat> blackInit;
    // 先手向きで格納する.
    std::optional<cv::Mat> whiteInit;
    std::deque<cv::Mat> blackLast;
    // 先手向きで格納する.
    std::deque<cv::Mat> whiteLast;

    static constexpr size_t kMaxLastImageCount = 4;

    void each(Color color, std::function<void(cv::Mat const &)> cb) const;
    void push(cv::Mat const &img, Color color);
  };

  std::map<PieceUnderlyingType, Entry> store;

  void each(Color color, std::function<void(Piece, cv::Mat const &ete)> cb) const;
  void update(Position const &position, cv::Mat const &board);
  std::string toPng() const;
};

struct Statistics {
  std::deque<float> squareAreaHistory;
  std::optional<float> squareArea;

  std::deque<float> aspectRatioHistory;
  std::optional<float> aspectRatio;

  std::deque<Contour> preciseOutlineHistory;
  std::optional<Contour> preciseOutline;

  void update(Status const &s);

  std::deque<BoardImage> boardHistory;
  std::deque<std::array<BoardImage, 3>> stableBoardHistory;
  void push(cv::Mat const &board, Status &s, Game &g);
  // 盤面画像を180度回転してから盤面認識処理すべき場合に true.
  bool rotate = false;

  std::deque<Move> moveCandidateHistory;

  static std::optional<Move> Detect(cv::Mat const &boardBefore,
                                    cv::Mat const &boardAfter,
                                    CvPointSet const &changes,
                                    Position const &position,
                                    std::vector<Move> const &moves,
                                    Color const &color,
                                    std::deque<PieceType> const &hand,
                                    PieceBook &book);
  PieceBook book;
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
  Statistics stat;
  Game game;
};

class Img {
  Img() = delete;

public:
  static cv::Mat PieceROI(cv::Mat const &board, int x, int y, float shrink = 1);
  static void Compare(BoardImage const &before, BoardImage const &after, CvPointSet &buffer, double similarity[9][9] = nullptr);
  // 2 つの画像を同じサイズになるよう変形する
  static std::pair<cv::Mat, cv::Mat> Equalize(cv::Mat const &a, cv::Mat const &b);
  // 2 枚の画像を比較する. right を ±degrees 度, x と y 方向にそれぞれ ±width*translationRatio, ±height*translationRatio 移動して画像の一致度を計算し, 最大の一致度を返す.
  static double Similarity(cv::Mat const &left, cv::Mat const &right, int degrees = 5, float translationRatio = 0.5f);
  static std::string EncodeToBase64(cv::Mat const &image);
  static void Bitblt(cv::Mat const &src, cv::Mat &dst, int x, int y);
};

class Utility {
  Utility() = delete;

public:
#if defined(__APPLE__)
  static cv::Mat MatFromUIImage(void *ptr);
  static void *UIImageFromMat(cv::Mat const &m);
  static char const *CStringFromU8String(std::u8string s) SWIFT_RETURNS_INDEPENDENT_VALUE {
    return (char const *)s.c_str();
  }
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
