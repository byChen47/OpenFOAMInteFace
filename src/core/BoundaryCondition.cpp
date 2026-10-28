#include "core/BoundaryCondition.h"

#include <algorithm>
#include <array>
#include <utility>

namespace ofgui::core {
namespace {

/// Types the first version can create/edit; everything else imported from a
/// case is preserved verbatim but stays read only (case import specification
/// section 4.6).
constexpr std::array<std::string_view, 27> kKnownTypes{
    "alphatWallFunction",
    "calculated",
    "cyclic",
    "empty",
    "epsilonWallFunction",
    "fixedFluxPressure",
    "fixedGradient",
    "fixedValue",
    "flowRateInletVelocity",
    "inletOutlet",
    "kqRWallFunction",
    "movingWallVelocity",
    "noSlip",
    "nutkWallFunction",
    "omegaWallFunction",
    "outletInlet",
    "pressureInletOutletVelocity",
    "slip",
    "surfaceNormalFixedValue",
    "symmetry",
    "symmetryPlane",
    "totalPressure",
    "turbulentIntensityKineticEnergyInlet",
    "turbulentMixingLengthDissipationRateInlet",
    "uniformFixedValue",
    "wall",
    "zeroGradient"};

} // namespace

BoundaryCondition::BoundaryCondition(std::string type)
    : type_(std::move(type))
{
}

const std::string& BoundaryCondition::type() const noexcept
{
    return type_;
}

void BoundaryCondition::setType(std::string type)
{
    type_ = std::move(type);
}

const std::map<std::string, std::string, std::less<>>& BoundaryCondition::parameters() const noexcept
{
    return parameters_;
}

void BoundaryCondition::setParameter(std::string name, std::string value)
{
    parameters_.insert_or_assign(std::move(name), std::move(value));
}

bool BoundaryCondition::removeParameter(std::string_view name)
{
    return parameters_.erase(std::string(name)) > 0;
}

bool BoundaryCondition::hasParameter(std::string_view name) const
{
    return parameters_.find(name) != parameters_.end();
}

std::optional<std::string> BoundaryCondition::parameter(std::string_view name) const
{
    const auto found = parameters_.find(name);
    if (found == parameters_.end())
    {
        return std::nullopt;
    }

    return found->second;
}

bool BoundaryCondition::isEmpty() const noexcept
{
    return type_.empty();
}

bool BoundaryCondition::isKnownType() const noexcept
{
    return isKnownTypeName(type_);
}

bool BoundaryCondition::isKnownTypeName(std::string_view type) noexcept
{
    return std::find(kKnownTypes.begin(), kKnownTypes.end(), type) != kKnownTypes.end();
}

} // namespace ofgui::core
