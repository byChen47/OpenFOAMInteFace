#include "io/ProjectJsonDiagnostic.h"

#include <algorithm>

namespace ofgui::io {

std::string ProjectJsonDiagnostic::toString() const
{
    std::string text;
    if (!source.empty())
    {
        text += source;
        text += ": ";
    }
    if (!path.empty())
    {
        text += path;
        text += ": ";
    }
    text += projectJsonSeverityName(severity);
    text += " [";
    text += code;
    text += "] ";
    text += message;
    return text;
}

std::string_view projectJsonSeverityName(ProjectJsonSeverity severity) noexcept
{
    switch (severity)
    {
    case ProjectJsonSeverity::Info:
        return "info";
    case ProjectJsonSeverity::Warning:
        return "warning";
    case ProjectJsonSeverity::Error:
        return "error";
    }
    return "error";
}

bool projectJsonHasError(const std::vector<ProjectJsonDiagnostic>& diagnostics) noexcept
{
    return std::any_of(
        diagnostics.begin(),
        diagnostics.end(),
        [](const ProjectJsonDiagnostic& diagnostic)
        {
            return diagnostic.severity == ProjectJsonSeverity::Error;
        });
}

std::size_t projectJsonCount(
    const std::vector<ProjectJsonDiagnostic>& diagnostics,
    ProjectJsonSeverity severity) noexcept
{
    return static_cast<std::size_t>(std::count_if(
        diagnostics.begin(),
        diagnostics.end(),
        [severity](const ProjectJsonDiagnostic& diagnostic)
        {
            return diagnostic.severity == severity;
        }));
}

} // namespace ofgui::io
