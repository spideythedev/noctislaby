#pragma once

#include <limits>

namespace nle {

#if defined(NLE_DOUBLE_PRECISION)
using real_t = double;
#else
using real_t = float;
#endif

inline constexpr real_t REAL_EPSILON = std::numeric_limits<real_t>::epsilon();

} // namespace nle
