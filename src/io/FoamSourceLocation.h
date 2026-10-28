#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace ofgui::io {

/// Position of a token or AST node inside a dictionary file.
///
/// `file` is a display label (case-relative path or "<string>"); it is never
/// opened by the parser. `byteOffset` is the 0-based offset of the first byte
/// of the token, `line`/`column` are 1-based and column counts bytes, so a
/// non-ASCII byte counts as one column (the parser works on raw bytes).
struct FoamSourceLocation
{
    std::string file;
    std::size_t byteOffset{0};
    int line{0};
    int column{0};

    /// "file:line:column" - the compact form used by diagnostics and UI hints.
    std::string toString() const
    {
        std::string out = file.empty() ? std::string{"<string>"} : file;
        out += ':';
        out += std::to_string(line);
        out += ':';
        out += std::to_string(column);
        return out;
    }

    bool operator==(const FoamSourceLocation& other) const
    {
        return byteOffset == other.byteOffset && line == other.line
               && column == other.column && file == other.file;
    }
};

} // namespace ofgui::io
