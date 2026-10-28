#include "io/CaseImporter.h"

#include "io/ImportDictionaryLoader.h"
#include "io/ImportReportBookkeeping.h"

#include "io/BlockMeshProjector.h"
#include "io/CaseDirectoryScan.h"
#include "io/CaseFieldApplier.h"
#include "io/CaseFieldReader.h"
#include "io/CaseProjectionContext.h"
#include "io/CaseScanner.h"
#include "io/CaseSourceRecorder.h"
#include "io/ControlDictProjector.h"
#include "io/FoamDictionary.h"
#include "io/ImportCodes.h"
#include "io/ImportIssueFactory.h"
#include "io/ReadOnlyDictionaryProjector.h"
#include "io/TransportPropertiesProjector.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace ofgui::io {
using detail::defaultLevel;
using detail::fileNameOf;
using detail::findFile;
using detail::parseDictionary;
using detail::sortIssues;


CaseImporter::CaseImporter(CaseImportOptions options) noexcept
    : options_(options)
{
}

CaseImportResult CaseImporter::import(const std::filesystem::path& caseRoot) const
{
    CaseImportResult result;
    ImportReport& report = result.report;

    const CaseDirectoryScan scan = CaseScanner(options_).scan(caseRoot);
    report.caseRoot = scan.caseRoot;
    report.startDirectory = scan.startDirectory;

    if (!scan.hasCaseRoot)
    {
        report.issues.push_back(makeImportIssue(importcodes::kCaseRootMissing, scan.caseRoot, 0, 0, {}, {}, scan.caseRoot));
        return result;
    }

    for (const CaseScannedEntry& entry : scan.entries)
    {
        ImportedFile file;
        file.relativePath = entry.relativePath;
        file.kind = std::string(caseEntryKindName(entry.kind));
        file.level = defaultLevel(entry.kind);
        file.byteSize = static_cast<std::size_t>(entry.byteSize);
        file.gzip = entry.gzip;
        report.files.push_back(std::move(file));
    }
    if (scan.truncated)
    {
        report.issues.push_back(makeImportIssue(importcodes::kReportTooManyFiles, scan.caseRoot, 0, 0, {}, {}, std::to_string(options_.maxFiles)));
    }

    // system/controlDict: required
    if (!scan.hasControlDict)
    {
        report.issues.push_back(makeImportIssue(importcodes::kCaseMissingControlDict, "system", 0, 0, {}, {}, "controlDict"));
    }
    else
    {
        bool partial = false;
        bool failed = false;
        if (std::optional<FoamDictionary> dictionary = parseDictionary(
                caseRoot, options_, "system/controlDict", report.issues, partial, failed))
        {
            const CaseProjectionContext context{"system/controlDict", report.issues, options_};
            ControlDictProjection projection = ControlDictProjector::project(*dictionary, context);
            result.project.setSolver(projection.solver);
            result.readOnlySettings.insert(result.readOnlySettings.end(),
                                           std::make_move_iterator(projection.settings.begin()),
                                           std::make_move_iterator(projection.settings.end()));
            result.partial = result.partial || partial;
        }
        else
        {
            // Without a readable controlDict there is no solver setting to show,
            // so the import cannot produce a usable project.
            report.issues.push_back(makeImportIssue(
                importcodes::kCaseUnreadableControlDict, "system/controlDict", 0, 0, {}, {}));
        }
        if (ImportedFile* file = findFile(report, "system/controlDict"))
        {
            file->level = partial || failed ? SupportLevel::ReadOnly : SupportLevel::Editable;
            file->partial = partial;
            file->hasParseErrors = failed;
        }
    }

    // system/blockMeshDict: single hex block defines domain, mesh and patches
    BlockMeshProjection blockMesh;
    bool hasBlockMesh = false;
    if (scan.hasBlockMeshDict)
    {
        bool partial = false;
        bool failed = false;
        if (std::optional<FoamDictionary> dictionary = parseDictionary(
                caseRoot, options_, "system/blockMeshDict", report.issues, partial, failed))
        {
            const CaseProjectionContext context{"system/blockMeshDict", report.issues, options_};
            blockMesh = BlockMeshProjector::project(*dictionary, context);
            hasBlockMesh = blockMesh.hasDomain;
            if (blockMesh.hasDomain)
            {
                result.project.setDomain(blockMesh.domain);
            }
            if (blockMesh.hasMesh)
            {
                result.project.setMesh(blockMesh.mesh);
            }
            result.partial = result.partial || partial;
            if (ImportedFile* file = findFile(report, "system/blockMeshDict"))
            {
                file->level = SupportLevel::Editable;
                file->partial = partial;
                file->hasParseErrors = failed;
            }
        }
    }
    if (findFile(report, "system/PDRblockMeshDict") != nullptr)
    {
        if (ImportedFile* file = findFile(report, "system/PDRblockMeshDict"))
        {
            file->level = SupportLevel::Recognized;
        }
        if (scan.hasBlockMeshDict)
        {
            report.issues.push_back(makeImportIssue(importcodes::kExportPdrCoexists, "system", 0, 0, {}, {}, "PDRblockMeshDict"));
        }
    }

    // constant/
    if (!scan.hasConstantDirectory)
    {
        report.issues.push_back(makeImportIssue(importcodes::kCaseMissingConstant, "constant", 0, 0, {}, {}, "constant"));
    }
    else
    {
        bool partial = false;
        bool failed = false;
        if (std::optional<FoamDictionary> dictionary = parseDictionary(
                caseRoot, options_, "constant/transportProperties", report.issues, partial, failed))
        {
            const CaseProjectionContext context{"constant/transportProperties", report.issues, options_};
            result.transport = TransportPropertiesProjector::project(*dictionary, context);
            result.readOnlySettings.insert(result.readOnlySettings.end(),
                                           result.transport.settings.begin(),
                                           result.transport.settings.end());
            result.partial = result.partial || partial;
            if (!result.transport.hasTransportModel && !failed)
            {
                // OpenFOAM defaults to Newtonian when the keyword is absent.
                report.issues.push_back(makeImportIssue(
                    importcodes::kTransportModelDefaulted,
                    "constant/transportProperties",
                    0,
                    0,
                    "transportModel",
                    {},
                    "Newtonian"));
            }
            if (ImportedFile* file = findFile(report, "constant/transportProperties"))
            {
                file->level = partial ? SupportLevel::ReadOnly : SupportLevel::Editable;
                file->partial = partial;
                file->hasParseErrors = failed;
            }
        }
        if (!scan.hasTurbulenceProperties)
        {
            report.issues.push_back(makeImportIssue(importcodes::kTurbulenceMissing, "constant", 0, 0, {}, {}, "laminar"));
        }
    }

    // system/ read-only dictionaries
    for (const std::string_view name : {"system/fvSchemes", "system/fvSolution", "system/decomposeParDict"})
    {
        bool partial = false;
        bool failed = false;
        if (std::optional<FoamDictionary> dictionary =
                parseDictionary(caseRoot, options_, name, report.issues, partial, failed))
        {
            const std::vector<ImportedSetting> settings = ReadOnlyDictionaryProjector::flatten(*dictionary, name);
            result.readOnlySettings.insert(result.readOnlySettings.end(), settings.begin(), settings.end());
            result.partial = result.partial || partial;
            if (ImportedFile* file = findFile(report, name))
            {
                file->level = SupportLevel::ReadOnly;
                file->partial = partial;
                file->hasParseErrors = failed;
            }
        }
    }
    {
        bool partial = false;
        bool failed = false;
        if (std::optional<FoamDictionary> dictionary = parseDictionary(
                caseRoot, options_, "constant/turbulenceProperties", report.issues, partial, failed))
        {
            const std::vector<ImportedSetting> settings =
                ReadOnlyDictionaryProjector::flatten(*dictionary, "constant/turbulenceProperties");
            result.readOnlySettings.insert(result.readOnlySettings.end(), settings.begin(), settings.end());
            result.partial = result.partial || partial;
            if (ImportedFile* file = findFile(report, "constant/turbulenceProperties"))
            {
                file->level = SupportLevel::ReadOnly;
                file->partial = partial;
                file->hasParseErrors = failed;
            }
        }
    }

    // start time directory fields
    if (!scan.hasStartDirectory)
    {
        report.issues.push_back(makeImportIssue(importcodes::kCaseMissingStartDirectory, {}, 0, 0, {}, {}, "0"));
    }
    else
    {
        const std::vector<FieldFile> fields = CaseFieldReader::readAll(caseRoot, scan, options_, report.issues);
        for (const FieldFile& field : fields)
        {
            if (ImportedFile* file = findFile(report, field.relativePath))
            {
                file->objectClass = field.projection.classText;
                file->partial = field.partial;
                file->hasParseErrors = !field.parsed;
                if (field.binary || !field.projection.fieldClass.has_value())
                {
                    file->level = SupportLevel::Unsupported;
                }
                else
                {
                    file->level = (field.projection.name == "p" || field.projection.name == "U")
                        ? SupportLevel::Editable
                        : SupportLevel::ReadOnly;
                }
            }
            result.partial = result.partial || field.partial;
        }
        CaseFieldApplier::apply(fields, hasBlockMesh ? &blockMesh : nullptr, result.project, report);
    }

    // Source metadata for D-025; persistence is schemaVersion 2 (t17).
    const std::string caseName = fileNameOf(std::filesystem::path(scan.caseRoot).generic_string());
    const SourceRecording recording =
        CaseSourceRecorder::record(caseRoot, scan, caseName, options_.maxFileBytes);
    result.project.setSource(recording.info);

    // Issue hygiene: order as specified, then cap.
    sortIssues(report.issues);
    if (report.issues.size() > options_.maxIssues)
    {
        report.issues.resize(options_.maxIssues - 1);
        report.issues.push_back(makeImportIssue(importcodes::kReportTruncated, {}, 0, 0, {}, {}, std::to_string(options_.maxIssues)));
    }

    result.ok = !report.blocking();
    return result;
}

} // namespace ofgui::io