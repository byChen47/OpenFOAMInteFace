#pragma once

#include "io/FoamNode.h"

#include <cstddef>
#include <string>

namespace ofgui::io {

/// Quotes and escapes a string value for OpenFOAM output.
inline std::string foamValueQuoted(const std::string& content)
{
    std::string out;
    out.reserve(content.size() + 2);
    out += '"';
    for (const char c : content)
    {
        if (c == '"')
        {
            out += '\\';
        }
        out += c;
    }
    out += '"';
    return out;
}

inline std::string foamValueLeafText(const FoamNode& node);

/// Renders the children of a list or dimension set between `open` and `close`.
inline std::string foamValueJoined(const FoamNode& node, char open, char close)
{
    std::string out;
    out += open;
    for (std::size_t i = 0; i < node.childCount(); ++i)
    {
        if (i != 0)
        {
            out += ' ';
        }
        out += foamValueLeafText(*node.childAt(i));
    }
    out += close;
    return out;
}

/// Single-line source-like rendering of one value node.
inline std::string foamValueLeafText(const FoamNode& node)
{
    switch (node.kind())
    {
    case FoamNodeKind::String:
        return foamValueQuoted(node.rawText());
    case FoamNodeKind::Verbatim:
        return "#{" + node.rawText() + "#}";
    case FoamNodeKind::List:
        return foamValueJoined(node, '(', ')');
    case FoamNodeKind::DimensionSet:
        return foamValueJoined(node, '[', ']');
    case FoamNodeKind::Sequence:
    {
        std::string out;
        for (std::size_t i = 0; i < node.childCount(); ++i)
        {
            if (i != 0)
            {
                out += ' ';
            }
            out += foamValueLeafText(*node.childAt(i));
        }
        return out;
    }
    case FoamNodeKind::Dictionary:
        return "{}";
    default:
        return node.rawText();
    }
}

} // namespace ofgui::io
