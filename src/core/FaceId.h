#pragma once

#include <array>
#include <cstddef>

namespace ofgui::core {

/// The six boundary faces of a single rectangular domain.
///
/// The numeric values are part of the persisted project schema and of the VTK
/// pick mapping, so the order must never change: `XMin=0, XMax=1, YMin=2,
/// YMax=3, ZMin=4, ZMax=5` (max face = min face + 1 on every axis).
///
/// This enum used to live in `src/render/FaceId.h`; it moved to the core layer
/// because `BoundaryPatch` owns face assignments and `render` must not be the
/// business data source (see `docs/DECISIONS.md`, FaceId migration entry).
enum class FaceId : int
{
    XMin = 0,
    XMax = 1,
    YMin = 2,
    YMax = 3,
    ZMin = 4,
    ZMax = 5
};

/// All faces in stable enum order; iterate this instead of hand written arrays.
inline constexpr std::array<FaceId, 6> kAllFaceIds{
    FaceId::XMin,
    FaceId::XMax,
    FaceId::YMin,
    FaceId::YMax,
    FaceId::ZMin,
    FaceId::ZMax
};

inline constexpr std::size_t faceIndex(FaceId faceId) noexcept
{
    return static_cast<std::size_t>(faceId);
}

/// Axis owning the face: 0 = x, 1 = y, 2 = z.
inline constexpr int faceAxis(FaceId faceId) noexcept
{
    return static_cast<int>(faceId) / 2;
}

/// True for the max side (`XMax`, `YMax`, `ZMax`) of an axis.
inline constexpr bool faceIsMax(FaceId faceId) noexcept
{
    return (static_cast<int>(faceId) % 2) == 1;
}

/// ASCII name used in messages, JSON and export ("xMin" ... "zMax").
inline constexpr const char* faceName(FaceId faceId) noexcept
{
    constexpr std::array<const char*, 6> names{
        "xMin", "xMax", "yMin", "yMax", "zMin", "zMax"};
    return names[faceIndex(faceId)];
}

} // namespace ofgui::core
