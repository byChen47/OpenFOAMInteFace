#include "io/FoamDiagnostic.h"

#include <algorithm>
#include <cstddef>
#include <string>

namespace ofgui::io {

std::string FoamDiagnostic::toString() const
{
    std::string out = location.toString();
    out += ": ";
    out += foamSeverityName(severity);
    if (!code.empty())
    {
        out += " [";
        out += code;
        out += ']';
    }
    out += ' ';
    out += message;
    return out;
}

std::string_view foamSeverityName(FoamSeverity severity)
{
    switch (severity)
    {
    case FoamSeverity::Info:
        return "info";
    case FoamSeverity::Warning:
        return "warning";
    case FoamSeverity::Error:
        return "error";
    }
    return "unknown";
}

bool foamHasError(const std::vector<FoamDiagnostic>& diagnostics)
{
    return std::any_of(diagnostics.begin(), diagnostics.end(), [](const FoamDiagnostic& diagnostic) {
        return diagnostic.severity == FoamSeverity::Error;
    });
}

std::size_t foamCount(const std::vector<FoamDiagnostic>& diagnostics, FoamSeverity severity)
{
    return static_cast<std::size_t>(std::count_if(
        diagnostics.begin(),
        diagnostics.end(),
        [severity](const FoamDiagnostic& diagnostic) { return diagnostic.severity == severity; }));
}

} // namespace ofgui::io
