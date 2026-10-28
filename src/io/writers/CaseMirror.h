#pragma once

#include "io/writers/CaseExportPlan.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>

namespace ofgui::io {

struct MirrorResult
{
    bool ok{false};
    std::size_t filesCopied{0};
    std::uintmax_t bytesCopied{0};
    std::string message;
};

/// Duplicates the `Copied` and `Mirrored` entries of an export plan.
///
/// Files are copied byte for byte: line endings are not normalised, `.gz`
/// payloads stay compressed and symbolic links are never followed
/// (`docs/CASE_IMPORT_SPEC.md` section 9.2).
class CaseMirror
{
public:
    CaseMirror() = default;

    /// Copies every copy entry below `stagingDirectory`, creating the parent
    /// directories as needed.
    [[nodiscard]] MirrorResult copyInto(
        const std::filesystem::path& stagingDirectory,
        const CaseExportPlan& plan) const;

    /// Streams one file; returns false and fills `message` on any error.
    [[nodiscard]] static bool copyFile(
        const std::filesystem::path& source,
        const std::filesystem::path& destination,
        std::string& message);
};

} // namespace ofgui::io
