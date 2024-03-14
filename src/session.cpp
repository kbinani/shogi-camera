#include <shogi_camera/shogi_camera.hpp>

#include "base64.hpp"
#include <opencv2/imgproc.hpp>

#include <format>
#include <iostream>
#include <limits>
#include <numbers>

namespace sci {

namespace {

float Angle(cv::Point2f const &a) {
  return atan2f(a.y, a.x);
}

cv::Point2d Rotate(cv::Point2d const &p, double radian) {
  return cv::Point2d(cos(radian) * p.x - sin(radian) * p.y, sin(radian) * p.x + cos(radian) * p.y);
}

float Normalize90To90(float a) {
  using namespace std::numbers;
  while (a < -pi) {
    a += pi;
  }
  while (a > pi) {
    a -= pi;
  }
  return a;
}

float MeanAngle(std::initializer_list<float> values) {
  RadianAverage ra;
  for (float v : values) {
    ra.push(v);
  }
  return ra.get();
}

std::optional<float> SquareDirection(std::vector<cv::Point2f> const &points) {
  using namespace std;
  if (points.size() != 4) {
    return nullopt;
  }
  // 最も短い辺のインデックス.
  int index = -1;
  float shortest = numeric_limits<float>::max();
  for (int i = 0; i < (int)points.size(); i++) {
    float length = cv::norm(points[i] - points[(i + 1) % 4]);
    if (length < shortest) {
      shortest = length;
      index = i;
    }
  }
  // 長辺の傾き
  float angle0 = Normalize90To90(Angle(points[(index + 1) % 4] - points[(index + 2) % 4]));
  float angle1 = Normalize90To90(Angle(points[(index + 3) % 4] - points[index % 4]));
  return MeanAngle({angle0, angle1});
}

std::optional<cv::Point2d> Intersection(cv::Point2d const &p1, cv::Point2d const &p2, cv::Point2d const &q1, cv::Point2d const &q2) {
  using namespace std;
  cv::Point2d v = p2 - p1;
  cv::Point2d w = q2 - q1;

  double cross = w.x * v.y - w.y * v.x;
  if (!isnormal(cross)) {
    // 平行
    return nullopt;
  }

  if (w.x != 0) {
    double s = (q1.y * w.x + p1.x * w.y - q1.x * w.y - w.x * p1.y) / cross;
    if (s == 0 || isnormal(s)) {
      return p1 + v * s;
    } else {
      return nullopt;
    }
  } else {
    if (v.x != 0) {
      double s = (q1.x - p1.x) / v.x;
      if (s == 0 || isnormal(s)) {
        return p1 + v * s;
      } else {
        return nullopt;
      }
    } else {
      // v と w がどちらも y 軸に平行
      return nullopt;
    }
  }
}

std::optional<cv::Point2d> Intersection(cv::Vec4f const &a, cv::Vec4f const &b) {
  cv::Point2d p1(a[2], a[3]);
  cv::Point2d p2(a[2] + a[0], a[3] + a[1]);
  cv::Point2d q1(b[2], b[3]);
  cv::Point2d q2(b[2] + b[0], b[3] + b[1]);
  return Intersection(p1, p2, q1, q2);
}

// 直線 line と点 p との距離を計算する. line は [vx, vy, x0, y0] 形式(cv::fitLine の結果の型と同じ形式)
double Distance(cv::Vec4f const &line, cv::Point2d const &p) {
  // line と直行する直線
  cv::Vec4f c(line[1], line[0], p.x, p.y);
  if (auto i = Intersection(line, c); i) {
    return cv::norm(*i - p);
  } else {
    return std::numeric_limits<double>::infinity();
  }
}

std::optional<cv::Vec4f> FitLine(std::vector<cv::Point2f> const &points) {
  if (points.size() < 2) {
    return std::nullopt;
  }
  cv::Vec4f line;
  cv::fitLine(cv::Mat(points), line, cv::DIST_L2, 0, 0.01, 0.01);
  return line;
}

std::optional<cv::Point2f> PerpendicularFoot(cv::Vec4f const &line, cv::Point2f const &point) {
  // point を通り line に直行する直線
  cv::Vec4f perpendicular(line[1], line[0], point.x, point.y);
  return Intersection(line, perpendicular);
}

std::optional<float> YFromX(cv::Vec4f const &line, float x) {
  // p = b + t * a
  cv::Point2f a(line[0], line[1]);
  cv::Point2f b(line[2], line[3]);
  // px = bx + t * ax
  // t = (px - bx) / ax
  float t = (x - b.x) / a.x;
  if (t == 0 || std::isnormal(t)) {
    return b.y + t * a.y;
  } else {
    return std::nullopt;
  }
}

bool SimilarArea(double a, double b) {
  double r = a / b;
  return 0.6 <= r && r <= 1.4;
}

bool IsAdjSquareAndSquare(Contour const &a, Contour const &b, float width, float height, float th) {
  assert(a.points.size() == 4);
  assert(b.points.size() == 4);
  float distance = cv::norm(a.mean() - b.mean());
  float size = std::min(width, height);
  if (distance > size * 1.5f) {
    // 遠すぎる
    return false;
  }
  if (distance < size * 0.5f) {
    // 近すぎる
    return false;
  }
  if (!SimilarArea(a.area, b.area)) {
    return false;
  }
  for (int i = 0; i < 4; i++) {
    cv::Point2f p0 = a.points[i];
    cv::Point2f p1 = a.points[(i + 1) % 4];
    for (int j = 0; j < 4; j++) {
      cv::Point2f q0 = b.points[j];
      cv::Point2f q1 = b.points[(j + 1) % 4];
      if ((cv::norm(p0 - q0) <= th && cv::norm(p1 - q1) <= th) ||
          (cv::norm(p0 - q1) <= th && cv::norm(p1 - q0) <= th)) {
        return true;
      }
    }
  }
  return false;
}

bool IsAdjSquareAndPiece(Contour const &a, PieceContour const &b, float th) {
  using namespace std;
  // a の中心から 4 辺 4 方向に b の中心があるかどうかを調べる
  cv::Point2f centerA = a.mean();
  cv::Point2f centerB = b.center();
  for (int i = 0; i < 4; i++) {
    cv::Point2f p0 = a.points[i];
    cv::Point2f p1 = a.points[(i + 1) % 4];
    cv::Point2f mid = (p0 + p1) * 0.5f;
    cv::Point2f dir = mid - centerA;

    cv::Point2f estimated = mid + dir;
    if (cv::norm(estimated - centerB) <= th) {
      return true;
    }
  }
  return false;
}

bool IsAdjPieceAndPiece(PieceContour const &a, PieceContour const &b, float width, float height, float th) {
  using namespace std;
  // a の中心から 90 度ごとに探索し, b の中心があるかどうか調べる
  cv::Point2f centerA = a.center();
  cv::Point2f dirA = a.direction / cv::norm(a.direction);
  cv::Point2f centerB = b.center();
  for (int i = 0; i < 4; i++) {
    float len = i % 2 == 0 ? height : width;
    cv::Mat mtx = cv::getRotationMatrix2D(cv::Point2f(0, 0), i * 90, 1);
    cv::Point2f dir = WarpAffine(dirA, mtx);
    cv::Point2f estimated = centerA + dir * len;
    if (cv::norm(estimated - centerB) <= th) {
      return true;
    }
  }
  return false;
}

bool IsAdj(LatticeContent const &a, LatticeContent const &b, float width, float height, float th) {
  using namespace std;
  if (a.index() == 0) {
    shared_ptr<Contour> sqA = get<0>(a);
    if (b.index() == 0) {
      shared_ptr<Contour> sqB = get<0>(b);
      if (IsAdjSquareAndSquare(*sqA, *sqB, width, height, th)) {
        return true;
      }
    } else {
      shared_ptr<PieceContour> pB = get<1>(b);
      if (IsAdjSquareAndPiece(*sqA, *pB, th)) {
        return true;
      }
    }
  } else {
    shared_ptr<PieceContour> pA = get<1>(a);
    if (b.index() == 0) {
      shared_ptr<Contour> sqB = get<0>(b);
      if (IsAdjSquareAndPiece(*sqB, *pA, th)) {
        return true;
      }
    } else {
      shared_ptr<PieceContour> pB = get<1>(b);
      if (IsAdjPieceAndPiece(*pB, *pA, width, height, th)) {
        return true;
      }
    }
  }
  return false;
}

void FindBoard(cv::Mat const &frame, Status &s, Statistics &stat) {
  using namespace std;
  using namespace std::numbers;
  vector<shared_ptr<Contour>> squares;
  for (auto const &square : s.squares) {
    if (square->points.size() != 4) {
      continue;
    }
    squares.push_back(square);
  }
  if (squares.empty()) {
    return;
  }
  // 四角形の面積の中央値を升目の面積(squareArea)とする.
  sort(squares.begin(), squares.end(), [](shared_ptr<Contour> const &a, shared_ptr<Contour> const &b) { return a->area < b->area; });
  size_t mid = squares.size() / 2;
  if (squares.size() % 2 == 0) {
    auto const &a = squares[mid - 1];
    auto const &b = squares[mid];
    s.squareArea = (a->area + b->area) * 0.5;
    s.aspectRatio = (a->aspectRatio() + b->aspectRatio()) * 0.5;
  } else {
    s.squareArea = squares[mid]->area;
    s.aspectRatio = squares[mid]->aspectRatio();
  }

  set<shared_ptr<Lattice>> lattices;
  // 同じ点と判定するための閾値. この距離より近ければ同じ点と見做す.
  float const distanceTh = sqrt(s.squareArea) * 0.3;
  {
    for (auto const &square : s.squares) {
      bool found = false;
      cv::Point2f center = square->mean();
      for (auto &lattice : lattices) {
        if (lattice->content->index() == 0) {
          auto sq = get<0>(*lattice->content);
          if (cv::pointPolygonTest(sq->points, center, false) >= 0 && SimilarArea(square->area, sq->area)) {
            lattice->center.insert(make_shared<LatticeContent>(square));
            found = true;
            break;
          }
        } else {
          auto pi = get<1>(*lattice->content);
          if (cv::pointPolygonTest(pi->points, center, false) >= 0) {
            lattice->center.insert(make_shared<LatticeContent>(square));
            found = true;
            break;
          }
        }
      }
      if (!found) {
        auto l = make_shared<Lattice>();
        l->content = make_shared<LatticeContent>(square);
        lattices.insert(l);
      }
    }
    for (auto const &piece : s.pieces) {
      bool found = false;
      cv::Point2f center = piece->center();
      for (auto &lattice : lattices) {
        if (lattice->content->index() == 0) {
          auto sq = get<0>(*lattice->content);
          if (cv::pointPolygonTest(sq->points, center, false) >= 0) {
            lattice->center.insert(make_shared<LatticeContent>(piece));
            found = true;
            break;
          }
        } else {
          auto pi = get<1>(*lattice->content);
          if (cv::pointPolygonTest(pi->points, center, false) >= 0 && SimilarArea(piece->area, pi->area)) {
            lattice->center.insert(make_shared<LatticeContent>(piece));
            found = true;
            break;
          }
        }
      }
      if (!found) {
        auto l = make_shared<Lattice>();
        l->content = make_shared<LatticeContent>(piece);
        lattices.insert(l);
      }
    }
    float w = sqrt(s.squareArea * s.aspectRatio);
    float h = sqrt(s.squareArea / s.aspectRatio);
    for (auto const &lattice : lattices) {
      for (auto const &other : lattices) {
        if (lattice == other) {
          continue;
        }
        if (lattice->adjacent.find(other) != lattice->adjacent.end()) {
          continue;
        }
        if (IsAdj(*lattice->content, *other->content, w, h, distanceTh)) {
          lattice->adjacent.insert(other);
          continue;
        }
        bool found = false;
        for (auto const &c : lattice->center) {
          if (IsAdj(*c, *other->content, w, h, distanceTh)) {
            lattice->adjacent.insert(other);
            found = true;
            break;
          }
        }
        if (found) {
          continue;
        }
        for (auto const &c : other->center) {
          if (IsAdj(*c, *lattice->content, w, h, distanceTh)) {
            lattice->adjacent.insert(other);
            break;
          }
        }
      }
    }
  }
  {
    // s.lattices から, 隣接しているもの同士を取り出してクラスター化する.
    deque<set<shared_ptr<Lattice>>> clusters;
    while (!lattices.empty()) {
      set<shared_ptr<Lattice>> cluster;
      shared_ptr<Lattice> first = *lattices.begin();
      cluster.insert(first);
      lattices.erase(first);
      bool changed = true;
      while (changed) {
        changed = false;
        for (auto const &l : cluster) {
          for (auto const &adj : l->adjacent) {
            if (cluster.find(adj) != cluster.end()) {
              continue;
            }
            cluster.insert(adj);
            if (auto found = lattices.find(adj); found != lattices.end()) {
              lattices.erase(found);
            }
            changed = true;
          }
        }
      }
      clusters.push_back(cluster);
    }
    sort(clusters.begin(), clusters.end(), [](auto const &a, auto const &b) {
      return a.size() > b.size();
    });

    // クラスター内でx,yのインデックスを付ける.
    for (auto const &cluster : clusters) {
      map<pair<int, int>, set<shared_ptr<Lattice>>> indexed;
      set<shared_ptr<Lattice>> done;

      // 画面中央に一番近い物を方向の基準にする
      cv::Point2f center(s.width * 0.5f, s.height * 0.5f);
      float minDistance = numeric_limits<float>::max();
      shared_ptr<Lattice> pivot;
      optional<cv::Point2f> direction;
      for (auto const &lattice : cluster) {
        float distance = cv::norm(lattice->meanCenter() - center);
        if (distance >= minDistance) {
          continue;
        }
        optional<cv::Point2f> dir;
        if (lattice->content->index() == 0) {
          shared_ptr<Contour> sq = get<0>(*lattice->content);
          if (auto d = sq->direction(); d) {
            dir = *d;
          } else {
            continue;
          }
        } else {
          shared_ptr<PieceContour> piece = get<1>(*lattice->content);
          dir = piece->direction;
        }
        minDistance = distance;
        pivot = lattice;
        direction = dir;
      }
      if (!direction || !pivot) {
        continue;
      }
      double base = atan2(direction->y, direction->x);
      // direction の方向を +y 方向, -90 度回した方向を +x 方向にする.
      indexed[make_pair(0, 0)].insert(pivot);
      done.insert(pivot);
      bool changed = true;
      while (changed) {
        changed = false;
        for (auto const &it : indexed) {
          auto [x, y] = it.first;
          map<pair<int, int>, set<shared_ptr<Lattice>>> update;
          for (auto const &lattice : it.second) {
            cv::Point2f p0 = lattice->meanCenter();
            for (auto const &adj : lattice->adjacent) {
              if (done.find(adj) != done.end()) {
                continue;
              }
              cv::Point2f p1 = adj->meanCenter();
              // lattice から見て, adj はどっち向き?
              cv::Point2f dir = p1 - p0;
              double angle = atan2(dir.y, dir.x);
              double diff = angle - base;
              while (diff < 0) {
                diff += pi * 2;
              }
              int dx = 0;
              int dy = 0;
              if (diff < pi / 4 || pi * 7 / 4 <= diff) {
                dx = 0;
                dy = 1;
              } else if (diff < pi * 3 / 4) {
                dx = -1;
                dy = 0;
              } else if (diff < pi * 5 / 4) {
                dx = 0;
                dy = -1;
              } else {
                dx = 1;
                dy = 0;
              }
              update[make_pair(x + dx, y + dy)].insert(adj);
              done.insert(adj);
            }
          }
          if (!update.empty()) {
            changed = true;
            for (auto const &j : update) {
              for (auto const &k : j.second) {
                indexed[j.first].insert(k);
              }
            }
          }
        }
      }
      int minX = numeric_limits<int>::max();
      int minY = numeric_limits<int>::max();
      for (auto const &it : indexed) {
        auto [x, y] = it.first;
        minX = std::min(minX, x);
        minY = std::min(minY, y);
      }

      map<pair<int, int>, set<shared_ptr<Lattice>>> reindexed;
      for (auto const &it : indexed) {
        auto [x, y] = it.first;
        reindexed[make_pair(x - minX, y - minY)] = it.second;
      }
      s.clusters.push_back(reindexed);
    }
  }
  struct Line {
    cv::Vec4f line;
    vector<cv::Point2f> points;
  };
  map<int, Line> vlines;
  map<int, Line> hlines;
  map<pair<int, int>, cv::Point2f> grids;
  if (!s.clusters.empty()) {
    // s.clusters の 2 番目以降について, s.clusters[0] にマージできないか調べる.
    deque<map<pair<int, int>, set<shared_ptr<Lattice>>>> clusters = s.clusters;
    s.clusters.clear();
    s.clusters.push_back(clusters[0]);
    auto &largest = s.clusters[0];
    for (int k = 1; k < clusters.size(); k++) {
      int minX, maxX, minY, maxY;
      minX = minY = numeric_limits<int>::max();
      maxX = maxY = numeric_limits<int>::min();
      for (auto const &i : largest) {
        auto [x, y] = i.first;
        minX = std::min(minX, x);
        maxX = std::max(maxX, x);
        minY = std::min(minY, y);
        maxY = std::max(maxY, y);
      }
      // largest の外側 dx, dy マス分補外する
      int const dx = std::max(9 - (maxX - minX + 1), 1);
      int const dy = std::max(9 - (maxY - minY + 1), 1);

      // cluster[k] との比較用に, 各格子の中心座標を計算する.
      grids.clear();
      // まずは既に content が存在している lattice の座標を埋める.
      for (auto const &i : largest) {
        auto [x, y] = i.first;
        float sumX = 0;
        float sumY = 0;
        for (auto const &j : i.second) {
          cv::Point2f center = j->meanCenter();
          sumX += center.x;
          sumY += center.y;
        }
        grids[i.first] = cv::Point2f(sumX / i.second.size(), sumY / i.second.size());
      }
      // 補間により, 四隅を除いて足りていない格子の中心座標を計算する.
      // 縦方向の既知の格子毎に, 直線をフィッティングする.
      vlines.clear();
      for (int x = minX; x <= maxX; x++) {
        vector<cv::Point2f> points;
        optional<pair<int, cv::Point2f>> top;
        optional<pair<int, cv::Point2f>> bottom;
        for (auto const &i : largest) {
          auto [x0, y0] = i.first;
          if (x0 != x) {
            continue;
          }
          float sumX = 0;
          float sumY = 0;
          for (auto const &j : i.second) {
            cv::Point2f center = j->meanCenter();
            sumX += center.x;
            sumY += center.y;
          }
          cv::Point2f average(sumX / i.second.size(), sumY / i.second.size());
          points.push_back(average);
          if (!top || !bottom) {
            top = make_pair(y0, average);
            bottom = make_pair(y0, average);
          } else {
            if (top->first > y0) {
              top = make_pair(y0, average);
            }
            if (bottom->first < y0) {
              bottom = make_pair(y0, average);
            }
          }
        }
        if (points.empty() || !top || !bottom) {
          continue;
        }
        if (auto line = FitLine(points); line) {
          Line l;
          // line の方向ベクトルが top -> bottom の向きと逆になっていたら反転させる
          double lineAngle = atan2((*line)[0], (*line)[1]);
          double direction = Angle(bottom->second - top->second);
          if (cos(lineAngle - direction) < 0) {
            l.line = cv::Vec4f(-(*line)[0], -(*line)[1], (*line)[2], (*line)[3]);
          } else {
            l.line = *line;
          }
          l.points.swap(points);
          vlines[x] = l;
        }
      }
      // 横方向の既知の格子毎に, 直線をフィッティングする.
      hlines.clear();
      for (int y = minY; y <= maxY; y++) {
        vector<cv::Point2f> points;
        optional<pair<int, cv::Point2f>> left;
        optional<pair<int, cv::Point2f>> right;
        for (auto const &i : largest) {
          auto [x0, y0] = i.first;
          if (y0 != y) {
            continue;
          }
          float sumX = 0;
          float sumY = 0;
          for (auto const &j : i.second) {
            cv::Point2f center = j->meanCenter();
            sumX += center.x;
            sumY += center.y;
          }
          cv::Point2f average(sumX / i.second.size(), sumY / i.second.size());
          points.push_back(average);
          if (!left || !right) {
            left = make_pair(x0, average);
            right = make_pair(x0, average);
          } else {
            if (left->first > x0) {
              left = make_pair(x0, average);
            }
            if (right->first < x0) {
              right = make_pair(x0, average);
            }
          }
        }
        if (points.empty() || !left || !right) {
          continue;
        }
        if (auto line = FitLine(points); line) {
          Line l;
          // line の方向ベクトルが left -> right と逆向きになっていたら反転させる
          double lineAngle = atan2((*line)[0], (*line)[1]);
          double direction = Angle(right->second - left->second);
          if (cos(lineAngle - direction) < 0) {
            l.line = cv::Vec4f(-(*line)[0], -(*line)[1], (*line)[2], (*line)[3]);
          } else {
            l.line = *line;
          }
          l.points.swap(points);
          hlines[y] = l;
        }
      }
      // フィッティングした直線の傾きを平滑化する
      static auto Equalize = [](map<int, Line> &lines, int extra) {
        // まず角度の平均値を求める. 角度が 0 度をまたぐと傾きの平滑化ができなくなるので, 45 度付近できるよう, まず平均を求める.
        RadianAverage ra;
        vector<cv::Point2f> angles;
        int vmin = numeric_limits<int>::max();
        int vmax = numeric_limits<int>::min();
        for (auto const &it : lines) {
          vmin = std::min(vmin, it.first);
          vmax = std::max(vmax, it.first);
          float v = it.first;
          float angle = atan2(it.second.line[1], it.second.line[0]);
          while (angle < 0) {
            angle += pi * 2;
          }
          while (angle > pi * 2) {
            angle -= pi * 2;
          }
          if (angle > pi) {
            angle -= pi;
          }
          ra.push(angle);
          angles.push_back(cv::Point2f(v, angle));
        }
        float mean = ra.get();
        float offset = pi / 8 - mean;
        float minAngle = numeric_limits<float>::max();
        float maxAngle = numeric_limits<float>::lowest();
        for (size_t i = 0; i < angles.size(); i++) {
          float angle = angles[i].y + offset;
          while (angle < 0) {
            angle += pi * 2;
          }
          while (angle > pi * 2) {
            angle -= pi * 2;
          }
          minAngle = std::min(minAngle, float(angle * 180 / pi));
          maxAngle = std::max(maxAngle, float(angle * 180 / pi));
          angles[i].y = angle;
        }
        if (maxAngle - minAngle >= 30) {
          return;
        }
        auto fit = FitLine(angles);
        if (!fit) {
          return;
        }
        map<int, Line> result;
        // 切片 b を b = B * v + A の形にして等間隔となるようにしたとき, B と A を最小二乗法により最適化する.
        // https://gyazo.com/20b68981f72fe75250d1e6f0534e57a1
        vector<tuple<int, float, Line>> k;
        float alpha = 0;
        float beta = 0;
        float gamma = 0;
        float f = 0;
        float e = 0;
        for (auto const &it : lines) {
          float v = it.first;
          Line line = it.second;
          auto angle = YFromX(*fit, v);
          if (!angle) {
            result[it.first] = line;
            continue;
          }
          // offset 回転させているので, *angle はだいたい 45 度付近になっている.
          float m = tan(*angle);
          if (!(m == 0 || isnormal(m))) {
            result[it.first] = line;
            continue;
          }
          float n = line.points.size();
          for (auto const &p : line.points) {
            float x = cos(offset) * p.x - sin(offset) * p.y;
            float y = sin(offset) * p.x + cos(offset) * p.y;
            gamma += y - m * x;
            f += (y - m * x) * v;
          }
          alpha += n;
          beta += n * v;
          e += n * v * v;
          k.push_back(make_tuple(it.first, *angle, line));
        }
        float B = (f - beta * gamma / alpha) / (e - beta * beta / alpha);
        float A = (gamma - beta * B) / alpha;
        for (auto const &it : k) {
          auto [v, angle, line] = it;
          float b = B * v + A;
          line.line = cv::Vec4f(cos(angle - offset), sin(angle - offset), -sin(-offset) * b, cos(-offset) * b);
          result[v] = line;
        }
        for (int v = vmin - extra; v <= vmax + extra; v++) {
          if (result.find(v) != result.end()) {
            continue;
          }
          auto angle = YFromX(*fit, v);
          if (!angle) {
            continue;
          }
          Line line;
          float b = B * v + A;
          line.line = cv::Vec4f(cos(*angle - offset), sin(*angle - offset), -sin(-offset) * b, cos(-offset) * b);
          result[v] = line;
        }
        result.swap(lines);
      };
      static auto Equalize_ = [](map<int, Line> &lines, int extra) {
        // まず角度の平均値を求める. 角度が 0 度をまたぐと傾きの平滑化ができなくなるので, まず平滑化の前に平均を求める.
        RadianAverage ra;
        vector<cv::Point2f> angles;
        int vmin = numeric_limits<int>::max();
        int vmax = numeric_limits<int>::min();
        for (auto const &it : lines) {
          vmin = std::min(vmin, it.first);
          vmax = std::max(vmax, it.first);
          float v = it.first;
          float angle = atan2(it.second.line[1], it.second.line[0]);
          while (angle < 0) {
            angle += pi * 2;
          }
          while (angle > pi * 2) {
            angle -= pi * 2;
          }
          if (angle > pi) {
            angle -= pi;
          }
          ra.push(angle);
          angles.push_back(cv::Point2f(v, angle));
        }
        float mean = ra.get();
        float offset = pi - mean;
        float minAngle = numeric_limits<float>::max();
        float maxAngle = numeric_limits<float>::lowest();
        for (size_t i = 0; i < angles.size(); i++) {
          float angle = angles[i].y + offset;
          while (angle < 0) {
            angle += pi * 2;
          }
          while (angle > pi * 2) {
            angle -= pi * 2;
          }
          minAngle = std::min(minAngle, float(angle * 180 / pi));
          maxAngle = std::max(maxAngle, float(angle * 180 / pi));
          angles[i].y = angle;
        }
        if (maxAngle - minAngle >= 30) {
          return;
        }
        auto fit = FitLine(angles);
        if (!fit) {
          return;
        }
        map<int, Line> result;
        vector<tuple<int, float, Line>> k;
        // 消失点をフィッティング対象にする.
        // https://gyazo.com/ea4c192304cfdca85e0168cadc4c7827
        float r = 0;
        float s = 0;
        float u = 0;
        float w = 0;
        float q = 0;
        for (auto const &it : lines) {
          float v = it.first;
          Line line = it.second;
          auto angle = YFromX(*fit, it.first);
          if (!angle) {
            result[it.first] = line;
            continue;
          }
          // offset 回転させているので, *angle はだいたい 180 度付近になっている.
          float m = tan(*angle);
          if (!(m == 0 || isnormal(m))) {
            result[it.first] = line;
            continue;
          }
          float n = line.points.size();
          for (auto const &p : line.points) {
            float x = cos(offset) * p.x - sin(offset) * p.y;
            float y = sin(offset) * p.x + cos(offset) * p.y;
            u += y - m * x;
            w += (y - m * x) * m;
          }
          q += m * m * n;
          r += n;
          s += m * n;
          k.push_back(make_tuple(it.first, *angle, line));
        }
        float ax = (s * u / r - w) / (q - s * s / r);
        float ay = (u + s * ax) / r;
        for (auto const &it : k) {
          auto [v, angle, line] = it;
          line.line = cv::Vec4f(cos(angle - offset), sin(angle - offset), cos(-offset) * ax - sin(-offset) * ay, sin(-offset) * ax + cos(-offset) * ay);
          result[v] = line;
        }
        for (int v = vmin - extra; v <= vmax + extra; v++) {
          if (result.find(v) != result.end()) {
            continue;
          }
          auto angle = YFromX(*fit, v);
          if (!angle) {
            continue;
          }
          Line line;
          line.line = cv::Vec4f(cos(*angle - offset), sin(*angle - offset), cos(-offset) * ax - sin(-offset) * ay, sin(-offset) * ax + cos(-offset) * ay);
          result[v] = line;
        }
        result.swap(lines);
      };
      Equalize(hlines, dy);
      Equalize(vlines, dx);
      // フィッティングした直線を元に足りていない点を交点を計算することで補う
      for (int x = minX - dx; x <= maxX + dx; x++) {
        auto found = vlines.find(x);
        if (found == vlines.end()) {
          continue;
        }
        Line vline = found->second;
        for (int y = minY - dy; y <= maxY + dy; y++) {
          if (grids.find(make_pair(x, y)) != grids.end()) {
            continue;
          }
          auto hline = hlines.find(y);
          if (hline == hlines.end()) {
            continue;
          }
          if (auto cross = Intersection(vline.line, hline->second.line); cross) {
            grids[make_pair(x, y)] = *cross;
          }
        }
      }

      // 以上の計算を largest に対して最初に 1 回だけ行っても良いかもしれないが, k = 2 以降でマージした結果を流用したいので毎回の k で計算する.

      auto const &cluster = clusters[k];

      bool all;
      optional<int> diffx;
      optional<int> diffy;
      bool rotate;
      for (auto r : {false, true}) {
        all = true;
        for (auto const &i : cluster) {
          auto [x, y] = i.first;
          if (r) {
            x = -x;
            y = -y;
          }
          for (auto const &j : i.second) {
            cv::Point2f center = j->meanCenter();
            optional<pair<int, int>> found;
            for (auto const &grid : grids) {
              cv::Point2f center0 = grid.second;
              if (cv::norm(center - center0) <= distanceTh) {
                found = grid.first;
                break;
              }
            }
            if (!found) {
              continue;
            }
            int tdx = x - found->first;
            int tdy = y - found->second;
            if (diffx && diffy) {
              if (*diffx != tdx || *diffy != tdy) {
                all = false;
                break;
              }
            } else {
              diffx = tdx;
              diffy = tdy;
            }
          }
          if (!all) {
            break;
          }
        }
        if (all && diffx && diffy) {
          rotate = r;
          break;
        }
      }
      if (all && diffx && diffy) {
        // cluster 内の lattice が全て largest 内の lattice のいずれかと中心が一致し, かつ, largest との grid の格子位置のズレが一定だった場合, cluster は largest にマージできる.
        for (auto const &i : cluster) {
          auto [x, y] = i.first;
          if (rotate) {
            x = -x;
            y = -y;
          }
          for (auto const &j : i.second) {
            largest[make_pair(x - *diffx, y - *diffy)].insert(j);
          }
        }
      } else {
        s.clusters.push_back(cluster);
      }
    }
    if (!s.clusters.empty() && !vlines.empty()) {
      deque<map<pair<int, int>, set<shared_ptr<Lattice>>>> clusters;
      map<pair<int, int>, cv::Point2f> tmpGrids;
      s.clusters.swap(clusters);
      grids.swap(tmpGrids);
      // clusters[0] の y 方向の向きが s.boardDirection と一致しているか調べる. 一致していなければ y 方向を反転させる
      double angle = atan2(vlines[0].line[1], vlines[0].line[0]);
      if (cos(angle - s.boardDirection) < 0) {
        bool first = true;
        for (auto const &cluster : clusters) {
          int minX, maxX, minY, maxY;
          minX = minY = numeric_limits<int>::max();
          maxX = maxY = numeric_limits<int>::min();
          for (auto const &i : cluster) {
            auto [x, y] = i.first;
            minX = std::min(minX, x);
            maxX = std::max(maxX, x);
            minY = std::min(minY, y);
            maxY = std::max(maxY, y);
          }
          map<pair<int, int>, set<shared_ptr<Lattice>>> c;
          for (auto const &i : cluster) {
            auto [x, y] = i.first;
            c[make_pair(maxX + minX - x, maxY + minY - y)] = i.second;
          }
          s.clusters.push_back(c);
          if (first) {
            for (auto const &it : tmpGrids) {
              auto [x, y] = it.first;
              grids[make_pair(maxX + minX - x, maxY + minY - y)] = it.second;
            }
          }
          first = false;
        }
      }
#if 0
      static int cnt = 0;
      cnt++;
      int index = 0;
      cv::Mat img(s.height * 2, s.width * 2, CV_8UC3, cv::Scalar::all(255));
      for (auto const &it : largest) {
        for (auto const &c : it.second) {
          if (c->content->index() == 0) {
            auto sq = get<0>(*c->content);
            vector<cv::Point> points;
            for (auto p : sq->points) {
              points.push_back(cv::Point(p.x * 2, p.y * 2));
            }
            cv::polylines(img, points, true, cv::Scalar(0, 0, 255));
          } else {
            auto piece = get<1>(*c->content);
            vector<cv::Point> points;
            for (auto p : piece->points) {
              points.push_back(cv::Point(p.x * 2, p.y * 2));
            }
            cv::polylines(img, points, true, cv::Scalar(255, 0, 0));
          }
        }
      }
      for (auto const &it : grids) {
        cv::circle(img, cv::Point2f(it.second.x * 2, it.second.y * 2), distanceTh * 2 / 2, cv::Scalar(0, 0, 255));
      }
      for (auto const &it : vlines) {
        auto y = YFromX(it.second.line, s.width / 2);
        if (!y) {
          continue;
        }
        cv::Point2f center(s.width / 2, *y);
        cv::Point2f dir(it.second.line[0], it.second.line[1]);
        dir = dir / cv::norm(dir);
        cv::Point2f from = center + s.height * 10 * dir;
        cv::Point2f to = center - s.height * 10 * dir;
        vector<cv::Point> points;
        points.push_back(from * 2);
        points.push_back(to * 2);
        cv::polylines(img, points, false, cv::Scalar(255, 0, 0));
      }
      for (auto const &it : hlines) {
        auto y = YFromX(it.second.line, s.width / 2);
        if (!y) {
          continue;
        }
        cv::Point2f center(s.width / 2, *y);
        cv::Point2f dir(it.second.line[0], it.second.line[1]);
        dir = dir / cv::norm(dir);
        cv::Point2f from = center + s.width * 10 * dir;
        cv::Point2f to = center - s.width * 10 * dir;
        vector<cv::Point> points;
        points.push_back(from * 2);
        points.push_back(to * 2);
        cv::polylines(img, points, false, cv::Scalar(0, 255, 0));
      }
      cout << "b64png(grids_" << cv::format("%04d", cnt) << "):" << base64::to_base64(Img::EncodeToPng(img)) << endl;
#endif
    }
#if 0
    static int cnt = 0;
    cnt++;
    int index = 0;
    for (auto const &cluster : s.clusters) {
      index++;
      cv::Mat all(s.height * 2, s.width * 2, CV_8UC3, cv::Scalar::all(0));
      for (auto const &it : cluster) {
        for (auto const &c : it.second) {
          if (c->content->index() == 0) {
            auto sq = get<0>(*c->content);
            vector<cv::Point> points;
            for (auto p : sq->points) {
              points.push_back(cv::Point(p.x * 2, p.y * 2));
            }
            cv::fillPoly(all, points, cv::Scalar(0, 0, 255));
          } else {
            auto piece = get<1>(*c->content);
            vector<cv::Point> points;
            for (auto p : piece->points) {
              points.push_back(cv::Point(p.x * 2, p.y * 2));
            }
            cv::fillPoly(all, points, cv::Scalar(255, 0, 0));
          }
        }
      }
      for (auto const &it : cluster) {
        auto [x, y] = it.first;
        for (auto const &c : it.second) {
          auto center = CenterFromLatticeContent(*c->content);
          cv::putText(all, to_string(x) + "," + to_string(y), cv::Point(center.x * 2 - sqrt(s.squareArea) / 2, center.y * 2), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar::all(255));
          break;
        }
      }
      cout << "b64png(sample_" << cv::format("%04d", cnt) << "_" << index << "):" << base64::to_base64(Img::EncodeToPng(all)) << endl;
    }
#endif
  }
  if (!s.clusters.empty()) {
    auto const &largest = s.clusters[0];
    int minX, maxX, minY, maxY;
    minX = minY = numeric_limits<int>::max();
    maxX = maxY = numeric_limits<int>::min();
    for (auto const &i : largest) {
      auto [x, y] = i.first;
      minX = std::min(minX, x);
      maxX = std::max(maxX, x);
      minY = std::min(minY, y);
      maxY = std::max(maxY, y);
    }
    if (maxX - minX == 8 && maxY - minY == 8) {
      for (auto const &i : largest) {
        auto [x, y] = i.first;
        int file = x - minX;
        int rank = y - minY;
        std::shared_ptr<Contour> square;
        std::shared_ptr<PieceContour> piece;
        for (auto const &j : i.second) {
          if (j->content->index() == 0) {
            if (!square) {
              square = get<0>(*j->content);
            }
          } else {
            if (!piece) {
              piece = get<1>(*j->content);
              break;
            }
          }
          if (piece) {
            break;
          }
          for (auto const &k : j->center) {
            if (k->index() == 0) {
              if (!square) {
                square = get<0>(*k);
              }
            } else {
              if (!piece) {
                piece = get<1>(*k);
                break;
              }
            }
          }
          if (piece) {
            break;
          }
        }
        if (piece) {
          s.detected[file][rank] = make_shared<Contour>(piece->toContour());
        } else if (square) {
          s.detected[file][rank] = square;
        }
      }
      auto top = hlines.find(0);
      auto bottom = hlines.find(8);
      auto left = vlines.find(0);
      auto right = vlines.find(8);
      if (top != hlines.end() && bottom != hlines.end() && left != vlines.end() && right != vlines.end()) {
        auto tl = Intersection(top->second.line, left->second.line);
        auto tr = Intersection(top->second.line, right->second.line);
        auto br = Intersection(bottom->second.line, right->second.line);
        auto bl = Intersection(bottom->second.line, left->second.line);
        if (tl && tr && br && bl) {
          // tl, tr, br, bl は駒中心を元に計算しているので 8x8 の範囲しか無い. 半マス分増やす
          cv::Point2f topLeft = (*tl) + (((*tl) - (*tr)) / 16) + (((*tl) - (*bl)) / 16);
          cv::Point2f topRight = (*tr) + (((*tr) - (*tl)) / 16) + (((*tr) - (*br)) / 16);
          cv::Point2f bottomRight = (*br) + (((*br) - (*bl)) / 16) + (((*br) - (*tr)) / 16);
          cv::Point2f bottomLeft = (*bl) + (((*bl) - (*br)) / 16) + (((*bl) - (*tl)) / 16);

          stat.outlineTL.push_back(topLeft);
          stat.outlineTR.push_back(topRight);
          stat.outlineBR.push_back(bottomRight);
          stat.outlineBL.push_back(bottomLeft);
          if (stat.outlineTL.size() > Statistics::kOutlineMaxCount) {
            stat.outlineTL.pop_front();
          }
          if (stat.outlineTR.size() > Statistics::kOutlineMaxCount) {
            stat.outlineTR.pop_front();
          }
          if (stat.outlineBR.size() > Statistics::kOutlineMaxCount) {
            stat.outlineBR.pop_front();
          }
          if (stat.outlineBL.size() > Statistics::kOutlineMaxCount) {
            stat.outlineBL.pop_front();
          }
        }
      }
    }
  }
  if (!stat.outlineTL.empty() && !stat.outlineTR.empty() && !stat.outlineBR.empty() && !stat.outlineBL.empty()) {
    cv::Point2f topLeft(0, 0);
    cv::Point2f topRight(0, 0);
    cv::Point2f bottomRight(0, 0);
    cv::Point2f bottomLeft(0, 0);
    for (auto const &p : stat.outlineTL) {
      topLeft += p;
    }
    topLeft = topLeft / float(stat.outlineTL.size());
    for (auto const &p : stat.outlineTR) {
      topRight += p;
    }
    topRight = topRight / float(stat.outlineTR.size());
    for (auto const &p : stat.outlineBR) {
      bottomRight += p;
    }
    bottomRight = bottomRight / float(stat.outlineBR.size());
    for (auto const &p : stat.outlineBL) {
      bottomLeft += p;
    }
    bottomLeft = bottomLeft / float(stat.outlineBL.size());

    Contour preciseOutline;
    preciseOutline.points = {topLeft, topRight, bottomRight, bottomLeft};
    preciseOutline.area = fabs(cv::contourArea(preciseOutline.points));
    s.preciseOutline = stat.preciseOutline = preciseOutline;

    cv::Point2f midBottom = (bottomRight + bottomLeft) * 0.5f;
    cv::Point2f midTop = (topRight + topLeft) * 0.5f;
    s.boardDirection = Angle(midBottom - midTop);
  }
}

void CreateWarpedBoard(cv::Mat const &frame, Status &s, Statistics const &stat) {
  using namespace std;
  optional<Contour> preciseOutline = stat.preciseOutline;
  if (!stat.preciseOutline || !stat.aspectRatio || !stat.squareArea) {
    return;
  }
  // 台形補正. キャプチャ画像と同じ面積で, アスペクト比が Status.aspectRatio と等しいサイズとなるような台形補正済み画像を作る.
  double area = *stat.squareArea * 81;
  if (area > frame.size().area()) {
    // キャプチャ画像より盤面が広いことはありえない.
    return;
  }
  // a = w/h, w = a * h
  // area = w * h = a * h^2, h = sqrt(area/a), w = sqrt(area * a)
  int width = (int)round(sqrt(area * (*stat.aspectRatio)));
  int height = (int)round(sqrt(area / (*stat.aspectRatio)));
  vector<cv::Point2f> dst({
      cv::Point2f(0, 0),
      cv::Point2f(width, 0),
      cv::Point2f(width, height),
      cv::Point2f(0, height),
  });
  cv::Mat mtx = cv::getPerspectiveTransform(stat.preciseOutline->points, dst);
  s.perspectiveTransform = mtx;
  s.rotate = stat.rotate;
  s.warpedWidth = width;
  s.warpedHeight = height;
  if (stat.rotate) {
    cv::Mat tmp1;
    cv::warpPerspective(frame, tmp1, mtx, cv::Size(width, height));
    cv::rotate(tmp1, s.boardWarped, cv::ROTATE_180);
  } else {
    cv::warpPerspective(frame, s.boardWarped, mtx, cv::Size(width, height));
  }
}

} // namespace

Session::Session() {
  s = std::make_shared<Status>();
  stop = false;
  std::thread th(std::bind(&Session::run, this));
  this->th.swap(th);
}

Session::~Session() {
  stop = true;
  cv.notify_one();
  th.join();
}

void Session::run() {
  using namespace std;
  while (!stop) {
    std::unique_lock<std::mutex> lock(mut);
    cv.wait(lock, [this]() { return !queue.empty() || stop; });
    if (stop) {
      lock.unlock();
      break;
    }
    cv::Mat frame = queue.front();
    queue.pop_front();
    lock.unlock();

    cv::cvtColor(frame, frame, cv::COLOR_RGB2GRAY);

    auto s = std::make_shared<Status>();
    s->stableBoardMaxSimilarity = BoardImage::kStableBoardMaxSimilarity;
    s->stableBoardThreshold = BoardImage::kStableBoardThreshold;
    s->blackResign = this->s->blackResign;
    s->whiteResign = this->s->whiteResign;
    s->boardReady = this->s->boardReady;
    s->wrongMove = this->s->wrongMove;
    s->width = frame.size().width;
    s->height = frame.size().height;
    Img::FindContours(frame, s->contours, s->squares, s->pieces);
    FindBoard(frame, *s, stat);
    stat.update(*s);
    s->book = stat.book;
    CreateWarpedBoard(frame, *s, stat);
    if (auto prePlayers = this->prePlayers; !players && prePlayers) {
      if (prePlayers->black) {
        if (auto move = prePlayers->black->next(game.position, game.moves, game.handBlack, game.handWhite); move) {
          move->decideSuffix(game.position);
          game.moves.push_back(*move);
        }
      }
      players = prePlayers;
      this->prePlayers = nullptr;
    }
    stat.push(s->boardWarped, *s, game, detected, players != nullptr);
    if (players && detected.size() == game.moves.size()) {
      if (game.moves.size() % 2 == 0) {
        // 次が先手番
        if (players->black && !s->blackResign) {
          auto move = players->black->next(game.position, game.moves, game.handBlack, game.handWhite);
          if (move) {
            move->decideSuffix(game.position);
            optional<Square> lastTo;
            if (game.moves.size() > 0) {
              lastTo = game.moves.back().to;
            }
            game.moves.push_back(*move);
          } else {
            s->blackResign = true;
            cout << "先手番AIが投了" << endl;
          }
        }
      } else {
        // 次が後手番
        if (players->white && !s->whiteResign) {
          auto move = players->white->next(game.position, game.moves, game.handWhite, game.handBlack);
          if (move) {
            move->decideSuffix(game.position);
            optional<Square> lastTo;
            if (game.moves.size() > 0) {
              lastTo = game.moves.back().to;
            }
            game.moves.push_back(*move);
          } else {
            s->whiteResign = true;
            cout << "後手番AIが投了" << endl;
          }
        }
      }
    }
    s->waitingMove = detected.size() != game.moves.size();
    s->game = game;
    if (!stat.stableBoardHistory.empty()) {
      s->stableBoard = stat.stableBoardHistory.back().back().image;
    }
    this->s = s;
  }
}

void Session::push(cv::Mat const &frame) {
  {
    std::lock_guard<std::mutex> lock(mut);
    queue.clear();
    queue.push_back(frame);
  }
  cv.notify_all();
}

void Session::resign(Color color) {
  if (color == Color::Black) {
    s->blackResign = true;
    std::cout << "先手番が投了" << std::endl;
  } else {
    s->whiteResign = true;
    std::cout << "後手番が投了" << std::endl;
  }
}

} // namespace sci
