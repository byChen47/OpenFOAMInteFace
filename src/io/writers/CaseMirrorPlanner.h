#pragma once

#include "io/writers/CaseExportOptions.h"
#include "io/writers/CaseExportPlan.h"

#include <core/CaseProject.h>
#include <core/ValidationIssue.h>

#include <vector>

namespace ofgui::io {

/// Collects the source files that the export copies instead of generating.
///
/// Paths produced by a dictionary writer are never copied, runtime products go
/// to the `skipped` list, files of the start directory and the recognized read
/// only dictionaries become `Copied` entries and every other source file
/// (`constant/polyMesh/**`, unknown dictionaries) becomes `Mirrored`.
class CaseMirrorPlanner
{
public:
    /// Appends to `plan.files` (copies) and `plan.skipped`; requires
    /// `options.sourceCaseRoot` to point at the source case, otherwise nothing
    /// is collected.
    static void collect(
        const core::CaseProject& project,
        const CaseExportOptions& options,
        CaseExportPlan& plan,
        std::vector<core::ValidationIssue>& issues);
};

} // namespace ofgui::io
