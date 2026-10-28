#include "io/FoamParser.h"

#include "io/FoamDiagnostic.h"
#include "io/FoamNode.h"
#include "io/FoamSourceLocation.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ofgui::io {
namespace {

constexpr std::size_t kMaxSnippetLength = 160;

} // namespace

FoamParser::FoamParser(std::string_view source, std::string fileLabel, FoamParseOptions options)
    : lexer_(source, std::move(fileLabel))
    , options_(options)
{
}

FoamDictionary FoamParser::parse()
{
    auto root = std::make_unique<FoamNode>(
        FoamNodeKind::Dictionary, FoamSourceLocation{lexer_.fileLabel(), 0, 1, 1});

    while (const FoamToken* token = peek(0))
    {
        if (token->kind == FoamTokenKind::CloseBrace)
        {
            report(FoamSeverity::Error, "IMPORT-PARSE-006", "unexpected '}' at top level", *token);
            take();
            continue;
        }
        parseEntryInto(*root);
    }

    const FoamNode* first = root->childCount() > 0 ? root->childAt(0) : nullptr;
    const bool hasHeader = first != nullptr && first->isDictionary() && first->keyword() == "FoamFile";
    if (!hasHeader && options_.requireFoamFileHeader)
    {
        reportAt(
            FoamSeverity::Error,
            "IMPORT-PARSE-003",
            "missing 'FoamFile' header: the first entry of a dictionary must be 'FoamFile { ... }'",
            0);
    }

    std::vector<FoamDiagnostic> all = lexer_.diagnostics();
    all.insert(
        all.end(),
        std::make_move_iterator(diagnostics_.begin()),
        std::make_move_iterator(diagnostics_.end()));
    std::stable_sort(
        all.begin(),
        all.end(),
        [](const FoamDiagnostic& lhs, const FoamDiagnostic& rhs) {
            return lhs.location.byteOffset < rhs.location.byteOffset;
        });

    if (options_.maxDiagnostics > 0 && all.size() > options_.maxDiagnostics)
    {
        const std::size_t truncated = all.size() - options_.maxDiagnostics;
        all.resize(options_.maxDiagnostics);
        FoamDiagnostic note;
        note.severity = FoamSeverity::Info;
        note.code = "IMPORT-REPORT-001";
        note.message = std::to_string(truncated) + " further diagnostic(s) were truncated";
        note.location = FoamSourceLocation{lexer_.fileLabel(), 0, 1, 1};
        all.push_back(std::move(note));
    }

    std::string source{lexer_.source()};
    std::string label = lexer_.fileLabel();
    return FoamDictionary{std::move(source), std::move(label), std::move(root), std::move(all)};
}

const FoamToken* FoamParser::peek(std::size_t ahead)
{
    while (buffer_.size() <= ahead && !exhausted_)
    {
        std::optional<FoamToken> token = lexer_.next();
        if (!token)
        {
            exhausted_ = true;
            break;
        }
        buffer_.push_back(std::move(*token));
    }
    if (ahead >= buffer_.size())
    {
        return nullptr;
    }
    return &buffer_[ahead];
}

FoamToken FoamParser::take()
{
    FoamToken token = std::move(buffer_.front());
    buffer_.pop_front();
    return token;
}

void FoamParser::parseEntryInto(FoamNode& dictionary)
{
    const FoamToken* first = peek(0);
    if (!first)
    {
        return;
    }
    if (first->kind == FoamTokenKind::Directive)
    {
        parseDirectiveInto(dictionary);
        return;
    }
    if (first->kind == FoamTokenKind::Semicolon)
    {
        report(FoamSeverity::Error, "IMPORT-PARSE-008", "stray ';' (no entry to terminate)", *first);
        take();
        return;
    }
    if (!first->isKeywordCandidate() && first->kind != FoamTokenKind::Variable
        && first->kind != FoamTokenKind::Expression)
    {
        report(
            FoamSeverity::Error,
            "IMPORT-PARSE-007",
            "expected a keyword, found '" + std::string{foamTokenKindName(first->kind)} + "'",
            *first);
        take();
        return;
    }

    const FoamToken keyword = take();
    if (keyword.kind == FoamTokenKind::Variable || keyword.kind == FoamTokenKind::Expression)
    {
        // `$p;` merges another dictionary; such an entry has no keyword of its own.
        std::unique_ptr<FoamNode> merge = makeLeaf(keyword);
        const FoamToken* trailing = peek(0);
        if (trailing && trailing->kind == FoamTokenKind::Semicolon)
        {
            take();
        }
        dictionary.addChild(std::move(merge));
        return;
    }
    const FoamToken* next = peek(0);
    if (next && next->kind == FoamTokenKind::OpenBrace)
    {
        const FoamToken open = take();
        std::unique_ptr<FoamNode> child = parseDictionaryBody(open);
        child->setKeyword(keyword.text, keyword.kind == FoamTokenKind::String);
        attachEntry(dictionary, std::move(child), keyword);

        if (const FoamToken* trailing = peek(0))
        {
            if (trailing->kind == FoamTokenKind::Semicolon)
            {
                take();
            }
        }
        return;
    }

    std::unique_ptr<FoamNode> value = parseValueSequence(keyword);
    value->setKeyword(keyword.text, keyword.kind == FoamTokenKind::String);
    attachEntry(dictionary, std::move(value), keyword);
}

void FoamParser::enterNesting(const FoamToken& open)
{
    ++depth_;
    if (depth_ != options_.maxNestingDepth + 1)
    {
        return;
    }
    report(
        FoamSeverity::Error,
        "IMPORT-PARSE-009",
        "nesting is deeper than " + std::to_string(options_.maxNestingDepth)
            + " levels; the rest of this block is skipped",
        open);
}

void FoamParser::leaveNesting() noexcept
{
    if (depth_ > 0)
    {
        --depth_;
    }
}

bool FoamParser::nestingExceeded() const noexcept
{
    return depth_ > options_.maxNestingDepth;
}

std::unique_ptr<FoamNode> FoamParser::parseDictionaryBody(const FoamToken& open)
{
    auto dictionary = std::make_unique<FoamNode>(FoamNodeKind::Dictionary, locationOf(open));

    struct DepthGuard
    {
        FoamParser& parser;
        ~DepthGuard() { parser.leaveNesting(); }
    } guard{*this};
    enterNesting(open);
    if (nestingExceeded())
    {
        skipNestedBody();
        return dictionary;
    }
    for (;;)
    {
        const FoamToken* token = peek(0);
        if (!token)
        {
            report(
                FoamSeverity::Error,
                "IMPORT-PARSE-002",
                "missing '}' for the dictionary opened on line " + std::to_string(open.line),
                open);
            break;
        }
        if (token->kind == FoamTokenKind::CloseBrace)
        {
            take();
            break;
        }
        if (token->kind == FoamTokenKind::CloseParen)
        {
            report(FoamSeverity::Error, "IMPORT-PARSE-006", "unexpected ')' inside a dictionary", *token);
            take();
            continue;
        }
        if (token->kind == FoamTokenKind::CloseBracket)
        {
            report(FoamSeverity::Error, "IMPORT-PARSE-006", "unexpected ']' inside a dictionary", *token);
            take();
            continue;
        }
        parseEntryInto(*dictionary);
    }
    return dictionary;
}

void FoamParser::skipNestedBody()
{
    // The opening token was already consumed by the caller; `depth` counts the
    // containers still open inside it.
    std::size_t depth = 0;
    while (const FoamToken* token = peek(0))
    {
        const FoamTokenKind kind = token->kind;
        take();
        switch (kind)
        {
        case FoamTokenKind::OpenBrace:
        case FoamTokenKind::OpenParen:
            ++depth;
            break;
        case FoamTokenKind::CloseBrace:
        case FoamTokenKind::CloseParen:
            if (depth == 0)
            {
                return;
            }
            --depth;
            break;
        default:
            break;
        }
    }
}

void FoamParser::attachEntry(
    FoamNode& dictionary, std::unique_ptr<FoamNode> entry, const FoamToken& keyword)
{
    if (!keyword.text.empty())
    {
        if (const FoamNode* existing = dictionary.findChild(keyword.text))
        {
            report(
                FoamSeverity::Warning,
                "IMPORT-SYN-003",
                "duplicate keyword '" + keyword.text + "' (first occurrence on line "
                    + std::to_string(existing->location().line) + ")",
                keyword);
        }
    }
    dictionary.addChild(std::move(entry));
}

FoamSourceLocation FoamParser::locationOf(const FoamToken& token) const
{
    return FoamSourceLocation{lexer_.fileLabel(), token.byteOffset, token.line, token.column};
}

void FoamParser::report(
    FoamSeverity severity, std::string code, std::string message, const FoamToken& token)
{
    FoamDiagnostic diagnostic;
    diagnostic.severity = severity;
    diagnostic.code = std::move(code);
    diagnostic.message = std::move(message);
    diagnostic.location = locationOf(token);
    diagnostic.snippet = makeSnippet(token.byteOffset);
    diagnostics_.push_back(std::move(diagnostic));
}

void FoamParser::reportAt(
    FoamSeverity severity, std::string code, std::string message, std::size_t byteOffset)
{
    FoamDiagnostic diagnostic;
    diagnostic.severity = severity;
    diagnostic.code = std::move(code);
    diagnostic.message = std::move(message);
    diagnostic.location = FoamSourceLocation{lexer_.fileLabel(), byteOffset, 1, 1};
    diagnostic.snippet = makeSnippet(byteOffset);
    diagnostics_.push_back(std::move(diagnostic));
}

std::string FoamParser::makeSnippet(std::size_t byteOffset) const
{
    const std::string_view source = lexer_.source();
    if (source.empty())
    {
        return {};
    }
    const std::size_t offset = std::min(byteOffset, source.size() - 1);

    std::size_t begin = offset;
    while (begin > 0 && source[begin - 1] != '\n')
    {
        --begin;
    }
    std::size_t end = offset;
    while (end < source.size() && source[end] != '\n')
    {
        ++end;
    }

    std::string_view text = source.substr(begin, end - begin);
    while (!text.empty() && (text.back() == '\r' || text.back() == ' ' || text.back() == '\t'))
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
