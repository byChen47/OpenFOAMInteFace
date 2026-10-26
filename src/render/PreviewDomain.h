#pragma once

#include <array>

namespace ofgui {

struct PreviewDomain
{
    double minX = -1.0;
    double maxX = 1.0;
    double minY = -0.75;
    double maxY = 0.75;
    double minZ = -0.5;
    double maxZ = 0.5;

    std::array<double, 6> bounds() const;
};

PreviewDomain makeDefaultPreviewDomain();

} // namespace ofgui