#pragma once

#include <cstddef>

namespace ofgui::io {

/// Limits applied while scanning and importing a case (`docs/CASE_IMPORT_SPEC.md`
/// section 8.6). Every limit degrades gracefully: the importer reports it and
/// keeps whatever it already read.
struct CaseImportOptions
{
    /// Maximum number of entries listed below the case root (spec: 5000).
    std::size_t maxFiles{5000};
    /// Maximum number of issues kept in the report; the rest is summarised by
    /// `IMPORT-REPORT-001` (spec: 200).
    std::size_t maxIssues{200};
    /// Maximum size of a dictionary that is parsed (spec: 32 MiB).
    std::size_t maxFileBytes{32U * 1024U * 1024U};
    /// Reject a `blockMeshDict` with more than one block (spec section 4.7).
    bool requireSingleBlock{true};
};

} // namespace ofgui::io
