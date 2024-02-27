#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#if !defined(NDEBUG)
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
