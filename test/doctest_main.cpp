#if SHOGI_CAMERA_DEBUG

#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include <shogi_camera/shogi_camera.hpp>

using namespace sci;

#include "game.test.hpp"
#include "move.test.hpp"
#include "sunfish3.test.hpp"

namespace sci {

void RunTests() {
  doctest::Context context;
  context.run();
}

} // namespace sci

#endif
