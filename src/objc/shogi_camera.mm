#include "../shogi_camera.cpp"
#include <opencv2/imgcodecs/ios.h>

namespace sci {

cv::Mat Utility::MatFromUIImage(void *ptr) {
  cv::Mat image;
  UIImageToMat((__bridge UIImage *)ptr, image, true);
  return image;
}

cv::Mat Utility::MatFromCGImage(void *ptr) {
  cv::Mat image;
  CGImageToMat((CGImageRef)ptr, image);
  cv::cvtColor(image, image, cv::COLOR_BGR2RGB);
  return image;
}

void *Utility::UIImageFromMat(cv::Mat const &m) {
  return (__bridge_retained void *)MatToUIImage(m);
}

} // namespace sci
