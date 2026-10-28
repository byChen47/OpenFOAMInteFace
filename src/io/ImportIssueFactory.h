#pragma once

#include "io/FoamDiagnostic.h"

#include "core/ValidationIssue.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ofgui::io {

/// Severity, Chinese message and suggested fix of one `IMPORT-*` code.
struct ImportCodeInfo
{
    core::Severity severity = core::Severity::Error;
    std::string_view message;
    std::string_view suggestedFix;
};

/// Looks up an `IMPORT-*` code in the table that mirrors
/// `docs/CASE_IMPORT_SPEC.md` appendix C. Unknown codes yield `std::nullopt`.
[[nodiscard]] std::optional<ImportCodeInfo> importCodeInfo(std::string_view code) noexcept;

/// Builds an import issue: `origin` is always `import`, and an empty `file`
/// leaves the location unset (the spec forbids "unknown" line numbers).
[[nodiscard]] core::ValidationIssue makeImportIssue(
    std::string_view code,
    std::string_view relativePath,
    int line = 0,
    int column = 0,
    std::string nodePath = {},
    std::string snippet = {},
    std::string detail = {});

/// Converts parser diagnostics of one dictionary into import issues.
///
/// The file label of the diagnostics is replaced by `relativePath`, and
/// `nodePathPrefix` (for example `boundaryField.movingWall`) is prepended to
/// the (currently empty) node path so the report can point at a dictionary path.
void appendDictionaryDiagnostics(
    std::vector<core::ValidationIssue>& issues,
    const std::vector<FoamDiagnostic>& diagnostics,
    std::string_view relativePath,
    std::string_view nodePathPrefix = {});

} // namespace ofgui::io
