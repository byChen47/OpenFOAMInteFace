#pragma once

#include <QString>

#include <filesystem>

namespace ofgui::detail {

/// Converts a user-facing path to `std::filesystem::path` without going through
/// the local 8-bit code page, so non-ASCII case directories round-trip.
inline std::filesystem::path toPath(const QString& text)
{
    return std::filesystem::path(text.toStdU16String());
}

} // namespace ofgui::detail
