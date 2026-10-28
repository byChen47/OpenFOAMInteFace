#include "io/writers/AtomicCaseDirectory.h"

#include "io/writers/CaseMirror.h"
#include "io/writers/ExportManifestSchema.h"

#include <chrono>
#include <cstddef>
#include <ctime>
#include <fstream>
#include <string>
#include <system_error>
#include <utility>

namespace ofgui::io {
namespace {

std::string stamp()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t seconds = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
#if defined(_WIN32)
    localtime_s(&local, &seconds);
#else
    localtime_r(&seconds, &local);
#endif
    char buffer[32];
    const std::size_t written = std::strftime(buffer, sizeof(buffer), "%Y%m%d-%H%M%S", &local);
    return std::string(buffer, written);
}

bool writeTextFile(const std::filesystem::path& path, const std::string& text, std::string& message)
{
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error)
    {
        message = "cannot create directory '" + path.parent_path().string() + "': " + error.message();
        return false;
    }

    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream)
    {
        message = "cannot open '" + path.string() + "' for writing";
        return false;
    }

    stream.write(text.data(), static_cast<std::streamsize>(text.size()));
    stream.flush();
    if (!stream)
    {
        message = "failed while writing '" + path.string() + "'";
        return false;
    }
    return true;
}

} // namespace

AtomicCaseDirectory::AtomicCaseDirectory(std::filesystem::path target)
    : target_(std::move(target))
{
}

CaseWriteStatus AtomicCaseDirectory::write(const CaseExportPlan& plan, bool replaceExisting)
{
    message_.clear();
    backup_.clear();

    if (target_.empty())
    {
        message_ = "the export target is empty";
        return CaseWriteStatus::TargetNotWritable;
    }

    const std::filesystem::path parent = target_.parent_path().empty() ? std::filesystem::path{"."}
                                                                     : target_.parent_path();
    static unsigned long sequence = 0;
    ++sequence;
    const std::string suffix = "." + std::to_string(sequence) + "-" + stamp();
    temp_ = parent / (target_.filename().string() + ".tmp" + suffix);

    std::error_code error;
    std::filesystem::create_directories(parent, error);
    if (error)
    {
        message_ = "cannot create '" + parent.string() + "': " + error.message();
        return CaseWriteStatus::TargetNotWritable;
    }

    std::filesystem::remove_all(temp_, error);
    std::filesystem::create_directories(temp_, error);
    if (error)
    {
        message_ = "cannot create '" + temp_.string() + "': " + error.message();
        return CaseWriteStatus::TargetNotWritable;
    }

    for (const ExportFile& file : plan.files)
    {
        if (file.isCopy())
        {
            // Byte exact copies are handled by the mirror below.
            continue;
        }
        const std::filesystem::path destination = temp_ / std::filesystem::path{file.relativePath};
        if (!writeTextFile(destination, file.text, message_))
        {
            std::filesystem::remove_all(temp_, error);
            return CaseWriteStatus::WriteFailed;
        }
    }

    const MirrorResult mirrored = CaseMirror{}.copyInto(temp_, plan);
    if (!mirrored.ok)
    {
        std::filesystem::remove_all(temp_, error);
        message_ = mirrored.message;
        return CaseWriteStatus::MirrorFailed;
    }

    // The manifest describes the finished directory, so it is written last but
    // still before the rename (spec 9.5).
    if (!plan.manifestText.empty()
        && !writeTextFile(temp_ / std::filesystem::path{exportmanifest::kFileName}, plan.manifestText, message_))
    {
        std::filesystem::remove_all(temp_, error);
        return CaseWriteStatus::ManifestFailed;
    }

    const bool targetExists = std::filesystem::exists(target_, error) && !error;
    if (targetExists)
    {
        if (!replaceExisting)
        {
            std::filesystem::remove_all(temp_, error);
            message_ = "the export target '" + target_.string() + "' already exists";
            return CaseWriteStatus::TargetNotWritable;
        }

        const bool empty = std::filesystem::is_directory(target_, error) && !error
            && std::filesystem::is_empty(target_, error) && !error;
        if (empty)
        {
            // Nothing to preserve: drop the empty directory instead of keeping
            // a useless backup.
            std::filesystem::remove_all(target_, error);
            if (error)
            {
                std::filesystem::remove_all(temp_, error);
                message_ = "cannot remove the empty target directory: " + error.message();
                return CaseWriteStatus::WriteFailed;
            }
        }
        else
        {
            backup_ = parent / (target_.filename().string() + ".bak" + suffix);
            std::filesystem::remove_all(backup_, error);
            std::filesystem::rename(target_, backup_, error);
            if (error)
            {
                std::filesystem::remove_all(temp_, error);
                message_ = "cannot move the existing target aside: " + error.message();
                return CaseWriteStatus::WriteFailed;
            }
        }
    }

    std::filesystem::rename(temp_, target_, error);
    if (error)
    {
        message_ = "cannot move '" + temp_.string() + "' to '" + target_.string() + "': " + error.message();
        std::filesystem::remove_all(temp_, error);

        // Roll back: the previous content must survive a failed export.
        if (!backup_.empty())
        {
            std::error_code restoreError;
            std::filesystem::rename(backup_, target_, restoreError);
            if (restoreError)
            {
                message_ += "; the previous content is kept at '" + backup_.string() + "'";
            }
            else
            {
                backup_.clear();
            }
        }
        return CaseWriteStatus::WriteFailed;
    }

    return CaseWriteStatus::Success;
}

} // namespace ofgui::io
