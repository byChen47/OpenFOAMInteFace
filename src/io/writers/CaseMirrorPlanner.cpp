#include "io/writers/CaseMirrorPlanner.h"

#include "io/ImportCodes.h"
#include "io/ImportIssueFactory.h"
#include "io/writers/ExportSkipRules.h"
#include "io/writers/SourceCaseWalker.h"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ofgui::io {
namespace {

std::string joinPath(std::string_view directory, std::string_view name)
{
    if (directory.empty())
    {
        return std::string{name};
    }
    return std::string{directory} + "/" + std::string{name};
}

bool startsWithDirectory(std::string_view path, std::string_view directory)
{
    return !directory.empty() && path.size() > directory.size() + 1
        && path.compare(0, directory.size(), directory) == 0 && path[directory.size()] == '/';
}

/// Rewrites `<sourceStart>/x` to `<exportStart>/x` when the exported start
/// directory differs from the source one (spec 9.3 inherits it by default).
std::string mapStartDirectory(
    std::string_view path,
    std::string_view sourceStart,
    std::string_view exportStart)
{
    if (!startsWithDirectory(path, sourceStart) || sourceStart == exportStart)
    {
        return std::string{path};
    }
    const std::string_view remainder = path.substr(sourceStart.size() + 1);
    if (exportStart.empty())
    {
        return std::string{remainder};
    }
    return std::string{exportStart} + "/" + std::string{remainder};
}

/// Recognized but not model driven: preserved verbatim, never regenerated.
bool isCopiedDictionary(std::string_view path)
{
    return path == "constant/turbulenceProperties" || path == "system/decomposeParDict";
}

} // namespace

void CaseMirrorPlanner::collect(
    const core::CaseProject& project,
    const CaseExportOptions& options,
    CaseExportPlan& plan,
    std::vector<core::ValidationIssue>& issues)
{
    if (options.sourceCaseRoot.empty())
    {
        return;
    }

    const std::filesystem::path sourceRoot{options.sourceCaseRoot};
    const SourceCaseWalk walk = SourceCaseWalker{sourceRoot}.walk();
    if (walk.files.empty() && walk.startDirectory.empty())
    {
        issues.push_back(makeImportIssue(importcodes::kCaseRootMissing, options.sourceCaseRoot));
        return;
    }

    const std::string exportStart = options.startDirectory.empty() ? std::string{"0"} : options.startDirectory;

    std::set<std::string> takenOver;
    for (const ExportFile& file : plan.files)
    {
        takenOver.insert(file.relativePath);
    }
    for (const core::FieldDefinition& field : project.fields())
    {
        takenOver.insert(joinPath(exportStart, field.name()));
    }

    const ExportSkipRules rules{
        CaseExportSkipPolicy{options.keepRunScripts, options.mirrorOtherTimeDirectories}};

    std::set<std::string> planned;
    for (const SourceCaseFile& file : walk.files)
    {
        const ExportSkipReason reason = rules.classify(file.relativePath, walk.startDirectory);
        if (reason != ExportSkipReason::None)
        {
            plan.skipped.push_back(ExportSkippedFile{file.relativePath, std::string{exportSkipReasonName(reason)}});
            continue;
        }

        const std::string target = mapStartDirectory(file.relativePath, walk.startDirectory, exportStart);
        if (takenOver.count(target) != 0 || !planned.insert(target).second)
        {
            continue;
        }

        ExportFile entry;
        entry.relativePath = target;
        entry.sourcePath = file.absolutePath;
        entry.byteSize = file.byteSize;
        entry.action = (file.inStartDirectory || isCopiedDictionary(file.relativePath))
            ? ExportAction::Copied
            : ExportAction::Mirrored;
        plan.files.push_back(std::move(entry));
    }

    for (const std::string& link : walk.symlinks)
    {
        plan.skipped.push_back(
            ExportSkippedFile{link, std::string{exportSkipReasonName(ExportSkipReason::SymbolicLink)}});
    }

    if (walk.truncated || !walk.symlinks.empty())
    {
        issues.push_back(makeImportIssue(
            importcodes::kExportMirrorIncomplete,
            options.sourceCaseRoot,
            0,
            0,
            {},
            {},
            std::to_string(walk.symlinks.size()) + " symbolic link(s), "
                + (walk.truncated ? std::string{"file limit reached"} : std::string{"all files listed"})));
    }

    std::sort(plan.skipped.begin(), plan.skipped.end(), [](const ExportSkippedFile& left, const ExportSkippedFile& right)
              { return left.relativePath < right.relativePath; });
}

} // namespace ofgui::io
