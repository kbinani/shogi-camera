#if SHOGI_CAMERA_DEBUG

#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include <shogi_camera/shogi_camera.hpp>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

using namespace sci;

#include "game.test.hpp"
#include "img.test.hpp"
#include "move.test.hpp"

namespace sci {

void RunTests() {
  doctest::Context context;
  context.run();
}

} // namespace sci

#endif
