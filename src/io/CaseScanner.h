#pragma once

#include "io/CaseDirectoryScan.h"
#include "io/CaseImportOptions.h"

#include <filesystem>
#include <string_view>

namespace ofgui::io {

/// True when `name` looks like an OpenFOAM time directory (`0`, `0.5`, `1e-05`,
/// `latest`). Only the name is inspected; nothing inside the directory is read.
[[nodiscard]] bool isTimeDirectoryName(std::string_view name) noexcept;

/// Walks a case root and lists every file the importer may care about.
///
/// The scanner never parses a dictionary and never follows symbolic links: it
/// answers "what is there and how big is it", while `CaseImporter` decides what
/// each file means. Nothing is written, so a scan cannot modify the source case.
class CaseScanner
{
public:
    CaseScanner() = default;

    explicit CaseScanner(CaseImportOptions options) noexcept;

    /// @param caseRoot existing directory; a missing or non directory root yields
    ///        `hasCaseRoot == false` and no entries.
    [[nodiscard]] CaseDirectoryScan scan(const std::filesystem::path& caseRoot) const;

private:
    CaseImportOptions options_;
};

} // namespace ofgui::io
