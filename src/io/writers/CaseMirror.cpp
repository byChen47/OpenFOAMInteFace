#include "io/writers/CaseMirror.h"

#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

namespace ofgui::io {
namespace {

constexpr std::size_t kBufferSize = 64 * 1024;

} // namespace

bool CaseMirror::copyFile(
    const std::filesystem::path& source,
    const std::filesystem::path& destination,
    std::string& message)
{
    std::error_code error;
    const std::filesystem::path parent = destination.parent_path();
    if (!parent.empty())
    {
        std::filesystem::create_directories(parent, error);
        if (error)
        {
            message = "cannot create '" + parent.string() + "': " + error.message();
            return false;
        }
    }

    std::ifstream input(source, std::ios::binary);
    if (!input)
    {
        message = "cannot open source file '" + source.string() + "' for reading";
        return false;
    }

    std::ofstream output(destination, std::ios::binary | std::ios::trunc);
    if (!output)
    {
        message = "cannot open '" + destination.string() + "' for writing";
        return false;
    }

    std::array<char, kBufferSize> buffer{};
    while (input)
    {
        input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const std::streamsize read = input.gcount();
        if (read <= 0)
        {
            break;
        }
        output.write(buffer.data(), read);
        if (!output)
        {
            message = "failed while writing '" + destination.string() + "'";
            return false;
        }
    }

    if (!input.eof())
    {
        message = "failed while reading '" + source.string() + "'";
        return false;
    }

    output.flush();
    if (!output)
    {
        message = "failed while writing '" + destination.string() + "'";
        return false;
    }
    return true;
}

MirrorResult CaseMirror::copyInto(
    const std::filesystem::path& stagingDirectory,
    const CaseExportPlan& plan) const
{
    MirrorResult result;

    if (stagingDirectory.empty())
    {
        result.message = "the staging directory is empty";
        return result;
    }

    for (const ExportFile& file : plan.files)
    {
        if (file.action == ExportAction::Written || file.sourcePath.empty())
        {
            continue;
        }

        const std::filesystem::path destination = stagingDirectory / std::filesystem::path{file.relativePath};
        if (!copyFile(file.sourcePath, destination, result.message))
        {
            result.message = file.relativePath + ": " + result.message;
            return result;
        }
        ++result.filesCopied;
        std::error_code error;
        const std::uintmax_t size = std::filesystem::file_size(destination, error);
        result.bytesCopied += error ? file.byteSize : size;
    }

    result.ok = true;
    return result;
}

} // namespace ofgui::io
