#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include <shogi_camera/shogi_camera.hpp>

using namespace sci;

#if !defined(NDEBUG)
#include "move.test.hpp"
#include "sunfish3.test.hpp"
#endif

namespace sci {

void RunTests() {
#if !defined(NDEBUG)
  doctest::Context context;
  context.run();
#endif
}

} // namespace sci
