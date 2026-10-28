#include "io/ImportDictionaryLoader.h"

#include "io/FoamDictionary.h"
#include "io/FoamParseOptions.h"
#include "io/ImportCodes.h"
#include "io/ImportIssueFactory.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <system_error>

namespace ofgui::io::detail {
namespace {

using core::ValidationIssue;

} // namespace

bool hasIncludeDirective(const std::vector<FoamDiagnostic>& diagnostics)
{
    return std::any_of(diagnostics.begin(), diagnostics.end(), [](const FoamDiagnostic& diagnostic) {
        return diagnostic.code == "IMPORT-DIR-001" || diagnostic.code == "IMPORT-DIR-002"
            || diagnostic.code == "IMPORT-DIR-003";
    });
}

bool hasParseError(const std::vector<FoamDiagnostic>& diagnostics)
{
    return std::any_of(diagnostics.begin(), diagnostics.end(), [](const FoamDiagnostic& diagnostic) {
        return diagnostic.severity == FoamSeverity::Error;
    });
}

std::optional<FoamDictionary> parseDictionary(
    const std::filesystem::path& caseRoot,
    const CaseImportOptions& options,
    std::string_view relativePath,
    std::vector<ValidationIssue>& issues,
    bool& partial,
    bool& failed)
{
    failed = false;
    const std::filesystem::path path = caseRoot / std::filesystem::path(relativePath);
    std::error_code ec;
    if (!std::filesystem::is_regular_file(path, ec) || ec)
    {
        return std::nullopt;
    }
    const std::uintmax_t size = std::filesystem::file_size(path, ec);
    if (!ec && size > options.maxFileBytes)
    {
        issues.push_back(
            makeImportIssue(importcodes::kLimitFileTooLarge, relativePath, 0, 0, {}, {}, std::to_string(size)));
        failed = true;
        return std::nullopt;
    }

    FoamParseOptions parseOptions;
    parseOptions.maxFileBytes = options.maxFileBytes;
    FoamDictionary dictionary = FoamDictionary::parseFile(path, parseOptions);
    appendDictionaryDiagnostics(issues, dictionary.diagnostics(), relativePath);
    partial = partial || hasIncludeDirective(dictionary.diagnostics());
    if (hasParseError(dictionary.diagnostics()))
    {
        failed = true;
        return std::nullopt;
    }
    return dictionary;
}

std::string fileNameOf(std::string_view relativePath)
{
    const std::size_t slash = relativePath.find_last_of('/');
    return std::string(slash == std::string_view::npos ? relativePath : relativePath.substr(slash + 1));
}

} // namespace ofgui::io::detail
