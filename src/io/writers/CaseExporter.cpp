#include "io/writers/CaseExporter.h"

#include "io/ImportCodes.h"
#include "io/ImportIssueFactory.h"
#include "io/CaseSourceRecorder.h"
#include "io/writers/AtomicCaseDirectory.h"
#include "io/writers/ExportManifest.h"
#include "io/writers/ExportPlanBuilder.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace ofgui::io {
namespace {

/// True when `candidate` equals `ancestor` or lives below it.
bool isInside(const std::filesystem::path& candidate, const std::filesystem::path& ancestor)
{
    std::error_code error;
    const std::filesystem::path normalizedCandidate = std::filesystem::weakly_canonical(candidate, error);
    if (error)
    {
        return false;
    }
    const std::filesystem::path normalizedAncestor = std::filesystem::weakly_canonical(ancestor, error);
    if (error)
    {
        return false;
    }

    auto candidateIt = normalizedCandidate.begin();
    for (auto ancestorIt = normalizedAncestor.begin(); ancestorIt != normalizedAncestor.end(); ++ancestorIt)
    {
        if (candidateIt == normalizedCandidate.end() || *candidateIt != *ancestorIt)
        {
            return false;
        }
        ++candidateIt;
    }
    return true;
}

bool directoryHasContent(const std::filesystem::path& directory)
{
    std::error_code error;
    if (!std::filesystem::is_directory(directory, error) || error)
    {
        return false;
    }
    for (auto it = std::filesystem::directory_iterator(directory, error);
         !error && it != std::filesystem::directory_iterator();
         it.increment(error))
    {
        return true;
    }
    return false;
}

void appendIssue(std::vector<core::ValidationIssue>& issues, std::string_view code, std::string_view path)
{
    issues.push_back(makeImportIssue(code, path, 0, 0, std::string{path}, {}, {}));
}

} // namespace

bool CaseExportResult::hasBlockingIssues() const
{
    return std::any_of(issues.begin(), issues.end(), [](const core::ValidationIssue& issue)
                       { return issue.isBlocking(); });
}

bool CaseExportResult::hasCode(std::string_view code) const
{
    return std::any_of(issues.begin(), issues.end(), [code](const core::ValidationIssue& issue)
                       { return issue.code() == code; });
}

CaseExporter::CaseExporter(core::CaseValidator validator)
    : validator_(std::move(validator))
{
}

CaseExportResult CaseExporter::exportProject(
    const core::CaseProject& project,
    const CaseExportOptions& options) const
{
    CaseExportResult result;
    result.targetDirectory = options.targetDirectory;

    const core::ValidationResult validation = validator_.validate(project);
    result.issues = validation.issues();

    if (validation.hasBlockingErrors() || validation.hasErrors())
    {
        appendIssue(result.issues, importcodes::kExportRejectedByValidation, "project");
        return result;
    }

    if (!options.sourceCaseRoot.empty() && isInside(options.targetDirectory, options.sourceCaseRoot))
    {
        appendIssue(result.issues, importcodes::kExportTargetInsideSource, options.targetDirectory.generic_string());
        return result;
    }

    if (directoryHasContent(options.targetDirectory) && !options.allowExistingTarget)
    {
        appendIssue(result.issues, importcodes::kExportTargetExists, options.targetDirectory.generic_string());
        return result;
    }

    // The guard above already refused a non-empty target that the caller did not
    // confirm, so any target reaching this point is either empty or explicitly
    // replaceable. AtomicCaseDirectory drops an empty target without a backup and
    // moves a non-empty one aside, which keeps this call site branch-free.

    result.plan = ExportPlanBuilder::build(project, options, result.issues);
    // Planning can surface errors of its own (for example a dictionary that does
    // not parse); those must stop the export before anything is written.
    const bool planningFailed =
        std::any_of(result.issues.begin(), result.issues.end(), [](const core::ValidationIssue& issue)
                    { return issue.isAtLeast(core::Severity::Error); });
    if (planningFailed)
    {
        return result;
    }
    if (result.plan.empty())
    {
        appendIssue(result.issues, importcodes::kExportWriteFailed, options.targetDirectory.generic_string());
        return result;
    }

    // The manifest lists every planned file, so it is built after planning and
    // written into the staging directory before the rename.
    result.plan.manifestText = ExportManifest::build(result.plan, currentUtcTimestamp());

    AtomicCaseDirectory writer{options.targetDirectory};
    const CaseWriteStatus status = writer.write(result.plan, true);
    result.backupDirectory = writer.backupDirectory();

    switch (status)
    {
    case CaseWriteStatus::Success:
        break;
    case CaseWriteStatus::TargetNotWritable:
        appendIssue(result.issues, importcodes::kExportTargetNotWritable, options.targetDirectory.generic_string());
        return result;
    case CaseWriteStatus::WriteFailed:
        appendIssue(result.issues, importcodes::kExportWriteFailed, options.targetDirectory.generic_string());
        return result;
    case CaseWriteStatus::MirrorFailed:
        appendIssue(result.issues, importcodes::kExportMirrorFailed, options.targetDirectory.generic_string());
        return result;
    case CaseWriteStatus::ManifestFailed:
        appendIssue(result.issues, importcodes::kExportManifestFailed, options.targetDirectory.generic_string());
        return result;
    }

    result.fileCount = result.plan.size();
    result.totalBytes = result.plan.totalBytes();
    result.writtenCount = result.plan.count(ExportAction::Written);
    result.copiedCount = result.plan.count(ExportAction::Copied);
    result.mirroredCount = result.plan.count(ExportAction::Mirrored);
    result.skippedCount = result.plan.skipped.size();
    result.ok = true;
    return result;
}

} // namespace ofgui::io
