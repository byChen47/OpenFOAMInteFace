#pragma once

#include <core/Vec3d.h>

#include <array>
#include <string>

namespace ofgui::io {

/// Shortest round-trip representation of a real, matching OpenFOAM's writer
/// (`1e-06` rather than `1.00000e-06`, `0.5` rather than `0.500000`).
std::string formatFoamReal(core::Real value);

std::string formatFoamInt(long long value);

/// Coordinate formatting with 15 significant digits: removes the binary noise
/// that a `vertices * scale` round-trip leaves behind (`0.10000000000000002`
/// becomes `0.1`) while keeping every user supplied digit.
std::string formatFoamCoordinate(core::Real value);

/// `(x y z)` with single spaces, as used for `uniform` vector values.
std::string formatFoamVector(const core::Vec3d& value);

/// `(x y z)` for mesh vertices: like `formatFoamVector` but with 15 significant
/// digits so a `vertices * scale` round-trip does not print `0.10000000000000002`.
std::string formatFoamPoint(const core::Vec3d& value);

/// `[m l t T mol A cd]`, the seven SI exponents in OpenFOAM order.
std::string formatFoamDimensions(const std::array<int, 7>& dimensions);

} // namespace ofgui::io
