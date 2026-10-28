#include "core/DomainBox.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace ofgui::core {

DomainBox::DomainBox(const Vec3d& min, const Vec3d& max)
    : min_(min)
    , max_(max)
{
}

const Vec3d& DomainBox::min() const noexcept
{
    return min_;
}

const Vec3d& DomainBox::max() const noexcept
{
    return max_;
}

void DomainBox::setMin(const Vec3d& min) noexcept
{
    min_ = min;
}

void DomainBox::setMax(const Vec3d& max) noexcept
{
    max_ = max;
}

DomainMode DomainBox::mode() const noexcept
{
    return mode_;
}

void DomainBox::setMode(DomainMode mode) noexcept
{
    mode_ = mode;
}

const std::string& DomainBox::unit() const noexcept
{
    return unit_;
}

void DomainBox::setUnit(std::string unit)
{
    unit_ = std::move(unit);
}

Vec3d DomainBox::size() const noexcept
{
    return max_ - min_;
}

bool DomainBox::isFinite() const noexcept
{
    return min_.isFinite() && max_.isFinite();
}

bool DomainBox::hasPositiveExtent() const noexcept
{
    const Vec3d extent = size();
    return extent.x > 0.0 && extent.y > 0.0 && extent.z > 0.0;
}

Real DomainBox::volume() const noexcept
{
    if (!hasPositiveExtent() || !isFinite())
    {
        return 0.0;
    }

    const Vec3d extent = size();
    return extent.x * extent.y * extent.z;
}

Real DomainBox::largestExtent() const noexcept
{
    const Vec3d extent = size();
    return std::max({extent.x, extent.y, extent.z});
}

Real DomainBox::smallestExtent() const noexcept
{
    const Vec3d extent = size();
    return std::min({extent.x, extent.y, extent.z});
}

DomainMode DomainBox::suggestMode(Real relativeThickness) const noexcept
{
    if (!isFinite() || !hasPositiveExtent())
    {
        return DomainMode::ThreeD;
    }

    const Vec3d extent = size();
    const Real thickest = largestExtent();
    if (!(thickest > 0.0) || !(relativeThickness > 0.0))
    {
        return DomainMode::ThreeD;
    }

    // Only the z axis counts as the 2D thickness direction, matching the
    // OpenFOAM convention of empty front/back patches. A tie (x == z) is
    // ambiguous and stays 3D.
    if (!(extent.z < extent.x) || !(extent.z < extent.y))
    {
        return DomainMode::ThreeD;
    }

    return (extent.z <= relativeThickness * thickest) ? DomainMode::TwoD : DomainMode::ThreeD;
}

} // namespace ofgui::core
