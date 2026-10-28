#include "io/writers/SourceCaseWalker.h"

#include "io/CaseDirectoryProbe.h"
#include "io/writers/ExportSkipRules.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include <system_error>
#include <utility>

namespace ofgui::io {
namespace {

/// Directories that never contribute exported files: runtime products only.
bool shouldPruneDirectory(const std::filesystem::path& directory)
{
    const std::string name = directory.filename().string();
    return name == "postProcessing" || ExportSkipRules::isRuntimeArtifact(name);
}

} // namespace

SourceCaseWalker::SourceCaseWalker(std::filesystem::path root)
    : root_(std::move(root))
{
}

SourceCaseWalk SourceCaseWalker::walk() const
{
    SourceCaseWalk result;
    if (root_.empty() || !detail::isDirectory(root_))
    {
        return result;
    }

    if (detail::isDirectory(root_ / "0"))
    {
        result.startDirectory = "0";
    }
    else if (detail::isDirectory(root_ / "0.orig"))
    {
        result.startDirectory = "0.orig";
    }

    std::error_code error;
    std::filesystem::recursive_directory_iterator iterator(
        root_,
        std::filesystem::directory_options::skip_permission_denied,
        error);
    if (error)
    {
        return result;
    }

    const std::filesystem::recursive_directory_iterator end;
    for (; iterator != end; iterator.increment(error))
    {
        if (error)
        {
            break;
        }

        const std::filesystem::directory_entry& entry = *iterator;
        std::error_code statusError;
        if (entry.is_symlink(statusError))
        {
            result.symlinks.push_back(detail::relativePath(root_, entry.path()));
            iterator.disable_recursion_pending();
            continue;
        }
        if (entry.is_directory(statusError))
        {
            if (shouldPruneDirectory(entry.path()))
            {
                iterator.disable_recursion_pending();
            }
            continue;
        }
        if (!entry.is_regular_file(statusError))
        {
            continue;
        }

        if (result.files.size() >= kMaxFiles)
        {
            result.truncated = true;
            break;
        }

        SourceCaseFile file;
        file.relativePath = detail::relativePath(root_, entry.path());
        file.absolutePath = entry.path();
        std::error_code sizeError;
        const std::uintmax_t size = entry.file_size(sizeError);
        file.byteSize = sizeError ? 0 : size;
        const std::string prefix = result.startDirectory + "/";
        file.inStartDirectory = !result.startDirectory.empty()
            && file.relativePath.size() > prefix.size()
            && file.relativePath.compare(0, prefix.size(), prefix) == 0;
        result.files.push_back(std::move(file));
    }

    std::sort(result.files.begin(), result.files.end(), [](const SourceCaseFile& left, const SourceCaseFile& right)
              { return left.relativePath < right.relativePath; });
    std::sort(result.symlinks.begin(), result.symlinks.end());
    return result;
}

} // namespace ofgui::io
