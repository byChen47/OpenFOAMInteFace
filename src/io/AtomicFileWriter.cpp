#include "io/AtomicFileWriter.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace ofgui::io {
namespace {

std::string randomSuffix()
{
    static std::atomic<std::uint64_t> counter{0};
    std::random_device device;
    const auto now = static_cast<std::uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    const auto mixed = now
        ^ (static_cast<std::uint64_t>(device()) << 21)
        ^ (static_cast<std::uint64_t>(device()) << 42)
        ^ counter.fetch_add(0x9e3779b97f4a7c15ULL);

    static constexpr char kHexDigits[] = "0123456789abcdef";
    std::string suffix(16, '0');
    for (int i = 15; i >= 0; --i)
    {
        suffix[static_cast<std::size_t>(i)] = kHexDigits[mixed >> (4 * (15 - i)) & 0xF];
    }
    return suffix;
}

QString fromPath(const std::filesystem::path& path)
{
    return QString::fromStdString(path.string());
}

AtomicWriteResult failure(AtomicWriteStatus status, QString message)
{
    AtomicWriteResult result;
    result.status = status;
    result.message = std::move(message);
    return result;
}

} // namespace

AtomicFileWriter::AtomicFileWriter(std::filesystem::path target)
    : target_(std::move(target))
{
}

AtomicWriteResult AtomicFileWriter::write(std::string_view content) const
{
    if (target_.empty())
    {
        return failure(
            AtomicWriteStatus::TargetNotWritable,
            QStringLiteral("no target path was given"));
    }

    std::error_code error;
    std::filesystem::path directory = target_.parent_path();
    if (directory.empty())
    {
        directory = ".";
    }
    if (!std::filesystem::exists(directory, error) || error)
    {
        error.clear();
        std::filesystem::create_directories(directory, error);
        if (error)
        {
            return failure(
                AtomicWriteStatus::TargetNotWritable,
                QStringLiteral("cannot create the directory '%1': %2")
                    .arg(fromPath(directory), QString::fromStdString(error.message())));
        }
    }

    error.clear();
    if (std::filesystem::is_directory(target_, error) && !error)
    {
        return failure(
            AtomicWriteStatus::TargetNotWritable,
            QStringLiteral("'%1' is a directory").arg(fromPath(target_)));
    }

    const std::filesystem::path temporary =
        directory / (target_.filename().string() + ".tmp-" + randomSuffix());

    {
        std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
        if (!stream)
        {
            return failure(
                AtomicWriteStatus::WriteFailed,
                QStringLiteral("cannot create the temporary file '%1'").arg(fromPath(temporary)));
        }

        stream.write(content.data(), static_cast<std::streamsize>(content.size()));
        stream.flush();
        if (!stream)
        {
            stream.close();
            std::filesystem::remove(temporary, error);
            return failure(
                AtomicWriteStatus::WriteFailed,
                QStringLiteral("cannot write the temporary file '%1'").arg(fromPath(temporary)));
        }
    }

    // Keep the previous permission bits (the temporary file is created with the
    // process defaults, which may be more permissive than the target).
    error.clear();
    const std::filesystem::file_status existing = std::filesystem::status(target_, error);
    if (!error && std::filesystem::exists(existing))
    {
        std::filesystem::permissions(temporary, existing.permissions(), error);
    }

    error.clear();
    std::filesystem::rename(temporary, target_, error);
    if (error)
    {
        // Replacing an existing file is not atomic on every platform. Never
        // delete the old file before the new one is in place: move it aside,
        // retry, and restore it if the retry fails.
        const std::filesystem::path backup =
            directory / (target_.filename().string() + ".bak-" + randomSuffix());

        std::error_code moveError;
        std::filesystem::rename(target_, backup, moveError);
        if (moveError)
        {
            std::error_code cleanupError;
            std::filesystem::remove(temporary, cleanupError);
            return failure(
                AtomicWriteStatus::WriteFailed,
                QStringLiteral("cannot replace '%1': %2")
                    .arg(fromPath(target_), QString::fromStdString(moveError.message())));
        }

        error.clear();
        std::filesystem::rename(temporary, target_, error);
        if (error)
        {
            std::error_code cleanupError;
            std::filesystem::remove(temporary, cleanupError);

            std::error_code restoreError;
            std::filesystem::rename(backup, target_, restoreError);
            if (restoreError)
            {
                return failure(
                    AtomicWriteStatus::WriteFailed,
                    QStringLiteral("cannot replace '%1': %2; the previous file is kept at '%3'")
                        .arg(
                            fromPath(target_),
                            QString::fromStdString(error.message()),
                            fromPath(backup)));
            }
            return failure(
                AtomicWriteStatus::WriteFailed,
                QStringLiteral("cannot replace '%1': %2")
                    .arg(fromPath(target_), QString::fromStdString(error.message())));
        }

        std::error_code cleanupError;
        std::filesystem::remove(backup, cleanupError);
    }

    return AtomicWriteResult{};
}

AtomicWriteResult AtomicFileWriter::write(const QByteArray& content) const
{
    return write(std::string_view(content.constData(), static_cast<std::size_t>(content.size())));
}

} // namespace ofgui::io
