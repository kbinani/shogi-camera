#pragma once

#include <opencv2/core/core.hpp>

namespace com::github::kbinani::sci {

void findSquares(cv::Mat const& image, std::vector<std::vector<cv::Point2i>> &squares);

}
