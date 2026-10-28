#include "io/CaseFieldReader.h"

#include "io/CaseProjectionContext.h"
#include "io/FieldProjector.h"
#include "io/FoamDictionary.h"
#include "io/FoamNode.h"
#include "io/ImportCodes.h"
#include "io/ImportIssueFactory.h"

#include <algorithm>
#include <string>

namespace ofgui::io {
namespace {

bool isBinaryFormat(const FoamDictionary& dictionary)
{
    const auto* foamHeader = dictionary.header();
    if (foamHeader == nullptr)
    {
        return false;
    }
    const auto* format = foamHeader->findChild("format");
    return format != nullptr && format->rawText() == "binary";
}

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

} // namespace

std::vector<FieldFile> CaseFieldReader::readAll(
    const std::filesystem::path& caseRoot,
    const CaseDirectoryScan& scan,
    const CaseImportOptions& options,
    std::vector<core::ValidationIssue>& issues)
{
    std::vector<FieldFile> files;
    if (!scan.hasStartDirectory)
    {
        return files;
    }

    FoamParseOptions parseOptions;
    parseOptions.maxFileBytes = options.maxFileBytes;

    for (const CaseScannedEntry& entry : scan.entries)
    {
        if (entry.kind != CaseEntryKind::Field || entry.directory || entry.symlink)
        {
            continue;
        }
        if (entry.relativePath.rfind(scan.startDirectory + "/", 0) != 0)
        {
            continue;
        }

        FieldFile file;
        file.relativePath = entry.relativePath;

        FoamDictionary dictionary;
        if (entry.byteSize > options.maxFileBytes)
        {
            issues.push_back(
                makeImportIssue(importcodes::kLimitFileTooLarge, entry.relativePath, 0, 0, {}, {}, std::to_string(entry.byteSize)));
            dictionary = FoamDictionary{};
        }
        else
        {
            dictionary = FoamDictionary::parseFile(caseRoot / std::filesystem::path(entry.relativePath), parseOptions);
        }

        const std::vector<FoamDiagnostic>& diagnostics = dictionary.diagnostics();
        appendDictionaryDiagnostics(issues, diagnostics, entry.relativePath);
        file.partial = hasIncludeDirective(diagnostics);
        file.binary = isBinaryFormat(dictionary);

        if (file.binary)
        {
            issues.push_back(makeImportIssue(importcodes::kFmtBinary, entry.relativePath, 0, 0, "FoamFile.format"));
        }
        if (hasParseError(diagnostics))
        {
            files.push_back(std::move(file));
            continue;
        }

        CaseProjectionContext context{entry.relativePath, issues, options};
        file.projection = FieldProjector::project(dictionary, context);
        file.headerMissing = file.projection.headerMissing;
        file.parsed = !file.headerMissing && !file.binary;
        files.push_back(std::move(file));
    }

    std::stable_sort(files.begin(), files.end(), [](const FieldFile& left, const FieldFile& right) {
        return left.relativePath < right.relativePath;
    });
    return files;
}

} // namespace ofgui::io
