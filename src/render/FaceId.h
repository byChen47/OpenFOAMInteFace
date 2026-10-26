#pragma once

#include <array>
#include <cstddef>

namespace ofgui {

enum class FaceId : int
{
    XMin = 0,
    XMax = 1,
    YMin = 2,
    YMax = 3,
    ZMin = 4,
    ZMax = 5
};

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

} // namespace ofgui