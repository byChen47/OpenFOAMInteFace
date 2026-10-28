#pragma once

#include "io/writers/CaseExportOptions.h"
#include "io/writers/CaseExportPlan.h"

#include <core/CaseProject.h>
#include <core/ValidationIssue.h>

#include <vector>

namespace ofgui::io {

/// Turns a project plus export options into the complete list of files that
/// would be written. No file system access happens here.
class ExportPlanBuilder
{
public:
    static CaseExportPlan build(
        const core::CaseProject& project,
        const CaseExportOptions& options,
        std::vector<core::ValidationIssue>& issues);
};

} // namespace ofgui::io
