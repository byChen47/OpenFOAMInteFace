#pragma once

#include "io/writers/CaseExportPlan.h"

#include <string>
#include <string_view>

namespace ofgui::io {

/// Builds the `.ofgport-export.json` manifest of one export run.
///
/// The manifest lists every exported file with its action and byte count plus
/// the source files that were skipped, so the user can audit what an export did
/// (`docs/CASE_IMPORT_SPEC.md` section 9.5). It is written into the staging
/// directory before the directory rename and never lists itself.
class ExportManifest
{
public:
    /// `generatedAt` is an ISO-8601 UTC timestamp (`2026-10-27T14:30:00Z`).
    [[nodiscard]] static std::string build(
        const CaseExportPlan& plan,
        std::string_view generatedAt);
};

} // namespace ofgui::io
