#pragma once

#include "io/FoamSourceLocation.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace ofgui::io {

/// Severity of a parse/IO diagnostic.
///
/// The four-level scheme of docs/CASE_IMPORT_SPEC.md §8.1 maps onto these three:
/// `Error` covers Error/Blocking (import cannot trust the file), `Warning`
/// covers Warning/Partial, `Info` covers informational notes.
enum class FoamSeverity
{
    Info,
    Warning,
    Error,
};

/// One parser/IO problem with enough context to jump to the offending line.
///
/// `code` uses the stable IMPORT-* identifiers from docs/CASE_IMPORT_SPEC.md
/// Appendix C so the import report can group issues without string matching.
struct FoamDiagnostic
{
    FoamSeverity severity{FoamSeverity::Error};
    std::string code;
    std::string message;
    FoamSourceLocation location;
    std::string snippet; ///< offending source line, trimmed and length-capped

    /// "file:line:column: error [IMPORT-PARSE-001] message"
    std::string toString() const;
};

std::string_view foamSeverityName(FoamSeverity severity);

/// True when at least one diagnostic has `FoamSeverity::Error`.
bool foamHasError(const std::vector<FoamDiagnostic>& diagnostics);
std::size_t foamCount(const std::vector<FoamDiagnostic>& diagnostics, FoamSeverity severity);

} // namespace ofgui::io
