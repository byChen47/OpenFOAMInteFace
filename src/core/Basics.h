#pragma once

#include <cstddef>

namespace ofgui::core {

/// Floating point type used by every geometric and physical quantity in the model.
using Real = double;

/// Signed index type for grid and mesh related arithmetic.
using Index = std::ptrdiff_t;

/// Absolute tolerance used by `Real` comparisons of domain geometry.
inline constexpr Real kRealTolerance = 1.0e-12;

} // namespace ofgui::core
