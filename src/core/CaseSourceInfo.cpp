#include "core/CaseSourceInfo.h"

#include <algorithm>

namespace ofgui::core {
namespace {

constexpr char kHexDigits[] = "0123456789abcdef";

[[nodiscard]] int hexValue(char c) noexcept
{
    if (c >= '0' && c <= '9')
    {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f')
    {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F')
    {
        return c - 'A' + 10;
    }
    return -1;
}

} // namespace

std::string sha256ToHex(const Sha256Digest& digest)
{
    std::string hex(kSha256HexLength, '0');
    for (std::size_t i = 0; i < kSha256ByteCount; ++i)
    {
        const std::uint8_t byte = digest[i];
        hex[2 * i] = kHexDigits[byte >> 4];
        hex[2 * i + 1] = kHexDigits[byte & 0x0F];
    }
    return hex;
}

bool sha256FromHex(std::string_view hex, Sha256Digest& digest)
{
    if (hex.size() != kSha256HexLength)
    {
        return false;
    }

    Sha256Digest parsed{};
    for (std::size_t i = 0; i < kSha256ByteCount; ++i)
    {
        const int high = hexValue(hex[2 * i]);
        const int low = hexValue(hex[2 * i + 1]);
        if (high < 0 || low < 0)
        {
            return false;
        }
        parsed[i] = static_cast<std::uint8_t>((high << 4) | low);
    }

    digest = parsed;
    return true;
}

std::string SourceFileHash::sha256Hex() const
{
    return sha256ToHex(sha256);
}

bool SourceFileHash::setSha256Hex(std::string_view hex)
{
    return sha256FromHex(hex, sha256);
}

bool CaseSourceInfo::isEmpty() const noexcept
{
    return sourceRelativePath.empty() && startDirectory.empty() && importedAt.empty()
        && files.empty();
}

std::size_t CaseSourceInfo::fileCount() const noexcept
{
    return files.size();
}

const SourceFileHash* CaseSourceInfo::findFile(std::string_view relativePath) const
{
    const auto it = std::find_if(files.begin(), files.end(),
        [relativePath](const SourceFileHash& file) { return file.relativePath == relativePath; });
    return it == files.end() ? nullptr : &*it;
}

bool CaseSourceInfo::setFileHash(std::string_view relativePath, std::string_view hex)
{
    Sha256Digest digest{};
    if (!sha256FromHex(hex, digest))
    {
        return false;
    }

    const auto it = std::find_if(files.begin(), files.end(),
        [relativePath](const SourceFileHash& file) { return file.relativePath == relativePath; });
    if (it != files.end())
    {
        it->sha256 = digest;
        return true;
    }

    SourceFileHash file;
    file.relativePath = std::string(relativePath);
    file.sha256 = digest;
    files.push_back(std::move(file));
    return true;
}

bool CaseSourceInfo::removeFile(std::string_view relativePath)
{
    const auto it = std::find_if(files.begin(), files.end(),
        [relativePath](const SourceFileHash& file) { return file.relativePath == relativePath; });
    if (it == files.end())
    {
        return false;
    }
    files.erase(it);
    return true;
}

} // namespace ofgui::core
