#pragma once

#include "render/FaceId.h"

#include <array>

namespace ofgui {

struct FaceAppearance
{
    const char* key;
    const char* displayName;
    std::array<double, 3> color;
};

FaceAppearance faceAppearance(FaceId faceId);

} // namespace ofgui