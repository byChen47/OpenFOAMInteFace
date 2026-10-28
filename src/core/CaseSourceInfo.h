#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace ofgui::core {

/// Number of bytes in a SHA-256 digest and in its lower-case hex form.
inline constexpr std::size_t kSha256ByteCount = 32;
inline constexpr std::size_t kSha256HexLength = 64;

/// Raw SHA-256 digest of one imported source file.
///
/// Persisted as lower-case hex so `.ofgproj` stays human diffable and stable.
using Sha256Digest = std::array<std::uint8_t, kSha256ByteCount>;

/// Lower-case hex representation (exactly 64 characters).
[[nodiscard]] std::string sha256ToHex(const Sha256Digest& digest);

/// Parses exactly 64 hex digits (upper or lower case) into `digest`.
///
/// On failure the digest is left untouched and `false` is returned, so callers
/// can surface a located import error instead of silently writing zeros.
[[nodiscard]] bool sha256FromHex(std::string_view hex, Sha256Digest& digest);

/// One file read while importing a case, keyed by its case-relative path.
///
/// The path is always relative to `CaseSourceInfo::sourceRelativePath` and uses
/// '/' separators, so the same project stays valid after the case is moved.
/// Absolute paths are never stored (docs/DECISIONS.md D-025).
struct SourceFileHash
{
    std::string relativePath;
    std::array<std::uint8_t, kSha256ByteCount> sha256{};

    [[nodiscard]] std::string sha256Hex() const;
    bool setSha256Hex(std::string_view hex);
};

/// Where a project came from: the imported case directory plus per-file hashes.
///
/// All members are plain standard-library types. An all-empty value means "this
/// project was not created from an existing case" (a brand new project), which
/// is the default of `CaseProject::source()`.
struct CaseSourceInfo
{
    /// Case directory relative to the project file (or a user-relocatable root).
    std::string sourceRelativePath;

    /// Name of the directory that holds the initial fields: "0" or "0.orig".
    std::string startDirectory;

    /// Import timestamp as ISO-8601 UTC text (for display and diagnostics only).
    std::string importedAt;

    /// Files that were read while importing, in scan order.
    std::vector<SourceFileHash> files;

    /// True when no metadata and no file hash has been recorded.
    [[nodiscard]] bool isEmpty() const noexcept;

    /// Number of recorded file hashes.
    [[nodiscard]] std::size_t fileCount() const noexcept;

    /// @return the hash for `relativePath`, or nullptr when it was not recorded.
    [[nodiscard]] const SourceFileHash* findFile(std::string_view relativePath) const;

    /// Inserts or updates the hash of `relativePath`.
    /// @return false when `hex` is not a valid 64-digit SHA-256 string.
    bool setFileHash(std::string_view relativePath, std::string_view hex);

    /// @return false when no file with that path was recorded.
    bool removeFile(std::string_view relativePath);
};

} // namespace ofgui::core
