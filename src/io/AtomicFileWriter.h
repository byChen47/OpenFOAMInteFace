#pragma once

#include <QByteArray>
#include <QString>

#include <filesystem>
#include <string_view>

namespace ofgui::io {

/// Why an atomic write did not produce the target file.
enum class AtomicWriteStatus
{
    Success,
    TargetNotWritable,
    WriteFailed,
};

struct AtomicWriteResult
{
    AtomicWriteStatus status{AtomicWriteStatus::Success};
    QString message;

    bool ok() const noexcept { return status == AtomicWriteStatus::Success; }
};

/// Writes a file by creating a temporary file next to the target and renaming
/// it over the target. A failed write never leaves a partially written target
/// behind: the target either keeps its previous content or does not exist.
class AtomicFileWriter
{
public:
    explicit AtomicFileWriter(std::filesystem::path target);

    const std::filesystem::path& target() const noexcept { return target_; }

    AtomicWriteResult write(std::string_view content) const;
    AtomicWriteResult write(const QByteArray& content) const;

private:
    std::filesystem::path target_;
};

} // namespace ofgui::io
