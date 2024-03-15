#include <shogi_camera/shogi_camera.hpp>

#include <opencv2/imgproc.hpp>

namespace sci {

namespace {

std::optional<cv::Point2f> PieceDirection(std::vector<cv::Point2f> const &points) {
  using namespace std;
  if (points.size() != 5) {
    return nullopt;
  }
  vector<int> indices;
  for (int i = 0; i < 5; i++) {
    indices.push_back(i);
  }
  sort(indices.begin(), indices.end(), [&points](int a, int b) {
    float lengthA = cv::norm(points[a] - points[(a + 1) % 5]);
    float lengthB = cv::norm(points[b] - points[(b + 1) % 5]);
    return lengthA < lengthB;
  });
  // 最も短い辺が隣り合っている場合, 将棋の駒らしいと判定する.
  int min0 = indices[0];
  int min1 = indices[1];
  if (min1 < min0) {
    swap(min0, min1);
  }
  if (min0 + 1 != min1 && !(min0 == 0 && min1 == 4)) {
    return nullopt;
  }
  if (min0 == 0 && min1 == 4) {
    swap(min0, min1);
  }
  // 将棋の駒の頂点
  cv::Point2f apex = points[min1];
  // 底辺の中点
  cv::Point2f midBottom = (points[(min0 + 3) % 5] + points[(min0 + 4) % 5]) * 0.5;

  return apex - midBottom;
}

} // namespace

std::optional<cv::Point2f> Contour::direction(float length) const {
  using namespace std;
  if (auto d = PieceDirection(points); d) {
    double norm = cv::norm(*d);
    return *d / norm * length;
  } else if (points.size() == 4) {
    vector<pair<size_t, double>> lengths;
    for (size_t i = 0; i < points.size(); i++) {
      double length = cv::norm(points[i] - points[(i + 1) % 4]);
      lengths.push_back(make_pair(i, length));
    }
    sort(lengths.begin(), lengths.end(), [](auto const &a, auto const &b) {
      return a.second > b.second;
    });
    size_t max0 = lengths[0].first;
    size_t max1 = lengths[1].first;
    if (max1 < max0) {
      swap(max0, max1);
    }
    if (max0 + 2 == max1) {
      // 最も長い辺が対面同士になっている場合, 長方形だと判定する.
      cv::Point2f v0 = points[(max0 + 1) % 4] - points[max0];
      cv::Point2f v1 = points[max1] - points[(max1 + 1) % 4];
      return (v0 + v1) * 0.5f;
    }
    return nullopt;
  }
}

Status::Status() {
}

SessionWrapper::SessionWrapper() : ptr(std::make_shared<Session>()) {
}

void SessionWrapper::startGame(Color userColor, int aiLevel) {
  std::shared_ptr<Player> player;
  if (aiLevel == 0) {
    player = std::make_shared<CsaAdapter>(userColor == Color::Black ? Color::White : Color::Black, "192.168.1.33", 4081, "ShogiCameraApp", "bar");
  } else {
    player = std::make_shared<Sunfish3AI>();
  }
  if (userColor == Color::White) {
    ptr->setPlayers(player, nullptr);
  } else {
    ptr->setPlayers(nullptr, player);
  }
}

} // namespace sci
