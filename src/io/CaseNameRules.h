#pragma once

#include "io/CaseDirectoryScan.h"

#include <string_view>

namespace ofgui::io::detail {

/// True for a directory name that parses as a non negative number (`0`, `0.5`,
/// `1e-05`). `latest` is handled by `isTimeDirectoryName`.
bool isNumericName(std::string_view name) noexcept;

/// Entry kind of a file below `system/`: everything but `PDRblockMeshDict` is a
/// dictionary the scan may parse.
CaseEntryKind kindForSystemFile(std::string_view name) noexcept;

} // namespace ofgui::io::detail
