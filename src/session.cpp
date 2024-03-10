#include <shogi_camera/shogi_camera.hpp>

#include "base64.hpp"
#include <opencv2/imgproc.hpp>

#include <iostream>
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
  cv::Point2f b(line[0], line[1]);
  cv::Point2f a(line[2], line[3]);
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

void FindBoard(cv::Mat const &frame, Status &s) {
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

#if 1
  // 同じ点と判定するための閾値. この距離より近ければ同じ点と見做す.
  float const distanceTh = sqrt(s.squareArea) * 0.3;
  {
    for (auto const &square : s.squares) {
      bool found = false;
      cv::Point2f center = square->mean();
      for (auto &lattice : s.lattices) {
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
        s.lattices.push_back(l);
      }
    }
    for (auto const &piece : s.pieces) {
      bool found = false;
      cv::Point2f center = piece->center();
      for (auto &lattice : s.lattices) {
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
        s.lattices.push_back(l);
      }
    }
    cout << "total = " << (s.squares.size() + s.pieces.size()) << ", lattices.size() = " << s.lattices.size() << endl;
    float w = sqrt(s.squareArea * s.aspectRatio);
    float h = sqrt(s.squareArea / s.aspectRatio);
    for (auto const &lattice : s.lattices) {
      for (auto const &other : s.lattices) {
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
    set<shared_ptr<Lattice>> lattices;
    for (auto const &l : s.lattices) {
      lattices.insert(l);
    }
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
    cout << "clusters.size()=" << clusters.size() << endl;
    for (auto const &cluster : clusters) {
      cout << "  " << cluster.size() << endl;
    }

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
  if (s.clusters.size() > 1) {
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
      int dx = std::max(9 - (maxX - minX + 1), 1);
      int dy = std::max(9 - (maxY - minY + 1), 1);

      // cluster[k] との比較用に, 各格子の中心座標を計算する.
      map<pair<int, int>, cv::Point2f> grids;
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
      struct Line {
        cv::Vec4f line;
        int min;
        cv::Point2f minCenter;
        int max;
        cv::Point2f maxCenter;
        vector<cv::Point2f> points;
      };
      // 縦方向の既知の格子毎に, 直線をフィッティングする.
      map<int, Line> vlines;
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
          l.min = top->first;
          l.minCenter = top->second;
          l.max = bottom->first;
          l.maxCenter = bottom->second;
          l.points.swap(points);
          vlines[x] = l;
        }
      }
      // 横方向の既知の格子毎に, 直線をフィッティングする.
      map<int, Line> hlines;
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
          l.min = left->first;
          l.minCenter = left->second;
          l.max = right->first;
          l.maxCenter = right->second;
          l.points.swap(points);
          hlines[y] = l;
        }
      }
      //      // フィッティングした直線の傾きを平滑化する
      //      static auto Equalize = [](map<int, Line> &lines) {
      //        vector<cv::Point2f> tangents;
      //        for (auto const& it : lines) {
      //          float v = it.first;
      //          float tangent = it.second.line[1] / it.second.line[0];
      //          tangents.push_back(cv::Point2f(v, tangent));
      //        }
      //        auto fit = FitLine(tangents);
      //        if (!fit) {
      //          return;
      //        }
      //        map<int, Line> result;
      //        for (auto const& it : lines) {
      //          float v = it.first;
      //          Line line = it.second;
      //          auto tangent = YFromX(*fit, v);
      //          if (!tangent) {
      //            result[it.first] = line;
      //            continue;
      //          }
      //          // line.line の傾きを tan(angle) に変更して, 切片を最小二乗法で最適化する.
      //          float m = *tangent;
      //          if (!(m == 0 || isnormal(m))) {
      //            result[it.first] = line;
      //            continue;
      //          }
      //          float sum = 0;
      //          for (auto const& p : line.points) {
      //            sum += p.y - m * p.x;
      //          }
      //          float b = sum / line.points.size();
      //          Line cp = line;
      //          cp.line = cv::Vec4f(1, *tangent, 0, b);
      //          result[it.first] = cp;
      //        }
      //        result.swap(lines);
      //      };
      //      Equalize(vlines);
      //      Equalize(hlines);
      // フィッティングした直線を元に, まず補完によって largest の内側の足りていない点を補う
      // 縦から
      for (int x = minX; x <= maxX; x++) {
        auto found = vlines.find(x);
        if (found == vlines.end()) {
          continue;
        }
        Line vline = found->second;
        for (int y = minY; y <= maxY; y++) {
          if (grids.find(make_pair(x, y)) != grids.end()) {
            // 補間の必要無い.
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
      // 横も
      for (int y = minY; y <= maxY; y++) {
        auto found = hlines.find(y);
        if (found == hlines.end()) {
          continue;
        }
        Line hline = found->second;
        for (int x = minX; x <= maxX; x++) {
          if (grids.find(make_pair(x, y)) != grids.end()) {
            // 補間の必要無い.
            continue;
          }
          auto vline = vlines.find(x);
          if (vline == vlines.end()) {
            continue;
          }
          if (auto cross = Intersection(hline.line, vline->second.line); cross) {
            grids[make_pair(x, y)] = *cross;
          }
        }
      }

      // 補間した点を元に, 足りていない hlines, vlines を補う.

#if 1
      static int cnt = 0;
      cnt++;
      int index = 0;
      cv::Mat all(s.height * 2, s.width * 2, CV_8UC3, cv::Scalar::all(255));
      for (auto const &it : largest) {
        for (auto const &c : it.second) {
          if (c->content->index() == 0) {
            auto sq = get<0>(*c->content);
            vector<cv::Point> points;
            for (auto p : sq->points) {
              points.push_back(cv::Point(p.x * 2, p.y * 2));
            }
            cv::polylines(all, points, true, cv::Scalar(0, 0, 255));
          } else {
            auto piece = get<1>(*c->content);
            vector<cv::Point> points;
            for (auto p : piece->points) {
              points.push_back(cv::Point(p.x * 2, p.y * 2));
            }
            cv::polylines(all, points, true, cv::Scalar(255, 0, 0));
          }
        }
      }
      for (auto const &it : grids) {
        cv::circle(all, cv::Point2f(it.second.x * 2, it.second.y * 2), 5, cv::Scalar(0, 0, 255));
      }
      for (auto const &it : vlines) {
        cv::Point2f center = it.second.minCenter;
        cv::Point2f dir(it.second.line[0], it.second.line[1]);
        dir = dir / cv::norm(dir);
        cv::Point2f from = center + s.height * 2 * dir;
        cv::Point2f to = center - s.height * 2 * dir;
        vector<cv::Point> points;
        points.push_back(from * 2);
        points.push_back(to * 2);
        cv::polylines(all, points, false, cv::Scalar(255, 0, 0));
      }
      for (auto const &it : hlines) {
        cv::Point2f center = it.second.minCenter;
        cv::Point2f dir(it.second.line[0], it.second.line[1]);
        dir = dir / cv::norm(dir);
        cv::Point2f from = center + s.width * 2 * dir;
        cv::Point2f to = center - s.width * 2 * dir;
        vector<cv::Point> points;
        points.push_back(from * 2);
        points.push_back(to * 2);
        cv::polylines(all, points, false, cv::Scalar(0, 255, 0));
      }
      cout << "b64png(grids_" << cnt << "):" << base64::to_base64(Img::EncodeToPng(all)) << endl;
#endif

      // 以上の計算を largest に対して最初に 1 回だけ行っても良いかもしれないが, k = 2 以降でマージした結果を流用したいので毎回の k で計算する.

      auto const &cluster = clusters[k];
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
      cout << "b64png(sample_" << cnt << "_" << index << "):" << base64::to_base64(Img::EncodeToPng(all)) << endl;
    }
#endif
  }
#elif 1
  {
    RadianAverage av;
    double sumCos = 0;
    int count = 0;
    vector<float> all;
    double sum = 0;
    //    for (auto const &square : s.squares) {
    //      if (auto direction = square->direction(); direction) {
    //        double angle = Normalize90To90(Angle(*direction));
    //        sumCos += fabs(cos(angle));
    //        av.push(angle);
    //        all.push_back(angle / pi * 180);
    //        if (-5 <= angle / pi * 180 & angle / pi * 180 <= 5) {
    //          square->direction();
    //        }
    //        count++;
    //      }
    //    }
    for (auto const &piece : s.pieces) {
      double angle = Angle(piece->direction);
      while (angle < 0) {
        angle += pi * 2;
      }
      angle = fmod(angle, pi);
      //      if (angle > pi * 3 / 2) {
      //        angle = pi * 2 - angle;
      //      } else if (angle > pi) {
      //        angle = angle - pi;
      //      } else if (angle > pi / 2) {
      //        angle = pi - angle;
      //      }
      av.push(angle);
      sumCos += fabs(cos(angle));
      sum += angle / pi * 180;
      count++;
      all.push_back(angle / pi * 180);
    }
    double p = av.get();
    double q = acos(sumCos / count);
    double r = sum / count;
    double t = r / 180 * pi;
    s.boardDirection = t; // sum / count / 180 * pi;// acos(sumCos / count);
    cout << "p=" << (p * 180 / pi) << "q=" << (q * 180 / pi) << "r=" << r << endl;
  }
#else
  {
    // squares の各辺の傾きから, 盤面の向きを推定する.
    vector<double> angles;
    for (auto const &square : s.squares) {
      for (size_t i = 0; i < 3; i++) {
        auto const &a = square->points[i];
        auto const &b = square->points[i + 1];
        double dx = b.x - a.x;
        double dy = b.y - a.y;
        double angle = atan2(dy, dx);
        while (angle < 0) {
          angle += pi * 2;
        }
        while (pi * 0.5 < angle) {
          angle -= pi * 0.5;
        }
        angles.push_back(angle * 180 / pi);
      }
    }
    for (auto const &piece : s.pieces) {
      float angle = Angle(piece->direction);
      while (angle < 0) {
        angle += pi * 2;
      }
      while (pi * 0.5 < angle) {
        angle -= pi * 0.5;
      }
      angles.push_back(angle * 180 / pi);
    }
    // 盤面の向きを推定する.
    // 5 度単位でヒストグラムを作り, 最頻値を調べる. angle は [0, 90] に限定しているので index は [0, 18]
    array<int, 19> count;
    count.fill(0);
    for (double const &angle : angles) {
      int index = angle / 5;
      count[index] += 1;
    }
    int maxIndex = -1;
    int maxCount = -1;
    for (int i = 0; i < (int)count.size(); i++) {
      if (maxCount < count[i]) {
        maxCount = count[i];
        maxIndex = i;
      }
    }
    // 最頻となったヒストグラムの位置から, 前後 5 度に収まっている angle について, その平均値を計算する.
    double targetAngle = (maxIndex + 0.5) * 5;
    RadianAverage ra;
    for (double const &angle : angles) {
      if (cos(5.0 / 180.0 * pi) <= fabs(cos((angle - targetAngle) / 180 * pi))) {
        double radian = angle / 180.0 * pi;
        ra.push(radian);
      }
    }
    float direction = Normalize90To90(ra.get());
    s.boardDirection = direction;

    // Session.boardDirection は対局者の向きにしたい.
    map<bool, int> vote;
    // square の長手方向から, direction を 90 度回して訂正するか, そのまま採用するか投票する.
    for (auto const &square : s.squares) {
      if (auto d = SquareDirection(square->points); d) {
        bool shouldCorrect = fabs(cos(*d - direction)) < 0.25;
        vote[shouldCorrect] += 1;
      }
    }
    // piece の頂点の向きから, direction を 90 度回して訂正するか, そのまま採用するか投票する.
    for (auto const &piece : s.pieces) {
      float a = Normalize90To90(Angle(piece->direction));
      bool shouldCorrect = fabs(cos(a - direction)) < 0.25;
      vote[shouldCorrect] += 1;
    }
    //    cout << "vote[true] = " << vote[true] << "; vote[false] = " << vote[false] << endl;
    if (vote[true] > vote[false]) {
      cout << "vote[true] = " << vote[true] << "; vote[false] = " << vote[false] << "; correct" << endl;
      direction = Normalize90To90(direction + pi * 0.5);
      //      s.boardDirection = direction;
    } else {
      cout << "vote[true] = " << vote[true] << "; vote[false] = " << vote[false] << "; no-correct" << endl;
    }
  }

  if (false) {
    // TODO: ここの処理は斜めから撮影した時に s.outline が盤面を正確に覆わなくなる問題の対策のための処理. 多少斜め方向からの撮影なら, この処理なくても問題なく盤面を認識できているので無くても良い. 本当は斜め角度大のときも正確に検出できるようにしたい.
    //  各 square, piece を起点に, その長軸と短軸それぞれについて, 軸付近に中心を持つ駒・升を検出する.
    //  どの軸にも属さない square, piece を除去する.
    set<shared_ptr<Contour>> squares;     // 抽出した squares. 後で s.squares と swap する.
    set<shared_ptr<PieceContour>> pieces; // 抽出した pieces. 後で s.pieces と swap する.
    double cosThreshold = cos(2.5 / 180.0 * pi);
    auto Detect = [&](double direction, double tolerance, vector<cv::Vec4f> &grids) {
      set<shared_ptr<Contour>> cSquares;
      set<shared_ptr<PieceContour>> cPieces;
      // tolerance: 軸との距離の最大値. この距離以下なら軸上に居るとみなす.
      for (auto const &square : s.squares) {
        if (square->area <= s.squareArea * 0.7 || s.squareArea * 1.3 <= square->area) {
          // 升目の面積と比べて 3 割以上差がある場合対象外にする.
          continue;
        }
        if (cSquares.find(square) != cSquares.end()) {
          // 処理済み.
          continue;
        }
        cv::Point2f center = square->mean();
        // square の各辺の方向のうち, direction と近い方向をその square の向きとする.
        RadianAverage ra;
        for (int i = 0; i < 4; i++) {
          cv::Point2f edge = square->points[i] - square->points[(i + 1) % 4];
          double dir = Angle(edge);
          if (fabs(cos(dir - direction)) > cosThreshold) {
            ra.push(dir);
          }
        }
        if (ra.count != 2) {
          continue;
        }
        double squareDirection = ra.get();
        cv::Vec4f axis(cos(squareDirection), sin(squareDirection), center.x, center.y);
        vector<cv::Point2f> centers;
        for (auto const &sq : s.squares) {
          cv::Point2f c = sq->mean();
          if (sq == square) {
            // 自分自身.
            centers.push_back(c);
            continue;
          }
          if (cSquares.find(sq) != cSquares.end()) {
            // 処理済み.
            continue;
          }
          if (sq->area <= s.squareArea * 0.7 || s.squareArea * 1.3 <= sq->area) {
            // 升目の面積と比べて 3 割以上差がある場合対象外にする.
            continue;
          }
          double distance = Distance(axis, c);
          if (!isfinite(distance)) {
            continue;
          }
          if (distance <= tolerance) {
            cSquares.insert(sq);
            centers.push_back(c);
          }
        }
        for (auto const &p : s.pieces) {
          if (p->area > s.squareArea) {
            // 升目の面積より大きい piece は対象外にする.
            continue;
          }
          if (cPieces.find(p) != cPieces.end()) {
            // 処理済み
            continue;
          }
          cv::Point2f c = p->center();
          double distance = Distance(axis, c);
          if (!isfinite(distance)) {
            continue;
          }
          if (distance <= tolerance) {
            cPieces.insert(p);
            centers.push_back(c);
          }
        }
        if (centers.empty()) {
          continue;
        }
        auto grid = FitLine(centers);
        if (!grid) {
          continue;
        }
        if (fabs(atan2((*grid)[1], (*grid)[0]) - direction) <= cosThreshold) {
          // grid の向きと direction が大きく違っていたら, 検出した grid は無視する.
          continue;
        }
        for (auto const &cS : cSquares) {
          squares.insert(cS);
        }
        for (auto const &cP : cPieces) {
          pieces.insert(cP);
        }
        grids.push_back(*grid);
      }
    };
    Detect(s.boardDirection, sqrt(s.squareArea * s.aspectRatio) / 2, s.vgrids);
    Detect(s.boardDirection + pi * 0.5, sqrt(s.squareArea / s.aspectRatio) / 2, s.hgrids);
    s.squares.clear();
    for (auto const &square : squares) {
      s.squares.push_back(square);
    }
    s.pieces.clear();
    for (auto const &piece : pieces) {
      s.pieces.push_back(piece);
    }
  }
#endif

  {
    // squares と pieces を -1 * boardDirection 回転した状態で矩形を検出する.
    vector<cv::Point2d> centers;
    for (auto const &square : s.squares) {
      auto center = Rotate(square->mean(), -s.boardDirection);
      centers.push_back(center);
    }
    for (auto const &piece : s.pieces) {
      auto center = Rotate(piece->center(), -s.boardDirection);
      centers.push_back(center);
    }
    if (!centers.empty()) {
      cv::Point2f top = centers[0];
      cv::Point2f bottom = top;
      cv::Point2f left = top;
      cv::Point2f right = top;
      for (auto const &center : centers) {
        if (center.y < top.y) {
          top = center;
        }
        if (center.y > bottom.y) {
          bottom = center;
        }
        if (center.x < left.x) {
          left = center;
        }
        if (center.x > right.x) {
          right = center;
        }
      }
      cv::Point2f lt(left.x, top.y);
      cv::Point2f rt(right.x, top.y);
      cv::Point2f rb(right.x, bottom.y);
      cv::Point2f lb(left.x, bottom.y);
      s.outline.points = {
          Rotate(lt, s.boardDirection),
          Rotate(rt, s.boardDirection),
          Rotate(rb, s.boardDirection),
          Rotate(lb, s.boardDirection),
      };
      s.outline.area = fabs(cv::contourArea(cv::Mat(s.outline.points)));
      s.bx = left.x;
      s.by = top.y;
      s.bwidth = right.x - left.x;
      s.bheight = bottom.y - top.y;
    }
  }
}

// [x, y] の升目の中心位置を返す.
// [0, 0] が 9一, [8, 8] が 1九. 戻り値の座標は入力画像での座標系.
cv::Point2f PieceCenter(Status const &s, int x, int y) {
  float fx = s.bx + x * s.bwidth / 8;
  float fy = s.by + y * s.bheight / 8;
  return Rotate(cv::Point2f(fx, fy), s.boardDirection);
}

void FindPieces(cv::Mat const &frame, Status &s) {
  using namespace std;
  {
    set<pair<bool, int>> attached; // first => s.square なら true, s.pieces なら false, second => s.squares または s.pieces のインデックス.
    float const squareSize = sqrtf(s.squareArea);
    for (int y = 0; y < 9; y++) {
      for (int x = 0; x < 9; x++) {
        cv::Point2f center = PieceCenter(s, x, y);
        // center に最も中心が近い Contour を s.pieces か s.squares の中から探す.
        // 距離は駒サイズのスケール sqrt(squareArea) / 2 より離れていると対象外にする.
        optional<pair<bool, int>> index;
        float nearest = numeric_limits<float>::max();
        for (int i = 0; i < s.pieces.size(); i++) {
          pair<bool, int> key = make_pair(false, i);
          if (attached.find(key) != attached.end()) {
            continue;
          }
          cv::Point2f m = s.pieces[i]->center();
          float distance = cv::norm(m - center);
          if (distance > squareSize * 0.5) {
            continue;
          }
          if (distance < nearest) {
            nearest = distance;
            index = key;
          }
        }
        if (!index) {
          // 駒の方を優先で探す. 既に見つかっているならそちらを優先.
          for (int i = 0; i < s.squares.size(); i++) {
            pair<bool, int> key = make_pair(true, i);
            if (attached.find(key) != attached.end()) {
              continue;
            }
            cv::Point2f m = s.squares[i]->mean();
            float distance = cv::norm(m - center);
            if (distance > squareSize * 0.5) {
              continue;
            }
            if (distance < nearest) {
              nearest = distance;
              index = key;
            }
          }
        }
        if (!index) {
          continue;
        }
        attached.insert(*index);
        if (index->first) {
          s.detected[x][y] = s.squares[index->second];
        } else {
          s.detected[x][y] = make_shared<Contour>(s.pieces[index->second]->toContour());
        }
      }
    }
  }

  {
    // 検出した駒・升を元に, より正確な盤面の矩形を検出する.
    // 先手から見て盤面の上辺
    optional<cv::Vec4f> top;
    {
      vector<cv::Point2f> points;
      for (int x = 0; x < 9; x++) {
        if (s.detected[x][0]) {
          points.push_back(s.detected[x][0]->mean());
        }
      }
      if (points.size() > 1) {
        cv::Vec4f v;
        cv::fitLine(cv::Mat(points), v, cv::DIST_L2, 0, 0.01, 0.01);
        top = v;
      }
    }
    optional<cv::Vec4f> bottom;
    {
      vector<cv::Point2f> points;
      for (int x = 0; x < 9; x++) {
        if (s.detected[x][8]) {
          points.push_back(s.detected[x][8]->mean());
        }
      }
      if (points.size() > 1) {
        cv::Vec4f v;
        cv::fitLine(cv::Mat(points), v, cv::DIST_L2, 0, 0.01, 0.01);
        bottom = v;
      }
    }
    optional<cv::Vec4f> left;
    {
      vector<cv::Point2f> points;
      for (int y = 0; y < 9; y++) {
        if (s.detected[0][y]) {
          points.push_back(s.detected[0][y]->mean());
        }
      }
      if (points.size() > 1) {
        cv::Vec4f v;
        cv::fitLine(cv::Mat(points), v, cv::DIST_L2, 0, 0.01, 0.01);
        left = v;
      }
    }
    optional<cv::Vec4f> right;
    {
      vector<cv::Point2f> points;
      for (int y = 0; y < 9; y++) {
        if (s.detected[8][y]) {
          points.push_back(s.detected[8][y]->mean());
        }
      }
      if (points.size() > 1) {
        cv::Vec4f v;
        cv::fitLine(cv::Mat(points), v, cv::DIST_L2, 0, 0.01, 0.01);
        right = v;
      }
    }
    if (top && bottom && left && right) {
      auto tl = Intersection(*top, *left);
      auto tr = Intersection(*top, *right);
      auto br = Intersection(*bottom, *right);
      auto bl = Intersection(*bottom, *left);
      if (tl && tr && br && bl) {
        // tl, tr, br, bl は駒中心を元に計算しているので 8x8 の範囲しか無い. 半マス分増やす
        cv::Point2d topLeft = (*tl) + (((*tl) - (*tr)) / 16) + (((*tl) - (*bl)) / 16);
        cv::Point2d topRight = (*tr) + (((*tr) - (*tl)) / 16) + (((*tr) - (*br)) / 16);
        cv::Point2d bottomRight = (*br) + (((*br) - (*bl)) / 16) + (((*br) - (*tr)) / 16);
        cv::Point2d bottomLeft = (*bl) + (((*bl) - (*br)) / 16) + (((*bl) - (*tl)) / 16);

        Contour preciseOutline;
        preciseOutline.points = {topLeft, topRight, bottomRight, bottomLeft};
        preciseOutline.area = fabs(cv::contourArea(preciseOutline.points));
        s.preciseOutline = preciseOutline;
      }
    }
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
      cv::Point2f(width, 0),
      cv::Point2f(width, height),
      cv::Point2f(0, height),
      cv::Point2f(0, 0),
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
    FindBoard(frame, *s);
    FindPieces(frame, *s);
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
