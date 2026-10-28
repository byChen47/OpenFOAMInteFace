#pragma once

#include "io/CaseImportOptions.h"

#include "core/ValidationIssue.h"

#include <string>
#include <vector>

namespace ofgui::io {

/// Shared input of every dictionary projector.
///
/// A projector never touches the file system: it receives the AST plus this
/// context and appends its findings. That keeps `CaseImporter` the only place
/// that reads or (later) writes a case.
struct CaseProjectionContext
{
    /// Case relative POSIX path of the dictionary, e.g. `system/blockMeshDict`.
    std::string relativePath;
    std::vector<core::ValidationIssue>& issues;
    const CaseImportOptions& options;
};

} // namespace ofgui::io
