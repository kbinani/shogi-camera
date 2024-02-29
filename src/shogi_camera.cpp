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
  if (auto d = PieceDirection(points); d) {
    double norm = cv::norm(*d);
    return *d / norm * length;
  } else {
    return std::nullopt;
  }
}

std::shared_ptr<PieceContour> PieceContour::Make(std::vector<cv::Point2f> const &points) {
  using namespace std;
  if (points.size() != 5) {
    return nullptr;
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
    return nullptr;
  }
  if (min0 == 0 && min1 == 4) {
    swap(min0, min1);
  }
  // 将棋の駒の頂点
  cv::Point2f apex = points[min1];
  // 底辺の中点
  cv::Point2f bottom1 = points[(min0 + 3) % 5];
  cv::Point2f bottom2 = points[(min0 + 4) % 5];
  cv::Point2f midBottom = (bottom1 + bottom2) * 0.5;

  auto ret = make_shared<PieceContour>();
  for (int i = 0; i < 5; i++) {
    ret->points.push_back(points[(min1 + i) % 5]);
  }
  ret->area = fabs(cv::contourArea(ret->points));
  ret->direction = (apex - midBottom) / cv::norm(apex - midBottom);
  ret->aspectRatio = cv::norm(bottom1 - bottom2) / cv::norm(apex - midBottom);

  return ret;
}

Status::Status() {
}

SessionWrapper::SessionWrapper() : ptr(std::make_shared<Session>()) {
}

void SessionWrapper::startGame(Color userColor, int aiLevel) {
  std::shared_ptr<AI> ai;
  if (aiLevel == 0) {
    ai = std::make_shared<RandomAI>();
  } else {
    ai = std::make_shared<Sunfish3AI>();
  }
  if (userColor == Color::White) {
    ptr->setPlayers(ai, nullptr);
  } else {
    ptr->setPlayers(nullptr, ai);
  }
}

} // namespace sci
