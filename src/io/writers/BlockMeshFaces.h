#pragma once

#include <core/FaceId.h>

#include <array>

namespace ofgui::io {

/// Vertex quads of a blockMesh hexahedron, in OpenFOAM's canonical orientation
/// (cyclic order, outward normal). Indexed by `core::faceIndex(FaceId)`.
inline constexpr std::array<std::array<int, 4>, 6> kHexFaceQuads{{
    {{0, 4, 7, 3}}, // xMin
    {{2, 6, 5, 1}}, // xMax
    {{1, 5, 4, 0}}, // yMin
    {{3, 7, 6, 2}}, // yMax
    {{0, 3, 2, 1}}, // zMin
    {{4, 5, 6, 7}}, // zMax
}};

inline constexpr const std::array<int, 4>& hexFaceQuad(core::FaceId face) noexcept
{
    return kHexFaceQuads[core::faceIndex(face)];
}

} // namespace ofgui::io
