#include "io/FoamToken.h"

#include <charconv>
#include <cstddef>
#include <system_error>

namespace ofgui::io {
namespace {

/// `std::from_chars` does not accept a leading '+' (the standard forbids it),
/// but OpenFOAM dictionaries occasionally carry one (e.g. `+1e-05`).
std::string_view stripLeadingPlus(std::string_view text)
{
    if (!text.empty() && text.front() == '+')
    {
        return text.substr(1);
    }
    return text;
}

bool isAsciiSpace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

} // namespace

FoamSourceLocation FoamToken::location(std::string_view fileLabel) const
{
    return FoamSourceLocation{std::string{fileLabel}, byteOffset, line, column};
}

bool FoamToken::isKeywordCandidate() const
{
    return kind == FoamTokenKind::Word || kind == FoamTokenKind::String
           || kind == FoamTokenKind::Number;
}

bool FoamToken::isLeafValue() const
{
    switch (kind)
    {
    case FoamTokenKind::Word:
    case FoamTokenKind::String:
    case FoamTokenKind::Number:
    case FoamTokenKind::Verbatim:
    case FoamTokenKind::Variable:
    case FoamTokenKind::Expression:
    case FoamTokenKind::Directive:
        return true;
    default:
        return false;
    }
}

char FoamToken::punctuation() const
{
    return foamTokenPunctuationChar(kind);
}

bool foamTokenIsPunctuation(FoamTokenKind kind)
{
    return foamTokenPunctuationChar(kind) != '\0';
}

char foamTokenPunctuationChar(FoamTokenKind kind)
{
    switch (kind)
    {
    case FoamTokenKind::Semicolon:
        return ';';
    case FoamTokenKind::OpenBrace:
        return '{';
    case FoamTokenKind::CloseBrace:
        return '}';
    case FoamTokenKind::OpenParen:
        return '(';
    case FoamTokenKind::CloseParen:
        return ')';
    case FoamTokenKind::OpenBracket:
        return '[';
    case FoamTokenKind::CloseBracket:
        return ']';
    case FoamTokenKind::Colon:
        return ':';
    case FoamTokenKind::Comma:
        return ',';
    case FoamTokenKind::Assign:
        return '=';
    case FoamTokenKind::Plus:
        return '+';
    case FoamTokenKind::Minus:
        return '-';
    case FoamTokenKind::Multiply:
        return '*';
    case FoamTokenKind::Divide:
        return '/';
    default:
        return '\0';
    }
}

std::string_view foamTokenKindName(FoamTokenKind kind)
{
    switch (kind)
    {
    case FoamTokenKind::Word:
        return "word";
    case FoamTokenKind::String:
        return "string";
    case FoamTokenKind::Number:
        return "number";
    case FoamTokenKind::Verbatim:
        return "verbatim";
    case FoamTokenKind::Variable:
        return "variable";
    case FoamTokenKind::Expression:
        return "expression";
    case FoamTokenKind::Directive:
        return "directive";
    case FoamTokenKind::Semicolon:
        return "';'";
    case FoamTokenKind::OpenBrace:
        return "'{'";
    case FoamTokenKind::CloseBrace:
        return "'}'";
    case FoamTokenKind::OpenParen:
        return "'('";
    case FoamTokenKind::CloseParen:
        return "')'";
    case FoamTokenKind::OpenBracket:
        return "'['";
    case FoamTokenKind::CloseBracket:
        return "']'";
    case FoamTokenKind::Colon:
        return "':'";
    case FoamTokenKind::Comma:
        return "','";
    case FoamTokenKind::Assign:
        return "'='";
    case FoamTokenKind::Plus:
        return "'+'";
    case FoamTokenKind::Minus:
        return "'-'";
    case FoamTokenKind::Multiply:
        return "'*'";
    case FoamTokenKind::Divide:
        return "'/'";
    }
    return "unknown";
}

bool isFoamWordChar(char c)
{
    if (isAsciiSpace(c))
    {
        return false;
    }
    switch (c)
    {
    case '"':
    case '\'':
    case '/':
    case ';':
    case '{':
    case '}':
        return false;
    default:
        break;
    }
    // Control characters never belong to a word; the lexer reports them.
    const auto byte = static_cast<unsigned char>(c);
    return byte >= 0x20 && byte != 0x7FU;
}

bool parseFoamReal(std::string_view text, double& value)
{
    const std::string_view body = stripLeadingPlus(text);
    if (body.empty())
    {
        return false;
    }

    double parsed = 0.0;
    const char* first = body.data();
    const char* last = body.data() + body.size();
    const std::from_chars_result result = std::from_chars(first, last, parsed);
    if (result.ec != std::errc{} || result.ptr != last)
    {
        return false;
    }
    value = parsed;
    return true;
}

bool parseFoamInt(std::string_view text, long long& value)
{
    const std::string_view body = stripLeadingPlus(text);
    if (body.empty())
    {
        return false;
    }

    long long parsed = 0;
    const char* first = body.data();
    const char* last = body.data() + body.size();
    const std::from_chars_result result = std::from_chars(first, last, parsed);
    if (result.ec != std::errc{} || result.ptr != last)
    {
        return false;
    }
    value = parsed;
    return true;
}

} // namespace ofgui::io
