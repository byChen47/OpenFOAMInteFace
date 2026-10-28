#pragma once

#include "core/CaseSourceInfo.h"
#include "io/CaseDirectoryScan.h"

#include <cstddef>
#include <filesystem>
#include <string>

namespace ofgui::io {

/// Current UTC time as ISO-8601 text (`2026-10-27T15:04:05Z`).
[[nodiscard]] std::string currentUtcTimestamp();

/// Outcome of hashing the imported files.
struct SourceRecording
{
    core::CaseSourceInfo info;
    std::size_t hashedFiles = 0;
    /// Files that could not be read (permissions, vanished) or exceed the limit.
    std::size_t skippedFiles = 0;
};

/// Records where a project came from: start directory plus one SHA-256 per file.
///
/// This is the t6 side of `docs/DECISIONS.md` D-025. `sourceRelativePath` must be
/// relative to the project file, so `CaseImporter` passes the case directory
/// name; persistence of the section itself is schemaVersion 2 (t17).
class CaseSourceRecorder
{
public:
    [[nodiscard]] static SourceRecording record(
        const std::filesystem::path& caseRoot,
        const CaseDirectoryScan& scan,
        std::string sourceRelativePath,
        std::size_t maxFileBytes = 32U * 1024U * 1024U);
};

} // namespace ofgui::io
