#pragma once

#include "io/CaseDirectoryScan.h"
#include "io/CaseSupportLevel.h"

#include "core/ValidationIssue.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace ofgui::io {

/// One file the scanner listed and the importer classified.
struct ImportedFile
{
    /// Case relative POSIX path, e.g. `0/p` or `system/blockMeshDict`.
    std::string relativePath;
    /// `field`, `dictionary`, `mesh-dir`, `script` or `other`
    /// (see `caseEntryKindName`).
    std::string kind;
    SupportLevel level = SupportLevel::Recognized;
    std::size_t byteSize = 0;
    bool gzip = false;
    /// `FoamFile.class` when it could be read, otherwise empty.
    std::string objectClass;
    /// True when the dictionary contains an `#include`-style directive that the
    /// parser does not expand (spec section 5.5).
    bool partial = false;
    /// True when the parser reported errors for this file.
    bool hasParseErrors = false;
};

/// Everything the import found, shown by the import report panel and asserted by
/// the tests (`docs/CASE_IMPORT_SPEC.md` section 6.7).
///
/// `caseRoot` is the absolute path for display only: it is never persisted into
/// the project file.
struct ImportReport
{
    std::string caseRoot;
    std::string startDirectory;
    /// `v2206` style banner note when the header carried one; may be empty.
    std::string foamVersionNote;
    std::vector<ImportedFile> files;
    std::vector<core::ValidationIssue> issues;
    /// True when at least one `BlockingError` was reported.
    [[nodiscard]] bool blocking() const;
    /// True when any issue carries `code`.
    [[nodiscard]] bool hasCode(std::string_view code) const;
    /// Number of issues carrying `code`.
    [[nodiscard]] std::size_t countCode(std::string_view code) const;
    /// Number of listed files with `level`.
    [[nodiscard]] std::size_t countFiles(SupportLevel level) const;
    /// Number of listed files with `partial == true`.
    [[nodiscard]] std::size_t partialCount() const;
    /// Relative paths of all listed files with `level`.
    [[nodiscard]] std::vector<std::string> pathsWithLevel(SupportLevel level) const;
    /// The listed file for `relativePath`, or nullptr.
    [[nodiscard]] const ImportedFile* findFile(std::string_view relativePath) const;
};

} // namespace ofgui::io
