#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace ofgui::io::detail {

/// True when `path` is a directory; symbolic links that cannot be resolved
/// report false because the scan never follows them.
bool isDirectory(const std::filesystem::path& path) noexcept;

bool isRegularFile(const std::filesystem::path& path) noexcept;

/// Case relative POSIX path of `entry` below `root`; the file name when the
/// relative computation fails.
std::string relativePath(const std::filesystem::path& root, const std::filesystem::path& entry);

/// File size in bytes, `0` when it cannot be read.
std::uintmax_t fileSize(const std::filesystem::path& path) noexcept;

/// Probes `constant/polyMesh` without reading it: are the five mandatory files
/// present, optionally gzip compressed?
bool polyMeshComplete(const std::filesystem::path& meshDirectory);

} // namespace ofgui::io::detail
