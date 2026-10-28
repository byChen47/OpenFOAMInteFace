#include "io/FoamLexer.h"

#include "io/FoamLexerChars.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

namespace ofgui::io {
namespace {

constexpr std::size_t kMaxSnippetLength = 160;

} // namespace

FoamLexer::FoamLexer(std::string_view source, std::string fileLabel)
    : source_(source)
    , fileLabel_(std::move(fileLabel))
{
    buildLineIndex();
}

const std::vector<FoamDiagnostic>& FoamLexer::diagnostics() const noexcept
{
    return diagnostics_;
}

std::string_view FoamLexer::source() const noexcept
{
    return source_;
}

const std::string& FoamLexer::fileLabel() const noexcept
{
    return fileLabel_;
}

std::size_t FoamLexer::byteOffset() const noexcept
{
    return pos_;
}

void FoamLexer::buildLineIndex()
{
    lineStarts_.clear();
    lineStarts_.push_back(0);
    for (std::size_t i = 0; i < source_.size(); ++i)
    {
        if (source_[i] == '\n')
        {
            lineStarts_.push_back(i + 1);
        }
    }
}

std::size_t FoamLexer::lineOf(std::size_t byteOffset) const
{
    const auto it = std::upper_bound(lineStarts_.begin(), lineStarts_.end(), byteOffset);
    return static_cast<std::size_t>(it - lineStarts_.begin());
}

std::size_t FoamLexer::columnOf(std::size_t byteOffset) const
{
    const std::size_t line = lineOf(byteOffset);
    return byteOffset - lineStarts_[line - 1] + 1;
}

std::optional<FoamToken> FoamLexer::next()
{
    for (;;)
    {
        skipTrivia();
        if (pos_ >= source_.size())
        {
            return std::nullopt;
        }

        const char c = source_[pos_];
        const std::size_t begin = pos_;

        switch (c)
        {
        case ';':
            ++pos_;
            return makeToken(FoamTokenKind::Semicolon, begin, ";");
        case '{':
            ++pos_;
            return makeToken(FoamTokenKind::OpenBrace, begin, "{");
        case '}':
            ++pos_;
            return makeToken(FoamTokenKind::CloseBrace, begin, "}");
        case '(':
            ++pos_;
            return makeToken(FoamTokenKind::OpenParen, begin, "(");
        case ')':
            ++pos_;
            return makeToken(FoamTokenKind::CloseParen, begin, ")");
        case '[':
            ++pos_;
            return makeToken(FoamTokenKind::OpenBracket, begin, "[");
        case ']':
            ++pos_;
            return makeToken(FoamTokenKind::CloseBracket, begin, "]");
        case ':':
            ++pos_;
            return makeToken(FoamTokenKind::Colon, begin, ":");
        case ',':
            ++pos_;
            return makeToken(FoamTokenKind::Comma, begin, ",");
        case '=':
            ++pos_;
            return makeToken(FoamTokenKind::Assign, begin, "=");
        case '+':
            ++pos_;
            return makeToken(FoamTokenKind::Plus, begin, "+");
        case '*':
            ++pos_;
            return makeToken(FoamTokenKind::Multiply, begin, "*");
        case '/':
            // Comments were consumed by skipTrivia(); a bare '/' is a token.
            ++pos_;
            return makeToken(FoamTokenKind::Divide, begin, "/");
        case '"':
            return readString();
        case '#':
            if (pos_ + 1 < source_.size() && source_[pos_ + 1] == '{')
            {
                return readVerbatim();
            }
            if (pos_ + 1 < source_.size() && isFoamWordChar(source_[pos_ + 1]))
            {
                return readDirective();
            }
            ++pos_;
            return makeToken(FoamTokenKind::Word, begin, "#");
        case '$':
            return readVariable();
        default:
            break;
        }

        if (c == '-' || c == '.' || foamIsAsciiDigit(c))
        {
            return readNumberOrWord();
        }

        if (const auto word = readWord())
        {
            return word;
        }
        // readWord() consumed and reported one invalid byte; keep scanning.
    }
}

void FoamLexer::skipTrivia()
{
    for (;;)
    {
        if (pos_ >= source_.size())
        {
            return;
        }

        const char c = source_[pos_];
        if (foamIsAsciiSpace(c))
        {
            ++pos_;
            continue;
        }

        if (c == '/' && pos_ + 1 < source_.size())
        {
            if (source_[pos_ + 1] == '/')
            {
                pos_ += 2;
                while (pos_ < source_.size() && source_[pos_] != '\n')
                {
                    ++pos_;
                }
                continue;
            }
            if (source_[pos_ + 1] == '*')
            {
                const std::size_t commentBegin = pos_;
                const std::size_t end = source_.find("*/", pos_ + 2);
                if (end == std::string_view::npos)
                {
                    report(FoamSeverity::Error, "IMPORT-LEX-002", "unterminated '/*' comment", commentBegin);
                    pos_ = source_.size();
                    return;
                }
                pos_ = end + 2;
                continue;
            }
        }

        return;
    }
}

FoamToken FoamLexer::makeToken(FoamTokenKind kind, std::size_t begin, std::string text) const
{
    FoamToken token;
    token.kind = kind;
    token.text = std::move(text);
    token.byteOffset = begin;
    token.line = static_cast<int>(lineOf(begin));
    token.column = static_cast<int>(columnOf(begin));
    return token;
}

void FoamLexer::report(
    FoamSeverity severity, std::string code, std::string message, std::size_t byteOffset)
{
    FoamDiagnostic diagnostic;
    diagnostic.severity = severity;
    diagnostic.code = std::move(code);
    diagnostic.message = std::move(message);
    diagnostic.location = FoamSourceLocation{
        fileLabel_, byteOffset, static_cast<int>(lineOf(byteOffset)), static_cast<int>(columnOf(byteOffset))};
    diagnostic.snippet = makeSnippet(byteOffset);
    diagnostics_.push_back(std::move(diagnostic));
}

std::string FoamLexer::makeSnippet(std::size_t byteOffset) const
{
    if (source_.empty())
    {
        return {};
    }

    const std::size_t line = lineOf(byteOffset);
    const std::size_t begin = lineStarts_[line - 1];
    const std::size_t end = (line < lineStarts_.size()) ? lineStarts_[line] : source_.size();
    std::string_view text = source_.substr(begin, end - begin);

    while (!text.empty() && (text.back() == '\n' || text.back() == '\r'))
    {
        text.remove_suffix(1);
    }
    while (!text.empty() && (text.front() == ' ' || text.front() == '\t'))
    {
        text.remove_prefix(1);
    }
    if (text.size() > kMaxSnippetLength)
    {
        text = text.substr(0, kMaxSnippetLength);
    }
    return std::string{text};
}

} // namespace ofgui::io
