#include "render/PreviewDomain.h"

namespace ofgui {

std::array<double, 6> PreviewDomain::bounds() const
{
    return {minX, maxX, minY, maxY, minZ, maxZ};
}

PreviewDomain makeDefaultPreviewDomain()
{
    return PreviewDomain{};
}

} // namespace ofgui