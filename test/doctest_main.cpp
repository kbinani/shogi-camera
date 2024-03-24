#if !defined(NDEBUG)
#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include <shogi_camera/shogi_camera.hpp>

using namespace sci;

#include "move.test.hpp"
#endif

namespace sci {

void RunTests() {
#if !defined(NDEBUG)
  doctest::Context context;
  context.run();
#endif
}

} // namespace sci
