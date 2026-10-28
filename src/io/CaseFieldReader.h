#pragma once

#include "io/CaseDirectoryScan.h"
#include "io/CaseFieldFile.h"
#include "io/CaseImportOptions.h"

#include "core/ValidationIssue.h"

#include <filesystem>
#include <vector>

namespace ofgui::io {

/// Reads every field dictionary of the start time directory.
///
/// Parsing and projection only: the caller decides which patches exist and
/// whether a projected field enters the model, because the patch list must be
/// complete before boundary conditions can be attached.
class CaseFieldReader
{
public:
    [[nodiscard]] static std::vector<FieldFile> readAll(
        const std::filesystem::path& caseRoot,
        const CaseDirectoryScan& scan,
        const CaseImportOptions& options,
        std::vector<core::ValidationIssue>& issues);
};

} // namespace ofgui::io
