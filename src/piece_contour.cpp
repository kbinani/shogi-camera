#include <shogi_camera/shogi_camera.hpp>

#include <opencv2/imgproc.hpp>

#include <numbers>

namespace sci {

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

PieceShape PieceContour::toShape() const {
  PieceShape ps;
  cv::Point2f bottom = points[2] - points[3];
  ps.width = cv::norm(bottom);
  cv::Point2f midBottom = points[3] + bottom * 0.5;
  cv::Point2f apex = points[0];
  ps.height = cv::norm(apex - midBottom);
  cv::Point2f s1 = points[1] - apex;
  cv::Point2f s2 = points[4] - apex;
  double angle = atan2(s1.y, s1.x) - atan2(s2.y, s2.x);
  while (angle < 0) {
    angle += std::numbers::pi * 2;
  }
  ps.capAngle = angle;
  return ps;
}

} // namespace sci
