#pragma once

namespace ofgui::io {

/// ASCII whitespace, matching the characters ISstream::nextValid() skips.
inline bool foamIsAsciiSpace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

inline bool foamIsAsciiDigit(char c)
{
    return c >= '0' && c <= '9';
}

/// Characters that may follow `$` in a `$name` variable.
///
/// Delimiters end the variable, matching the OpenFOAM v2206 subset documented in
/// docs/CASE_IMPORT_SPEC.md §5.4: `;`, whitespace, `)`, `}`, `(`, `[`, `]`.
inline bool foamIsVariableChar(char c)
{
    if (foamIsAsciiSpace(c))
    {
        return false;
    }
    switch (c)
    {
    case ';':
    case '{':
    case '}':
    case '(':
    case ')':
    case '[':
    case ']':
    case ',':
    case '"':
    case '\'':
    case '#':
        return false;
    default:
        return true;
    }
}

} // namespace ofgui::io
