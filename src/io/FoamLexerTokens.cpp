#include "io/FoamLexer.h"

#include "io/FoamLexerChars.h"
#include "io/FoamToken.h"

#include <cstddef>
#include <string>
#include <string_view>

namespace ofgui::io {

std::optional<FoamToken> FoamLexer::readString()
{
    const std::size_t begin = pos_;
    ++pos_;

    std::string content;
    while (pos_ < source_.size())
    {
        const char c = source_[pos_];
        if (c == '\\' && pos_ + 1 < source_.size())
        {
            // Spec §5.2: only \" is unescaped, every other backslash stays literal.
            if (source_[pos_ + 1] == '"')
            {
                content += '"';
            }
            else
            {
                content += '\\';
                content += source_[pos_ + 1];
            }
            pos_ += 2;
            continue;
        }
        if (c == '"')
        {
            ++pos_;
            return makeToken(FoamTokenKind::String, begin, std::move(content));
        }
        if (c == '\n')
        {
            report(FoamSeverity::Error, "IMPORT-LEX-001", "unterminated string before end of line", begin);
            return makeToken(FoamTokenKind::String, begin, std::move(content));
        }
        content += c;
        ++pos_;
    }

    report(FoamSeverity::Error, "IMPORT-LEX-001", "unterminated string at end of file", begin);
    return makeToken(FoamTokenKind::String, begin, std::move(content));
}

std::optional<FoamToken> FoamLexer::readDirective()
{
    const std::size_t begin = pos_;
    ++pos_; // '#'
    while (pos_ < source_.size() && isFoamWordChar(source_[pos_]))
    {
        ++pos_;
    }
    return makeToken(
        FoamTokenKind::Directive, begin, std::string{source_.substr(begin, pos_ - begin)});
}

std::optional<FoamToken> FoamLexer::readVerbatim()
{
    const std::size_t begin = pos_; // '#'
    pos_ += 2;                      // skip "#{"

    const std::size_t contentBegin = pos_;
    const std::size_t end = source_.find("#}", pos_);
    if (end == std::string_view::npos)
    {
        report(FoamSeverity::Error, "IMPORT-LEX-003", "unterminated '#{ ... #}' block", begin);
        const std::string content{source_.substr(contentBegin)};
        pos_ = source_.size();
        return makeToken(FoamTokenKind::Verbatim, begin, content);
    }

    std::string content{source_.substr(contentBegin, end - contentBegin)};
    pos_ = end + 2;
    return makeToken(FoamTokenKind::Verbatim, begin, std::move(content));
}

std::optional<FoamToken> FoamLexer::readVariable()
{
    const std::size_t begin = pos_; // '$'
    ++pos_;

    if (pos_ >= source_.size())
    {
        report(FoamSeverity::Warning, "IMPORT-LEX-007", "lone '$' at end of file", begin);
        return makeToken(FoamTokenKind::Word, begin, "$");
    }

    const char c = source_[pos_];
    if (c == '{')
    {
        if (pos_ + 1 < source_.size() && source_[pos_ + 1] == '{')
        {
            // ${{ expression }} - tracked with brace depth, never evaluated.
            std::size_t depth = 0;
            while (pos_ < source_.size())
            {
                const char ch = source_[pos_];
                if (ch == '{')
                {
                    ++depth;
                }
                else if (ch == '}')
                {
                    --depth;
                    if (depth == 0)
                    {
                        ++pos_;
                        return makeToken(
                            FoamTokenKind::Expression,
                            begin,
                            std::string{source_.substr(begin, pos_ - begin)});
                    }
                }
                ++pos_;
            }
            report(FoamSeverity::Error, "IMPORT-LEX-007", "unterminated '${{ ... }}' expression", begin);
            pos_ = source_.size();
            return makeToken(
                FoamTokenKind::Expression, begin, std::string{source_.substr(begin)});
        }

        // ${...} tracks nested braces so that ${_${FOAM_EXECUTABLE}} stays one token.
        std::size_t depth = 0;
        while (pos_ < source_.size())
        {
            const char ch = source_[pos_];
            if (ch == '{')
            {
                ++depth;
            }
            else if (ch == '}')
            {
                --depth;
                if (depth == 0)
                {
                    ++pos_;
                    return makeToken(
                        FoamTokenKind::Variable,
                        begin,
                        std::string{source_.substr(begin, pos_ - begin)});
                }
            }
            ++pos_;
        }
        report(FoamSeverity::Error, "IMPORT-LEX-007", "unterminated '${ ... }' variable", begin);
        pos_ = source_.size();
        return makeToken(
            FoamTokenKind::Variable, begin, std::string{source_.substr(begin)});
    }

    if (foamIsVariableChar(c))
    {
        while (pos_ < source_.size() && foamIsVariableChar(source_[pos_]))
        {
            ++pos_;
        }
        return makeToken(
            FoamTokenKind::Variable, begin, std::string{source_.substr(begin, pos_ - begin)});
    }

    report(FoamSeverity::Warning, "IMPORT-LEX-007", "lone '$' without a variable name", begin);
    return makeToken(FoamTokenKind::Word, begin, "$");
}

std::optional<FoamToken> FoamLexer::readNumberOrWord()
{
    const std::size_t begin = pos_;
    while (pos_ < source_.size())
    {
        const char c = source_[pos_];
        if (foamIsAsciiDigit(c) || c == '+' || c == '-' || c == '.' || c == 'E' || c == 'e')
        {
            ++pos_;
            continue;
        }
        break;
    }

    const std::string_view text = source_.substr(begin, pos_ - begin);
    double value = 0.0;
    if (parseFoamReal(text, value))
    {
        return makeToken(FoamTokenKind::Number, begin, std::string{text});
    }

    if (text == "-")
    {
        pos_ = begin + 1;
        return makeToken(FoamTokenKind::Minus, begin, "-");
    }

    // Not a number after all (e.g. "1.2.3", "-wall"): fall back to a word.
    pos_ = begin;
    const auto word = readWord();
    if (word)
    {
        report(
            FoamSeverity::Warning,
            "IMPORT-LEX-006",
            "malformed number '" + std::string{text} + "', read as a word",
            begin);
    }
    return word;
}

std::optional<FoamToken> FoamLexer::readWord()
{
    const std::size_t begin = pos_;
    int depth = 0;
    while (pos_ < source_.size())
    {
        const char c = source_[pos_];
        if (!isFoamWordChar(c))
        {
            break;
        }
        if (c == '(')
        {
            ++depth;
        }
        else if (c == ')')
        {
            if (depth == 0)
            {
                break;
            }
            --depth;
        }
        ++pos_;
    }

    if (pos_ == begin)
    {
        const unsigned char byte = static_cast<unsigned char>(source_[pos_]);
        report(
            FoamSeverity::Error,
            "IMPORT-LEX-004",
            "unexpected character (byte 0x" + std::string(1, "0123456789abcdef"[byte >> 4])
                + std::string(1, "0123456789abcdef"[byte & 0x0F]) + ")",
            begin);
        ++pos_;
        return std::nullopt;
    }

    if (depth > 0)
    {
        report(
            FoamSeverity::Warning,
            "IMPORT-LEX-005",
            "missing " + std::to_string(depth) + " closing ')' in word",
            begin);
    }

    return makeToken(FoamTokenKind::Word, begin, std::string{source_.substr(begin, pos_ - begin)});
}


} // namespace ofgui::io
