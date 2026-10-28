#pragma once

#include "core/BoundaryCondition.h"
#include "core/FieldValue.h"
#include "core/ObjectId.h"

#include <array>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ofgui::core {

/// OpenFOAM `FoamFile.class` of a field dictionary the GUI can edit.
enum class FieldClass
{
    VolScalarField,
    VolVectorField
};

/// `volScalarField` / `volVectorField`.
[[nodiscard]] std::string_view fieldClassName(FieldClass fieldClass) noexcept;

/// Parses a `FoamFile.class` word; unknown classes return `std::nullopt`.
[[nodiscard]] std::optional<FieldClass> fieldClassFromName(std::string_view name) noexcept;

/// One field file (`0/p`, `0/U`, ...): dimensions, interior value and one
/// boundary condition per patch.
class FieldDefinition
{
public:
    /// The seven SI exponents of a `dimensions [...]` entry.
    using Dimensions = std::array<int, 7>;

    FieldDefinition() = default;

    FieldDefinition(
        std::string name,
        FieldClass fieldClass,
        const Dimensions& dimensions = Dimensions{});

    [[nodiscard]] const ObjectId& id() const noexcept;
    void setId(const ObjectId& id) noexcept;

    [[nodiscard]] const std::string& name() const noexcept;
    void setName(std::string name);

    [[nodiscard]] FieldClass fieldClass() const noexcept;
    void setFieldClass(FieldClass fieldClass) noexcept;

    [[nodiscard]] const Dimensions& dimensions() const noexcept;
    void setDimensions(const Dimensions& dimensions) noexcept;

    [[nodiscard]] const FieldValue& internalField() const noexcept;
    void setInternalField(FieldValue value);

    [[nodiscard]] const std::map<ObjectId, BoundaryCondition>& boundaryConditions() const noexcept;

    /// Adds or replaces the condition of one patch.
    void setBoundaryCondition(const ObjectId& patchId, BoundaryCondition condition);

    /// @return true when a condition existed and was removed.
    bool removeBoundaryCondition(const ObjectId& patchId);

    [[nodiscard]] const BoundaryCondition* findBoundaryCondition(const ObjectId& patchId) const;
    [[nodiscard]] BoundaryCondition* findBoundaryCondition(const ObjectId& patchId);

    [[nodiscard]] bool hasBoundaryCondition(const ObjectId& patchId) const;

    /// Patch ids of `allPatchIds` that have no condition in this field, in input
    /// order. Used by `CaseValidator` to report incomplete `boundaryField`s.
    [[nodiscard]] std::vector<ObjectId> patchesWithoutCondition(
        const std::vector<ObjectId>& allPatchIds) const;

private:
    ObjectId id_;
    std::string name_;
    FieldClass fieldClass_ = FieldClass::VolScalarField;
    Dimensions dimensions_{};
    FieldValue internalField_;
    std::map<ObjectId, BoundaryCondition> boundaryConditions_;
};

} // namespace ofgui::core
