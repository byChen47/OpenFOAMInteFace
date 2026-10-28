#pragma once

#include "core/FieldDefinition.h"
#include "core/FieldValue.h"
#include "io/BoundaryConditionFactory.h"
#include "io/CaseProjectionContext.h"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace ofgui::io {

class FoamDictionary;

/// One `0/` or `0.orig/` dictionary projected into field definitions.
struct ProjectedField
{
    /// `object` when present, otherwise the file name (spec section 6.4).
    std::string name;
    /// `FoamFile.class` verbatim; empty when the header could not be read.
    std::string classText;
    /// Set only for `volScalarField` and `volVectorField`.
    std::optional<core::FieldClass> fieldClass;
    std::array<int, 7> dimensions{};
    bool hasDimensions = false;
    core::FieldValue internalField;
    std::vector<ProjectedBoundaryCondition> boundaryField;
    /// True when the header was missing or unreadable.
    bool headerMissing = false;
};

/// Projects one field dictionary (`0/p`, `0/U`, ...) into model values.
///
/// The projector never edits `internalField` or the boundary conditions beyond
/// wrapping them: uniform values become typed `FieldValue`s, everything else is
/// kept as raw OpenFOAM text (spec section 6.5).
class FieldProjector
{
public:
    [[nodiscard]] static ProjectedField project(
        const FoamDictionary& dictionary,
        const CaseProjectionContext& context);
};

} // namespace ofgui::io
