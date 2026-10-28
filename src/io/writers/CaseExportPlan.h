#pragma once

#include "io/writers/ExportAction.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace ofgui::io {

/// One file of the exported case.
///
/// `Written` entries carry the generated `text` (LF line endings, final
/// newline); `Copied` and `Mirrored` entries carry `sourcePath` and are
/// duplicated byte for byte.
struct ExportFile
{
    std::string relativePath;
    std::string text;
    std::filesystem::path sourcePath;
    ExportAction action{ExportAction::Written};
    /// Generated size for `Written`, source size for the copy actions.
    std::uintmax_t byteSize{0};

    [[nodiscard]] bool isCopy() const noexcept
    {
        return action == ExportAction::Copied || action == ExportAction::Mirrored;
    }
};

/// A source file that is not part of the export (runtime product, other time
/// directory, symbolic link).
struct ExportSkippedFile
{
    std::string relativePath;
    std::string reason;
};

/// Everything one export run would write, plus the skip list.
struct CaseExportPlan
{
    /// `Written` entries first, then the copy entries sorted by path.
    std::vector<ExportFile> files;
    std::vector<ExportSkippedFile> skipped;
    /// Payload of `.ofgport-export.json`; empty disables the manifest.
    std::string manifestText;

    [[nodiscard]] bool empty() const noexcept { return files.empty(); }
    [[nodiscard]] std::size_t size() const noexcept { return files.size(); }
    [[nodiscard]] std::size_t totalBytes() const noexcept;
    [[nodiscard]] std::size_t count(ExportAction action) const noexcept;
    [[nodiscard]] const ExportFile* find(std::string_view relativePath) const noexcept;
};

} // namespace ofgui::io
