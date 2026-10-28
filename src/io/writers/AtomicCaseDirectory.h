#pragma once

#include "io/writers/CaseExportPlan.h"

#include <filesystem>
#include <string>

namespace ofgui::io {

enum class CaseWriteStatus
{
    Success,
    TargetNotWritable,
    WriteFailed,
    /// Copying a source file into the staging directory failed.
    MirrorFailed,
    /// Writing `.ofgport-export.json` failed.
    ManifestFailed,
};

/// Writes a whole case into a temporary sibling directory and then renames it
/// into place, so a failed export never leaves a half written case behind.
class AtomicCaseDirectory
{
public:
    explicit AtomicCaseDirectory(std::filesystem::path target);

    /// Writes every planned file below `<target>.tmp-<pid>-<stamp>` and moves
    /// the directory to the target. An existing target is renamed to
    /// `<target>.bak-<stamp>` first when `replaceExisting` is true.
    CaseWriteStatus write(const CaseExportPlan& plan, bool replaceExisting);

    [[nodiscard]] const std::filesystem::path& target() const noexcept { return target_; }
    [[nodiscard]] const std::filesystem::path& backupDirectory() const noexcept { return backup_; }
    [[nodiscard]] const std::string& message() const noexcept { return message_; }
    [[nodiscard]] std::string tempDirectoryNameForTesting() const { return temp_.string(); }

private:
    std::filesystem::path target_;
    std::filesystem::path temp_;
    std::filesystem::path backup_;
    std::string message_;
};

} // namespace ofgui::io
