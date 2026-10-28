#include "io/BoundaryConditionFactory.h"

#include "io/ImportCodes.h"
#include "io/ImportIssueFactory.h"

#include <utility>

namespace ofgui::io {

std::optional<core::BoundaryCondition> BoundaryConditionFactory::create(
    std::string_view typeText,
    std::map<std::string, std::string> parameters,
    std::string_view file,
    int line,
    int column,
    std::string_view nodePath,
    std::vector<core::ValidationIssue>& issues)
{
    if (typeText.empty())
    {
        issues.push_back(
            makeImportIssue(importcodes::kBoundaryMissingType, file, line, column, std::string(nodePath)));
        return std::nullopt;
    }

    core::BoundaryCondition condition{std::string(typeText)};
    if (!condition.isKnownType())
    {
        issues.push_back(makeImportIssue(importcodes::kBoundaryUnknownType,
                                        file,
                                        line,
                                        column,
                                        std::string(nodePath) + ".type",
                                        std::string(typeText)));
    }
    for (auto& [name, value] : parameters)
    {
        // `type` is the only structural key; everything else is a parameter.
        condition.setParameter(name, value);
    }
    return condition;
}

} // namespace ofgui::io
