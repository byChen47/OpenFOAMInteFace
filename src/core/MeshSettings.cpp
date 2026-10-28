#include "core/MeshSettings.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace ofgui::core {

MeshSettings::MeshSettings(const CellCounts& cells, const Grading& grading, Real scale) noexcept
    : cells_(cells)
    , grading_(grading)
    , scale_(scale)
{
}

const MeshSettings::CellCounts& MeshSettings::cells() const noexcept
{
    return cells_;
}

void MeshSettings::setCells(const CellCounts& cells) noexcept
{
    cells_ = cells;
}

bool MeshSettings::setCellCount(std::size_t axis, int count) noexcept
{
    if (axis >= cells_.size())
    {
        return false;
    }

    cells_[axis] = count;
    return true;
}

int MeshSettings::cellCount(std::size_t axis) const
{
    return cells_.at(axis);
}

const MeshSettings::Grading& MeshSettings::grading() const noexcept
{
    return grading_;
}

void MeshSettings::setGrading(const Grading& grading) noexcept
{
    grading_ = grading;
}

bool MeshSettings::setGradingFactor(std::size_t axis, Real factor) noexcept
{
    if (axis >= grading_.size())
    {
        return false;
    }

    grading_[axis] = factor;
    return true;
}

Real MeshSettings::scale() const noexcept
{
    return scale_;
}

void MeshSettings::setScale(Real scale) noexcept
{
    scale_ = scale;
}

std::int64_t MeshSettings::totalCells() const noexcept
{
    const std::int64_t nx = cells_[0];
    const std::int64_t ny = cells_[1];
    const std::int64_t nz = cells_[2];
    if (nx <= 0 || ny <= 0 || nz <= 0)
    {
        return -1;
    }

    constexpr auto kMax = std::numeric_limits<std::int64_t>::max();
    if (nx > kMax / ny)
    {
        return kMax;
    }

    const std::int64_t layer = nx * ny;
    if (layer > kMax / nz)
    {
        return kMax;
    }

    return layer * nz;
}

bool MeshSettings::hasPositiveCells() const noexcept
{
    return std::all_of(cells_.begin(), cells_.end(), [](int count) { return count > 0; });
}

bool MeshSettings::hasPositiveGrading() const noexcept
{
    return std::all_of(grading_.begin(), grading_.end(),
        [](Real factor) { return std::isfinite(factor) && factor > 0.0; });
}

bool MeshSettings::isValid() const noexcept
{
    return hasPositiveCells() && hasPositiveGrading() && std::isfinite(scale_) && scale_ > 0.0;
}

bool MeshSettings::isSingleCellLayer(std::size_t axis) const
{
    return cells_.at(axis) == 1;
}

bool MeshSettings::exceedsCellBudget() const noexcept
{
    return totalCells() > kCellBudgetWarning;
}

} // namespace ofgui::core
