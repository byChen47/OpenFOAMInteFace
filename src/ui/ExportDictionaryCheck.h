#pragma once

#include "core/CaseProject.h"
#include "core/ValidationIssue.h"
#include "io/writers/CaseExportPlan.h"

#include <vector>

namespace ofgui {

/// Stable codes of the UI export preflight (`docs/DECISIONS.md` D-038).
inline constexpr const char* kExportDictionaryInvalid = "EXPORT-DICT-001";
inline constexpr const char* kExportFileMissing = "EXPORT-DICT-002";

/// Validates the generated dictionaries of one export plan before anything is
/// written to disk.
///
/// The writers are deterministic, but a malformed value in the model (or a
/// later writer regression) must be caught here instead of only by OpenFOAM.
/// Every generated file is re-parsed with the same `io::FoamDictionary` the
/// importer uses, and the files every usable case needs must be present and
/// non-empty. Copy/mirror entries are source bytes and are not parsed.
[[nodiscard]] std::vector<core::ValidationIssue> checkExportPlan(
    const io::CaseExportPlan& plan,
    const core::CaseProject& project);

} // namespace ofgui
