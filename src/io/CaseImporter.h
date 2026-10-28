#pragma once

#include "io/CaseImportOptions.h"
#include "io/CaseImportResult.h"

#include <filesystem>

namespace ofgui::io {

/// Reads an existing OpenFOAM case directory into a `CaseProject`.
///
/// `docs/CASE_IMPORT_SPEC.md` sections 4–6: the importer scans, parses with the
/// `Foam*` parser, projects into the model and reports every degradation as a
/// `core::ValidationIssue` with origin `import`. It never writes to the case
/// directory — hashing files is read only as well.
class CaseImporter
{
public:
    CaseImporter() = default;

    explicit CaseImporter(CaseImportOptions options) noexcept;

    [[nodiscard]] CaseImportResult import(const std::filesystem::path& caseRoot) const;

private:
    CaseImportOptions options_;
};

} // namespace ofgui::io
