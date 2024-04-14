#include <shogi_camera/shogi_camera.hpp>

#include "base64.hpp"
#include <opencv2/imgproc.hpp>

#include <format>
#include <iostream>
#include <limits>
#include <numbers>

using namespace std;

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

optional<float> MeanAngle(initializer_list<float> values) {
  RadianAverage ra;
  for (float v : values) {
    ra.push(v);
  }
  return ra.get();
}

optional<float> SquareDirection(vector<cv::Point2f> const &points) {
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

// 直線 line と点 p との距離を計算する. line は [vx, vy, x0, y0] 形式(cv::fitLine の結果の型と同じ形式)
double Distance(cv::Vec4f const &line, cv::Point2d const &p) {
  // line と直行する直線
  cv::Vec4f c(line[1], line[0], p.x, p.y);
  if (auto i = Intersection(line, c); i) {
    return cv::norm(*i - p);
  } else {
    return numeric_limits<double>::infinity();
  }
}

optional<cv::Vec4f> FitLine(vector<cv::Point2f> const &points) {
  if (points.size() < 2) {
    return nullopt;
  }
  cv::Vec4f line;
  cv::fitLine(cv::Mat(points), line, cv::DIST_L2, 0, 0.01, 0.01);
  return line;
}

optional<cv::Point2f> PerpendicularFoot(cv::Vec4f const &line, cv::Point2f const &point) {
  // point を通り line に直行する直線
  cv::Vec4f perpendicular(line[1], line[0], point.x, point.y);
  return Intersection(line, perpendicular);
}

optional<float> YFromX(cv::Vec4f const &line, float x) {
  // p = b + t * a
  cv::Point2f a(line[0], line[1]);
  cv::Point2f b(line[2], line[3]);
  // px = bx + t * ax
  // t = (px - bx) / ax
  float t = (x - b.x) / a.x;
  if (t == 0 || isnormal(t)) {
    return b.y + t * a.y;
  } else {
    return nullopt;
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
    for (shared_ptr<Lattice> const &lattice : lattices) {
      for (shared_ptr<Lattice> const &other : lattices) {
        if (lattice == other) {
          continue;
        }
        if (lattice->adjacent.contains(other)) {
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
          for (auto it = l->adjacent.begin(); it != l->adjacent.end(); it++) {
            auto adj = it->lock();
            if (!adj) {
              it = l->adjacent.erase(it);
              continue;
            }
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
            for (auto k = lattice->adjacent.begin(); k != lattice->adjacent.end(); k++) {
              auto adj = k->lock();
              if (!adj) {
                k = lattice->adjacent.erase(k);
                continue;
              }
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
  struct Lines {
    void clear() {
      lines.clear();
    }
    void insert(int k, Line line) {
      lines[k] = line;
    }
    optional<Line> find(int k) const {
      if (auto found = lines.find(k); found != lines.end()) {
        return found->second;
      } else {
        return nullopt;
      }
    }
    void rotate(int xOffset) {
      map<int, Line> backup;
      backup.swap(lines);
      for (auto &it : backup) {
        int x = xOffset - it.first;
        it.second.line[0] = -it.second.line[0];
        it.second.line[1] = -it.second.line[1];
        lines[x] = it.second;
      }
      if (fit) {
        fit->xOffset = xOffset;
      }
    }
    bool empty() const {
      return lines.empty();
    }
    double direction() {
      assert(!lines.empty());
      Line l = lines.begin()->second;
      return atan2(l.line[1], l.line[0]);
    }
    void each(function<void(int k, Line const &line)> cb) const {
      for (auto const &it : lines) {
        cb(it.first, it.second);
      }
    }
    void equalize(int extra) {
      // まず角度の平均値を求める. 角度が 0 度をまたぐと傾きの角度を直線近似できなくなるので, 180 度付近で近似したい. そのためにまず平均を求める.
      RadianAverage ra;
      vector<cv::Point2f> angles;
      int vmin = numeric_limits<int>::max();
      int vmax = numeric_limits<int>::min();
      for (auto const &it : lines) {
        vmin = std::min(vmin, it.first);
        vmax = std::max(vmax, it.first);
        float v = it.first;
        float angle = atan2(it.second.line[1], it.second.line[0]);
        auto m = ra.get();
        if (m && cos(*m - angle) < 0) {
          angle -= pi;
        }
        while (angle < 0) {
          angle += pi * 2;
        }
        while (angle > pi * 2) {
          angle -= pi * 2;
        }
        ra.push(angle);
        angles.push_back(cv::Point2f(v, angle));
      }
      auto mean = ra.get();
      if (!mean) {
        return;
      }
      float offset = pi - *mean;
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
      // https://gyazo.com/e5841fbf59652da39d2e19760e450461
      vector<tuple<int, float, Line>> k;
      float beta = 0;
      float gamma = 0;
      float f = 0;
      float e = 0;
      int N = (int)lines.size();
      for (auto const &it : lines) {
        float v = it.first;
        Line line = it.second;
        auto angle = YFromX(*fit, v);
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
        float gamma_v = 0;
        float f_v = 0;
        float n = line.points.size();
        for (auto const &p : line.points) {
          float x = cos(offset) * p.x - sin(offset) * p.y;
          float y = sin(offset) * p.x + cos(offset) * p.y;
          gamma_v += y - m * x;
          f_v += (y - m * x) * v;
        }
        gamma += gamma_v / n;
        f += f_v / n;
        beta += v;
        e += v * v;
        k.push_back(make_tuple(it.first, *angle, line));
      }
      float B = (f - beta * gamma / N) / (e - beta * beta / N);
      float A = (gamma - beta * B) / N;
      Fit info;
      info.slope = *fit;
      info.slopeOffset = offset;
      info.A = A;
      info.B = B;
      this->fit = info;
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
        auto l = fitted(v);
        if (!l) {
          continue;
        }
        Line line;
        line.line = *l;
        result[v] = line;
      }
      result.swap(lines);
    }
    optional<cv::Vec4f> fitted(float k) const {
      if (!fit) {
        return nullopt;
      }
      float v;
      if (fit->xOffset) {
        v = *fit->xOffset - k;
      } else {
        v = k;
      }
      auto angle = YFromX(fit->slope, v);
      if (!angle) {
        return nullopt;
      }
      float b = fit->B * v + fit->A;
      return cv::Vec4f(cos(*angle - fit->slopeOffset), sin(*angle - fit->slopeOffset), -sin(-fit->slopeOffset) * b, cos(-fit->slopeOffset) * b);
    }

    struct Fit {
      cv::Vec4f slope;
      double slopeOffset;
      float B;
      float A;
      optional<int> xOffset;
    };
    optional<Fit> fit;
    map<int, Line> lines;
  };
  Lines vlines;
  Lines hlines;
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
          vlines.insert(x, l);
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
          hlines.insert(y, l);
        }
      }
      // フィッティングした直線の傾きを平滑化する
      hlines.equalize(dy);
      vlines.equalize(dx);
      // フィッティングした直線を元に足りていない点を交点を計算することで補う
      for (int x = minX - dx; x <= maxX + dx; x++) {
        auto vline = vlines.find(x);
        if (!vline) {
          continue;
        }
        for (int y = minY - dy; y <= maxY + dy; y++) {
          if (grids.find(make_pair(x, y)) != grids.end()) {
            continue;
          }
          auto hline = hlines.find(y);
          if (!hline) {
            continue;
          }
          if (auto cross = Intersection(vline->line, hline->line); cross) {
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
      double angle = vlines.direction();
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
            vlines.rotate(maxX + minX);
            hlines.rotate(maxY + minY);
          }
          first = false;
        }
      } else {
        s.clusters.swap(clusters);
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
      vlines.each([&](int k, Line const &line) {
        auto y0 = YFromX(line.line, 0);
        if (!y0) {
          return;
        }
        auto y1 = YFromX(line.line, s.width);
        if (!y1) {
          return;
        }
        vector<cv::Point> points;
        points.push_back(cv::Point2f(0, (*y0) * 2));
        points.push_back(cv::Point2f(s.width * 2, (*y1) * 2));
        cv::polylines(img, points, false, cv::Scalar(255, 0, 0));
      });
      hlines.each([&](int k, Line const &line) {
        auto y0 = YFromX(line.line, 0);
        if (!y0) {
          return;
        }
        auto y1 = YFromX(line.line, s.width);
        if (!y1) {
          return;
        }
        vector<cv::Point> points;
        points.push_back(cv::Point2f(0, (*y0) * 2));
        points.push_back(cv::Point2f(s.width * 2, (*y1) * 2));
        cv::polylines(img, points, false, cv::Scalar(0, 255, 0));
      });
      LogPng(img) << "grids_" << cv::format("%04d", cnt);
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
      LogPng(all) << "sample_" << cv::format("%04d", cnt) << "_" << index;
    }
#endif
  }
  {
    cv::Vec4f top(1, 0, 0, 0);
    cv::Vec4f bottom(1, 0, 0, s.height);
    cv::Vec4f left(0, 1, 0, 0);
    cv::Vec4f right(0, 1, s.width, 0);
    for (auto const &it : hlines.lines) {
      cv::Vec4f const &line = it.second.line;
      auto t = Intersection(line, top);
      auto b = Intersection(line, bottom);
      if (t && b) {
        s.hlines.push_back(make_pair(*t, *b));
        continue;
      }
      auto l = Intersection(line, left);
      auto r = Intersection(line, right);
      if (l && r) {
        s.hlines.push_back(make_pair(*l, *r));
      }
    }
    for (auto const &it : vlines.lines) {
      cv::Vec4f const &line = it.second.line;
      auto t = Intersection(line, top);
      auto b = Intersection(line, bottom);
      if (t && b) {
        s.vlines.push_back(make_pair(*t, *b));
        continue;
      }
      auto l = Intersection(line, left);
      auto r = Intersection(line, right);
      if (l && r) {
        s.vlines.push_back(make_pair(*l, *r));
      }
    }
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
        shared_ptr<Contour> square;
        shared_ptr<PieceContour> piece;
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
      auto top = hlines.fitted(-0.5f);
      auto bottom = hlines.fitted(8.5f);
      auto left = vlines.fitted(-0.5f);
      auto right = vlines.fitted(8.5f);
      if (top && bottom && left && right && hlines.find(0) && hlines.find(8) && vlines.find(0) && vlines.find(8)) {
        auto topLeft = Intersection(*top, *left);
        auto topRight = Intersection(*top, *right);
        auto bottomRight = Intersection(*bottom, *right);
        auto bottomLeft = Intersection(*bottom, *left);
        cv::Rect bounds(0, 0, frame.size().width, frame.size().height);
        if (topLeft && bounds.contains(*topLeft) &&
            topRight && bounds.contains(*topRight) &&
            bottomRight && bounds.contains(*bottomRight) &&
            bottomLeft && bounds.contains(*bottomLeft)) {
          cv::Point2f midBottom = (*bottomRight + *bottomLeft) * 0.5f;
          cv::Point2f midTop = (*topRight + *topLeft) * 0.5f;
          double direction = Angle(midBottom - midTop);
          // boardDirection との違いが 30 度以上(前後に15度ずつ)の場合は無視する
          double cosDiffThreshold = cos(15.0 / 180.0 * pi);
          double cosDiffDirection = cos(direction - s.boardDirection);
          int maxCount = s.started ? Statistics::kOutlineMaxCountDuringGame : Statistics::kOutlineMaxCount;
          int count = (int)std::min({stat.outlineTL.size(), stat.outlineTR.size(), stat.outlineBL.size(), stat.outlineBR.size()});
          if (count < maxCount || abs(cosDiffDirection) > cosDiffThreshold) {
            if (cosDiffDirection < 0) {
              // 前回の boardDirection から 90 度以上違っていたら反転させる
              swap(topLeft, bottomRight);
              swap(topRight, bottomLeft);
            }
            s.corners.push_back(*topLeft);
            s.corners.push_back(*topRight);
            s.corners.push_back(*bottomRight);
            s.corners.push_back(*bottomLeft);

            bool ok = true;
            if (stat.squareArea && stat.outlineTL.size() == maxCount) {
              // 既存の outline と比較してかけ離れた位置の場合は無視する
              cv::Point2f sumTL(0, 0);
              cv::Point2f sumTR(0, 0);
              cv::Point2f sumBR(0, 0);
              cv::Point2f sumBL(0, 0);
              for (auto const &p : stat.outlineTL) {
                sumTL += p;
              }
              sumTL = sumTL / float(stat.outlineTL.size());
              for (auto const &p : stat.outlineTR) {
                sumTR += p;
              }
              sumTR = sumTR / float(stat.outlineTR.size());
              for (auto const &p : stat.outlineBR) {
                sumBR += p;
              }
              sumBR = sumBR / float(stat.outlineBR.size());
              for (auto const &p : stat.outlineBL) {
                sumBL += p;
              }
              sumBL = sumBL / float(stat.outlineBL.size());
              double threshold = sqrt(*stat.squareArea) * 0.3;
              if (cv::norm(sumTL - cv::Point2f(*topLeft)) >= threshold ||
                  cv::norm(sumTR - cv::Point2f(*topRight)) >= threshold ||
                  cv::norm(sumBR - cv::Point2f(*bottomRight)) >= threshold ||
                  cv::norm(sumBL - cv::Point2f(*bottomLeft)) >= threshold) {
                ok = false;
              }
            }
            if (ok) {
              stat.outlineTL.push_back(*topLeft);
              stat.outlineTR.push_back(*topRight);
              stat.outlineBR.push_back(*bottomRight);
              stat.outlineBL.push_back(*bottomLeft);
              if (stat.outlineTL.size() > maxCount) {
                stat.outlineTL.pop_front();
              }
              if (stat.outlineTR.size() > maxCount) {
                stat.outlineTR.pop_front();
              }
              if (stat.outlineBR.size() > maxCount) {
                stat.outlineBR.pop_front();
              }
              if (stat.outlineBL.size() > maxCount) {
                stat.outlineBL.pop_front();
              }
            }
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
    s.preciseOutline = preciseOutline;

    cv::Point2f midBottom = (bottomRight + bottomLeft) * 0.5f;
    cv::Point2f midTop = (topRight + topLeft) * 0.5f;
    s.boardDirection = Angle(midBottom - midTop);
  }
}

void CreateWarpedBoard(cv::Mat const &frameGray, cv::Mat const &frameColor, Status &s, Statistics const &stat) {
  optional<Contour> preciseOutline = s.preciseOutline;
  if (!preciseOutline || !stat.aspectRatio) {
    return;
  }
  float aspectRatio = *stat.aspectRatio;
  int width = (int)round(sqrt(Statistics::kBoardArea * aspectRatio));
  int height = (int)round(sqrt(Statistics::kBoardArea / aspectRatio));
  vector<cv::Point2f> dst({
      cv::Point2f(0, 0),
      cv::Point2f(width, 0),
      cv::Point2f(width, height),
      cv::Point2f(0, height),
  });
  cv::Mat mtx = cv::getPerspectiveTransform(preciseOutline->points, dst);
  s.perspectiveTransform = mtx;
  s.rotate = stat.rotate;
  s.warpedWidth = width;
  s.warpedHeight = height;
  if (stat.rotate) {
    cv::Mat tmp1;
    cv::warpPerspective(frameGray, tmp1, mtx, cv::Size(width, height));
    cv::rotate(tmp1, s.boardWarpedGray, cv::ROTATE_180);

    cv::warpPerspective(frameColor, tmp1, mtx, cv::Size(width, height));
    cv::rotate(tmp1, s.boardWarpedColor, cv::ROTATE_180);
  } else {
    cv::warpPerspective(frameGray, s.boardWarpedGray, mtx, cv::Size(width, height));
    cv::warpPerspective(frameColor, s.boardWarpedColor, mtx, cv::Size(width, height));
  }
}

} // namespace

Session::Session() : game(Handicap::平手, false) {
  s = make_shared<Status>();
  stop = false;
  thread runThread(std::bind(&Session::run, this));
  this->runThread.swap(runThread);
  thread playerThread(std::bind(&Session::runPlayer, this));
  this->playerThread.swap(playerThread);
}

Session::~Session() {
  stop = true;
  if (players) {
    if (players->black) {
      players->black->stop();
    }
    if (players->white) {
      players->white->stop();
    }
  }
  runThreadCv.notify_all();
  runThread.join();
  playerThreadCv.notify_all();
  playerThread.join();
}

void Session::run() {
  while (!stop) {
    unique_lock<mutex> lock(mut);
    runThreadCv.wait(lock, [this]() { return !queue.empty() || stop; });
    if (stop) {
      lock.unlock();
      break;
    }
    cv::Mat frameColor = queue.front();
    queue.pop_front();

    auto s = make_shared<Status>();
    s->result = this->s->result;
    s->boardReady = this->s->boardReady;
    s->wrongMove = this->s->wrongMove;
    s->boardDirection = this->s->boardDirection;
    s->yourTurnFirst = this->s->yourTurnFirst;
    s->aborted = this->s->aborted;
    s->game = this->game;
    s->started = this->started;
    s->handicapReady = this->s->handicapReady;
    memcpy(s->similarityAgainstStableBoard, this->s->similarityAgainstStableBoard, sizeof(s->similarityAgainstStableBoard));

    lock.unlock();

    cv::Mat frameGray;
    cv::cvtColor(frameColor, frameGray, cv::COLOR_RGB2GRAY);

    s->width = frameGray.size().width;
    s->height = frameGray.size().height;
    Img::FindContours(frameGray, s->contours, s->squares, s->pieces);
    FindBoard(frameGray, *s, stat);
    stat.update(*s);
    s->book = stat.book;
    CreateWarpedBoard(frameGray, frameColor, *s, stat);
    {
      lock_guard<mutex> lk(mut);
      if (nextFuture) {
        if (nextFuture->wait_for(chrono::milliseconds(100)) == future_status::ready) {
          auto output = nextFuture->get();
          nextFuture.reset();
          if (output.move) {
            output.move->decideSuffix(game.position);
            if (game.moves.empty()) {
              if (game.first != output.move->color) {
                unsafeResign(output.color, *s);
              }
            } else {
              if (game.moves.back().color == output.move->color) {
                unsafeResign(output.color, *s);
              }
            }
            game.moves.push_back(*output.move);
          } else {
            unsafeResign(output.color, *s);
          }
        }
      }
    }
    if (playerConfig) {
      if (holds_alternative<PlayerConfig::Local>(playerConfig->players)) {
        PlayerConfig::Local local = get<0>(playerConfig->players);
        bool notify = false;
        if (local.black && game.first == Color::Black) {
          lock_guard<mutex> lk(mut);
          if (!nextPromise && !nextFuture) {
            nextPromise = make_unique<promise<Output>>();
            nextFuture = make_unique<future<Output>>();
            nextInput = make_unique<Input>(local.black, game.position, Color::Black, game.moves, game.handBlack, game.handWhite);
            auto f = nextPromise->get_future();
            nextFuture->swap(f);
            notify = true;
          }
        } else if (local.white && game.first == Color::White) {
          lock_guard<mutex> lk(mut);
          if (!nextPromise && !nextFuture) {
            nextPromise = make_unique<promise<Output>>();
            nextFuture = make_unique<future<Output>>();
            nextInput = make_unique<Input>(local.white, game.position, Color::White, game.moves, game.handWhite, game.handBlack);
            auto f = nextPromise->get_future();
            nextFuture->swap(f);
            notify = true;
          }
        }
        if (notify) {
          playerThreadCv.notify_all();
        }
        auto p = make_shared<Players>();
        p->black = local.black;
        p->white = local.white;
        players = p;
        playerConfig = nullptr;
      } else if (holds_alternative<PlayerConfig::Remote>(playerConfig->players)) {
        PlayerConfig::Remote remote = get<1>(playerConfig->players);
        if (remote.csa->color_) {
          auto p = make_shared<Players>();
          if (*remote.csa->color_ == Color::Black) {
            p->black = remote.csa;
            p->white = remote.local;
          } else {
            p->black = remote.local;
            p->white = remote.csa;
          }
          players = p;
          playerConfig = nullptr;
          s->yourTurnFirst = game.first == OpponentColor(*remote.csa->color_) ? Ternary::True : Ternary::False;
        }
      }
    }
    auto ret = stat.push(s->boardWarpedGray, s->boardWarpedColor, *s, game, detected, players != nullptr);
    if (players) {
      if (detected.size() == game.moves.size()) {
        if (game.next() == Color::Black) {
          // 次が先手番
          bool notify = false;
          if (players->black && !s->result) {
            lock_guard<mutex> lk(mut);
            if (!nextPromise && !nextFuture) {
              nextPromise = make_unique<promise<Output>>();
              nextFuture = make_unique<future<Output>>();
              nextInput = make_unique<Input>(players->black, game.position, Color::Black, game.moves, game.handBlack, game.handWhite);
              auto f = nextPromise->get_future();
              nextFuture->swap(f);
              notify = true;
            }
          }
          if (notify) {
            playerThreadCv.notify_all();
          }
        } else {
          // 次が後手番
          bool notify = false;
          if (players->white && !s->result) {
            lock_guard<mutex> lk(mut);
            if (!nextPromise && !nextFuture) {
              nextPromise = make_unique<promise<Output>>();
              nextFuture = make_unique<future<Output>>();
              nextInput = make_unique<Input>(players->white, game.position, Color::White, game.moves, game.handWhite, game.handBlack);
              auto f = nextPromise->get_future();
              nextFuture->swap(f);
              notify = true;
            }
          }
          if (notify) {
            playerThreadCv.notify_all();
          }
        }
      }
    }
    s->waitingMove = detected.size() != game.moves.size();
    s->game = game;
    if (!stat.stableBoardHistory.empty()) {
      s->stableBoard = stat.stableBoardHistory.back().back().blurGray;
    }
    s->error = error;
    if (!s->result && ret) {
      s->result = ret;
    }
    this->s = s;
  }
}

void Session::runPlayer() {
  while (!stop) {
    unique_lock<mutex> lock(mut);
    playerThreadCv.wait(lock, [this]() { return (nextInput && nextPromise) || stop; });
    if (stop) {
      lock.unlock();
      break;
    }
    unique_ptr<Input> input;
    input.swap(nextInput);
    unique_ptr<promise<Output>> promise;
    promise.swap(nextPromise);
    lock.unlock();

    Output output;
    output.color = input->color;
    output.move = input->player->next(input->position, input->color, input->moves, input->hand, input->handEnemy);
    promise->set_value(output);
  }
}

void Session::push(cv::Mat const &frame) {
  {
    lock_guard<mutex> lock(mut);
    queue.clear();
    queue.push_back(frame);
  }
  runThreadCv.notify_all();
}

void Session::resign(Color color) {
  lock_guard<mutex> lock(mut);
  unsafeResign(color, *s);
}

void Session::unsafeResign(Color color, Status &s) {
  if (color == Color::Black) {
    if (!s.result) {
      Status::Result r;
      r.result = GameResult::WhiteWin;
      r.reason = GameResultReason::Resign;
      s.result = r;
    }
    cout << "先手番が投了" << endl;
  } else {
    if (!s.result) {
      Status::Result r;
      r.result = GameResult::BlackWin;
      r.reason = GameResultReason::Resign;
      s.result = r;
    }
    cout << "後手番が投了" << endl;
  }
  if (auto csa = dynamic_pointer_cast<CsaAdapter>(players->black); csa) {
    csa->resign(color);
  }
  if (auto csa = dynamic_pointer_cast<CsaAdapter>(players->white); csa) {
    csa->resign(color);
  }
}

void Session::setHandicap(Handicap h, bool handicapHand) {
  lock_guard<mutex> lk(mut);
  game = Game(h, handicapHand);
}

void Session::startGame(GameStartParameter p) {
  auto config = make_shared<PlayerConfig>();

  lock_guard<mutex> lk(mut);

  if (p.server) {
    weak_ptr<CsaServer> server = p.server;
    auto csa = make_shared<CsaAdapter>(server);
    csa->delegate = weak_from_this();
    auto writer = p.server->setLocalPeer(csa, p.userColor, game.handicap_, game.handicapHand_);
    csa->setWriter(writer);
    PlayerConfig::Remote remote;
    remote.csa = csa;
    config->players = remote;
  } else {
    PlayerConfig::Local local;
    shared_ptr<Player> ai;
    int aiLevel = p.option;
#if SHOGI_CAMERA_DEBUG
    if (aiLevel > 0) {
      ai = make_shared<Sunfish3AI>();
    } else {
      ai = make_shared<RandomAI>();
    }
#else
    ai = make_shared<RandomAI>();
#endif
    if (p.userColor == Color::White) {
      local.black = ai;
    } else {
      local.white = ai;
    }
    config->players = local;
  }
  setPlayerConfig(config);
  started = true;
  s->started = true;
}

void Session::stopGame() {
  if (!players) {
    return;
  }
  if (auto csa = dynamic_pointer_cast<CsaAdapter>(players->black); csa) {
    csa->send("%CHUDAN");
    csa->send("LOGOUT");
  }
  if (auto csa = dynamic_pointer_cast<CsaAdapter>(players->white); csa) {
    csa->send("%CHUDAN");
    csa->send("LOGOUT");
  }
}

optional<u8string> Session::name(Color color) {
  if (!players) {
    return nullopt;
  }
  if (color == Color::Black) {
    if (players->black) {
      return players->black->name();
    }
  } else {
    if (players->white) {
      return players->white->name();
    }
  }
  return nullopt;
}

void Session::csaAdapterDidGetError(u8string const &what) {
  if (error.empty()) {
    error = what;
  }
}

void Session::csaAdapterDidFinishGame(GameResult result, GameResultReason reason) {
  if (!s->result) {
    Status::Result r;
    r.result = result;
    r.reason = reason;
    s->result = r;
  }
  stop = true;
  runThreadCv.notify_all();
  playerThreadCv.notify_all();
}

} // namespace sci
