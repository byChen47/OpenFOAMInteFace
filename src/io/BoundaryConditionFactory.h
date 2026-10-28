#pragma once

#include "core/BoundaryCondition.h"
#include "core/ValidationIssue.h"

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ofgui::io {

/// One `boundaryField` entry as read from a field dictionary.
struct ProjectedBoundaryCondition
{
    std::string patchName;
    core::BoundaryCondition condition;
    int line = 0;
    int column = 0;
    /// False for a regular expression patch key (never enters the model).
    bool regularExpression = false;
    bool hasType = false;
};

/// Builds `core::BoundaryCondition` values from parser text.
///
/// The type word is kept verbatim; unknown types stay read only and are
/// reported once (`IMPORT-BC-001`) so the model still round trips the file.
class BoundaryConditionFactory
{
public:
    /// @return the condition, or `std::nullopt` when `typeText` is empty
    ///         (reported as `IMPORT-BC-002`).
    [[nodiscard]] static std::optional<core::BoundaryCondition> create(
        std::string_view typeText,
        std::map<std::string, std::string> parameters,
        std::string_view file,
        int line,
        int column,
        std::string_view nodePath,
        std::vector<core::ValidationIssue>& issues);
};

} // namespace ofgui::io
