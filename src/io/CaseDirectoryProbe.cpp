#include "io/CaseDirectoryProbe.h"

#include <array>
#include <system_error>

namespace ofgui::io::detail {
namespace {

namespace fs = std::filesystem;

constexpr std::array<const char*, 5> kPolyMeshFiles{"points", "faces", "owner", "neighbour", "boundary"};

} // namespace

bool isDirectory(const fs::path& path) noexcept
{
    std::error_code ec;
    return fs::is_directory(path, ec) && !ec;
}

bool isRegularFile(const fs::path& path) noexcept
{
    std::error_code ec;
    return fs::is_regular_file(path, ec) && !ec;
}

std::string relativePath(const fs::path& root, const fs::path& entry)
{
    std::error_code ec;
    const fs::path relative = fs::relative(entry, root, ec);
    if (ec || relative.empty())
    {
        return entry.filename().generic_string();
    }
    return relative.generic_string();
}

std::uintmax_t fileSize(const fs::path& path) noexcept
{
    std::error_code ec;
    const std::uintmax_t size = fs::file_size(path, ec);
    return ec ? 0U : size;
}

bool polyMeshComplete(const fs::path& meshDirectory)
{
    for (const char* name : kPolyMeshFiles)
    {
        const fs::path plain = meshDirectory / name;
        const fs::path gzip = meshDirectory / (std::string(name) + ".gz");
        if (!isRegularFile(plain) && !isRegularFile(gzip))
        {
            return false;
        }
    }
    return true;
}

} // namespace ofgui::io::detail
