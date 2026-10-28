#pragma once

#include "io/CaseDirectoryScan.h"
#include "io/CaseSupportLevel.h"
#include "io/ImportReport.h"

#include <core/ValidationIssue.h>

#include <string_view>
#include <vector>

namespace ofgui::io::detail {

/// Orders issues for display: severity (worst first), then file, line and code.
void sortIssues(std::vector<core::ValidationIssue>& issues);

/// Finds the report entry of one relative path, or nullptr.
ImportedFile* findFile(ImportReport& report, std::string_view relativePath);

/// Default support level for a scanned entry kind.
SupportLevel defaultLevel(CaseEntryKind kind) noexcept;

} // namespace ofgui::io::detail
