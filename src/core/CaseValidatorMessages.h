#pragma once

/// @file CaseValidatorMessages.h
/// Message and formatting helpers shared by the split `CaseValidator`
/// translation units.
///
/// Internal to `src/core`: this header is deliberately not part of the module
/// surface other modules may rely on. It only exists so the per-family
/// validators (CASE-DOM / CASE-MESH / CASE-PATCH / CASE-FIELD / CASE-SOLV)
/// keep byte-identical message text without duplicating helpers.

#include "core/Basics.h"
#include "core/FaceId.h"
#include "core/ValidationIssue.h"

#include <array>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace ofgui::core::detail {

inline constexpr std::array<const char*, 3> kAxisNames{"x", "y", "z"};

inline std::string formatReal(Real value)
{
    std::ostringstream stream;
    stream << std::setprecision(6) << value;
    return stream.str();
}

inline std::string joinNames(const std::string& separator, const std::vector<std::string>& names)
{
    std::string text;
    for (std::size_t index = 0; index < names.size(); ++index)
    {
        if (index > 0)
        {
            text += separator;
        }
        text += names[index];
    }

    return text;
}

inline std::string faceLabel(FaceId face)
{
    return faceName(face);
}

inline ValidationIssue makeIssue(
    Severity severity,
    std::string code,
    std::string message,
    std::string targetField)
{
    ValidationIssue issue(severity, std::move(code), std::move(message));
    issue.setTargetField(std::move(targetField));
    return issue;
}

} // namespace ofgui::core::detail
