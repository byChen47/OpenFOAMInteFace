#pragma once

#include "io/CaseImportOptions.h"
#include "io/FoamDiagnostic.h"

#include <core/ValidationIssue.h>

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ofgui::io {

class FoamDictionary;

namespace detail {

/// True when the parser recorded an unexpanded `#include*` or another directive.
bool hasIncludeDirective(const std::vector<FoamDiagnostic>& diagnostics);

/// True when the parser reported at least one error.
bool hasParseError(const std::vector<FoamDiagnostic>& diagnostics);

/// Parses one dictionary, reporting size limits and parser diagnostics.
std::optional<FoamDictionary> parseDictionary(
    const std::filesystem::path& caseRoot,
    const CaseImportOptions& options,
    std::string_view relativePath,
    std::vector<core::ValidationIssue>& issues,
    bool& partial,
    bool& failed);

/// Last path component of a case relative path.
std::string fileNameOf(std::string_view relativePath);

} // namespace detail

} // namespace ofgui::io
