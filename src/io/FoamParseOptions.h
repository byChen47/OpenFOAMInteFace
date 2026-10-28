#pragma once

#include <cstddef>

namespace ofgui::io {

/// Tunables for `FoamDictionary::parse` / `FoamDictionary::parseFile`.
///
/// The defaults follow docs/CASE_IMPORT_SPEC.md §5: recover what can be
/// recovered, keep at most 200 issues, and require a `FoamFile` header because
/// every v2206 dictionary shipped with OpenFOAM starts with one.
struct FoamParseOptions
{
    /// Hard cap on reported issues (spec §5.3). Extra issues are dropped and a
    /// single `IMPORT-REPORT-001` info diagnostic records the truncation.
    std::size_t maxDiagnostics{200};

    /// When true, a dictionary whose first entry is not `FoamFile` produces the
    /// `IMPORT-PARSE-003` error required by spec §5.1. Header-less fragments
    /// (`#include` targets, unit-test snippets) should disable this.
    bool requireFoamFileHeader{true};

    /// Enable the forward-recovery heuristic for a missing `;`: when a value is
    /// already present and the next token starts a new entry (`word {` or
    /// `word ;` on a later line), close the current entry and report
    /// `IMPORT-PARSE-001` instead of swallowing the following entry.
    bool recoverMissingSemicolon{true};

    /// Files larger than this are rejected with `IMPORT-IO-001` (spec §8).
    std::size_t maxFileBytes{32 * 1024 * 1024};

    /// Maximum `{`/`(` nesting depth. Deeper input is reported as
    /// `IMPORT-PARSE-009` and the offending block is skipped instead of being
    /// recursed into, which keeps a hostile or garbled file from overflowing the
    /// stack. Real v2206 dictionaries stay below 10 levels.
    std::size_t maxNestingDepth{64};
};

} // namespace ofgui::io
