#include "../shogi_camera_input.cpp"
#include <opencv2/imgcodecs/ios.h>
namespace sci {
cv::Mat Utility::MatFromUIImage(void *ptr) {
  cv::Mat image;
  UIImageToMat((__bridge UIImage *)ptr, image, true);
  return image;
}

void *Utility::UIImageFromMat(cv::Mat const &m) {
  return (__bridge_retained void *)MatToUIImage(m);
}
} // namespace sci
