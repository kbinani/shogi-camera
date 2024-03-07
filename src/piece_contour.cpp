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

void PieceShape::poly(cv::Point2f const &center, std::vector<cv::Point2f> &buffer, Color color) const {
  buffer.clear();
  if (color == Color::Black) {
    buffer.push_back(apex + center);
    buffer.push_back(point1 + center);
    buffer.push_back(point2 + center);
    buffer.push_back(cv::Point2f(-point2.x, point2.y) + center);
    buffer.push_back(cv::Point2f(-point1.x, point1.y) + center);
  } else {
    buffer.push_back(-apex + center);
    buffer.push_back(-point1 + center);
    buffer.push_back(-point2 + center);
    buffer.push_back(cv::Point2f(point2.x, -point2.y) + center);
    buffer.push_back(cv::Point2f(point1.x, -point1.y) + center);
  }
}

PieceShape PieceContour::toShape() const {
  using namespace std;
  cv::Point2f apex = points[0];
  cv::Point2f bottom = points[3] - points[2];
  cv::Point2f mid = points[2] + bottom * 0.5f;
  cv::Point2f direction = apex - mid;
  double angle = atan2(direction.y, direction.x) * 180 / numbers::pi;
  cv::Mat mtx = cv::getRotationMatrix2D(mid, -angle + 270, 1);
  cv::Point2f rApex = WarpAffine(apex, mtx) - mid;
  cv::Point2f rPoint1 = WarpAffine(points[1], mtx) - mid;
  cv::Point2f rPoint2 = WarpAffine(points[2], mtx) - mid;
  cv::Point2f rPoint3 = WarpAffine(points[3], mtx) - mid;
  cv::Point2f rPoint4 = WarpAffine(points[4], mtx) - mid;

  // 一旦 rApex が中心となるように平行移動する
  cv::Point2f p1 = rPoint1 - rApex;
  cv::Point2f p2 = rPoint2 - rApex;
  cv::Point2f p3 = rPoint3 - rApex;
  cv::Point2f p4 = rPoint4 - rApex;

  cv::Point2f p14 = (p1 + cv::Point2f(-p4.x, p4.y)) * 0.5f;
  if (p14.x < 0) {
    p14.x = -p14.x;
  }
  cv::Point2f p23 = (p2 + cv::Point2f(-p3.x, p3.y)) * 0.5f;
  if (p23.x < 0) {
    p23.x = -p23.x;
  }
  cv::Point2f center(0, p23.y * 0.5f);

  PieceShape ps;
  ps.apex = -center;
  ps.point1 = p14 - center;
  ps.point2 = p23 - center;

  return ps;
}

} // namespace sci
