#include "render/DomainBoundsAdapter.h"

#include "core/Basics.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace ofgui {

std::optional<PreviewDomain> DomainBoundsAdapter::previewDomain(const core::DomainBox& box)
{
    if (!box.isFinite() || !box.hasPositiveExtent())
    {
        return std::nullopt;
    }

    PreviewDomain preview;
    preview.minX = box.min().x;
    preview.maxX = box.max().x;
    preview.minY = box.min().y;
    preview.maxY = box.max().y;
    preview.minZ = box.min().z;
    preview.maxZ = box.max().z;
    return preview;
}

bool DomainBoundsAdapter::sameBounds(const PreviewDomain& left, const PreviewDomain& right)
{
    const std::array<double, 6> lhs = left.bounds();
    const std::array<double, 6> rhs = right.bounds();

    for (std::size_t index = 0; index < lhs.size(); ++index)
    {
        const double scale = std::max({1.0, std::abs(lhs[index]), std::abs(rhs[index])});
        if (std::abs(lhs[index] - rhs[index]) > core::kRealTolerance * scale)
        {
            return false;
        }
    }
    return true;
}

} // namespace ofgui
