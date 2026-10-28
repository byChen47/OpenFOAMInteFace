#pragma once

#include "io/writers/CaseExportOptions.h"
#include "io/writers/CaseExportPlan.h"

#include <core/CaseProject.h>
#include <core/CaseValidator.h>
#include <core/ValidationIssue.h>

#include <cstddef>
#include <filesystem>
#include <vector>

namespace ofgui::io {

struct CaseExportResult
{
    bool ok{false};
    CaseExportPlan plan;
    std::vector<core::ValidationIssue> issues;
    std::filesystem::path targetDirectory;
    std::filesystem::path backupDirectory;
    /// Files in the plan (`written` + `copied` + `mirrored`).
    std::size_t fileCount{0};
    std::size_t totalBytes{0};
    std::size_t writtenCount{0};
    std::size_t copiedCount{0};
    std::size_t mirroredCount{0};
    std::size_t skippedCount{0};

    [[nodiscard]] bool hasBlockingIssues() const;
    [[nodiscard]] bool hasCode(std::string_view code) const;
};

/// Validates a project, plans the dictionaries and writes the case atomically.
///
/// Nothing is written when validation reports an error: the caller gets the
/// issue list and can show it in the validation panel.
class CaseExporter
{
public:
    CaseExporter() = default;
    explicit CaseExporter(core::CaseValidator validator);

    [[nodiscard]] CaseExportResult exportProject(
        const core::CaseProject& project,
        const CaseExportOptions& options) const;

private:
    core::CaseValidator validator_;
};

} // namespace ofgui::io
