#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace ofgui::io {

/// One regular file found below a source case root.
struct SourceCaseFile
{
    /// Case relative POSIX path (`constant/polyMesh/points.gz`).
    std::string relativePath;
    std::filesystem::path absolutePath;
    std::uintmax_t byteSize{0};
    /// True when the file lives in the start directory (`0/` or `0.orig/`).
    bool inStartDirectory{false};
};

/// Everything `SourceCaseWalker::walk` found.
struct SourceCaseWalk
{
    /// `0`, `0.orig` or empty when the source has neither.
    std::string startDirectory;
    /// Sorted by `relativePath`.
    std::vector<SourceCaseFile> files;
    /// Symbolic links below the case root: listed, never followed (spec 4.1).
    std::vector<std::string> symlinks;
    /// The listing stopped at `SourceCaseWalker::kMaxFiles`.
    bool truncated{false};
};

/// Lists every regular file of a source case without interpreting any of them.
///
/// The walker only reads directory entries: no dictionary is parsed, nothing is
/// modified and symbolic links are never followed.
class SourceCaseWalker
{
public:
    /// Upper bound on listed files; protects against pathological cases.
    static constexpr std::size_t kMaxFiles = 20000;

    explicit SourceCaseWalker(std::filesystem::path root);

    [[nodiscard]] SourceCaseWalk walk() const;

    [[nodiscard]] const std::filesystem::path& root() const noexcept { return root_; }

private:
    std::filesystem::path root_;
};

} // namespace ofgui::io
