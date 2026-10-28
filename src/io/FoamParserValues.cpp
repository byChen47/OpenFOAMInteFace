#include "io/FoamParser.h"

#include "io/FoamDiagnostic.h"
#include "io/FoamNode.h"
#include "io/FoamParserTokens.h"
#include "io/FoamSourceLocation.h"

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

namespace ofgui::io {
namespace {

/// `#include` family: never expanded (docs/CASE_IMPORT_SPEC.md ?5.5).
bool isIncludeDirective(std::string_view name)
{
    return name == "include" || name == "includeIfPresent" || name == "sinclude"
        || name == "includeEtc" || name == "includeFunc";
}

/// Directives that need OpenFOAM semantics; kept verbatim and never evaluated.
bool isEvaluatedDirective(std::string_view name)
{
    return name == "inputMode" || name == "remove" || name == "calc" || name == "eval"
        || name == "codeStream" || name == "if" || name == "ifeq" || name == "message";
}

/// Reports the directive as "kept verbatim" and returns its bare name.
std::string directiveName(const std::string& text)
{
    if (!text.empty() && text.front() == '#')
    {
        return text.substr(1);
    }
    return text;
}

/// A value with a single item is exposed as that item: `key  value;` yields the
/// leaf itself and `key ( ... );` yields the list, so consumers do not have to
/// unwrap one-element sequences (multi-token values such as `uniform 0` stay
/// sequences).
std::unique_ptr<FoamNode> unwrapSingleValue(std::unique_ptr<FoamNode> value)
{
    if (value != nullptr && value->childCount() == 1)
    {
        return value->takeChild(0);
    }
    return value;
}

} // namespace

std::unique_ptr<FoamNode> FoamParser::parseValueSequence(const FoamToken& keyword)
{
    auto value = std::make_unique<FoamNode>(FoamNodeKind::Sequence, locationOf(keyword));

    for (;;)
    {
        const FoamToken* token = peek(0);
        if (!token)
        {
            report(
                FoamSeverity::Error,
                "IMPORT-PARSE-001",
                "missing ';' at end of file for entry '" + keyword.text + "'",
                keyword);
            return unwrapSingleValue(std::move(value));
        }

        switch (token->kind)
        {
        case FoamTokenKind::Semicolon:
        {
            take();
            return unwrapSingleValue(std::move(value));
        }

        case FoamTokenKind::CloseBrace:
            report(
                FoamSeverity::Error,
                "IMPORT-PARSE-001",
                "missing ';' before '}' for entry '" + keyword.text + "'",
                keyword);
            return unwrapSingleValue(std::move(value));

        case FoamTokenKind::OpenParen:
        {
            const FoamToken open = take();
            value->addChild(parseList(open));
            continue;
        }

        case FoamTokenKind::OpenBracket:
        {
            const FoamToken open = take();
            value->addChild(parseDimensionSet(open));
            continue;
        }

        case FoamTokenKind::OpenBrace:
        {
            // Anonymous sub-dictionary as part of a value, e.g. `patchInfo { ... }`.
            const FoamToken open = take();
            value->addChild(parseDictionaryBody(open));
            continue;
        }

        case FoamTokenKind::CloseParen:
        case FoamTokenKind::CloseBracket:
            report(
                FoamSeverity::Error,
                "IMPORT-PARSE-006",
                "unexpected '" + std::string{foamTokenKindName(token->kind)}
                    + "' while reading the value of '" + keyword.text + "'",
                *token);
            take();
            continue;

        default:
        {
            const FoamToken atom = take();
            if (value->childCount() == 0)
            {
                value->setLocation(locationOf(atom));
            }
            value->addChild(makeLeaf(atom));
            if (detectMissingSemicolon(keyword, atom))
            {
                return unwrapSingleValue(std::move(value));
            }
            continue;
        }
        }
    }
}

std::unique_ptr<FoamNode> FoamParser::parseList(const FoamToken& open)
{
    auto list = std::make_unique<FoamNode>(FoamNodeKind::List, locationOf(open));

    struct DepthGuard
    {
        FoamParser& parser;
        ~DepthGuard() { parser.leaveNesting(); }
    } guard{*this};
    enterNesting(open);
    if (nestingExceeded())
    {
        skipNestedBody();
        return list;
    }

    for (;;)
    {
        const FoamToken* token = peek(0);
        if (!token)
        {
            report(
                FoamSeverity::Error,
                "IMPORT-PARSE-002",
                "missing ')' for the list opened on line " + std::to_string(open.line),
                open);
            break;
        }

        switch (token->kind)
        {
        case FoamTokenKind::CloseParen:
            take();
            return list;

        case FoamTokenKind::Semicolon:
        case FoamTokenKind::CloseBrace:
            report(
                FoamSeverity::Error,
                "IMPORT-PARSE-002",
                "missing ')' for the list opened on line " + std::to_string(open.line),
                open);
            return list;

        case FoamTokenKind::OpenParen:
        {
            const FoamToken nested = take();
            list->addChild(parseList(nested));
            continue;
        }

        case FoamTokenKind::OpenBracket:
        {
            const FoamToken bracket = take();
            list->addChild(parseDimensionSet(bracket));
            continue;
        }

        case FoamTokenKind::OpenBrace:
        {
            const FoamToken brace = take();
            list->addChild(parseDictionaryBody(brace));
            continue;
        }

        case FoamTokenKind::CloseBracket:
            report(FoamSeverity::Error, "IMPORT-PARSE-006", "unexpected ']' inside a list", *token);
            take();
            continue;

        default:
        {
            const FoamToken atom = take();
            // Keyworded sub-dictionary inside a list: `( movingWall { type wall; } ... )`.
            if (foamTokenIsWord(atom))
            {
                const FoamToken* next = peek(0);
                if (next && next->kind == FoamTokenKind::OpenBrace)
                {
                    const FoamToken brace = take();
                    std::unique_ptr<FoamNode> sub = parseDictionaryBody(brace);
                    sub->setKeyword(atom.text, atom.kind == FoamTokenKind::String);
                    list->addChild(std::move(sub));
                    continue;
                }
            }
            list->addChild(makeLeaf(atom));
            continue;
        }
        }
    }
    return list;
}

std::unique_ptr<FoamNode> FoamParser::parseDimensionSet(const FoamToken& open)
{
    auto dimensions = std::make_unique<FoamNode>(FoamNodeKind::DimensionSet, locationOf(open));

    for (;;)
    {
        const FoamToken* token = peek(0);
        if (!token)
        {
            report(
                FoamSeverity::Error,
                "IMPORT-PARSE-002",
                "missing ']' for the dimension set opened on line " + std::to_string(open.line),
                open);
            break;
        }
        if (token->kind == FoamTokenKind::CloseBracket)
        {
            take();
            break;
        }
        if (token->kind == FoamTokenKind::Semicolon || token->kind == FoamTokenKind::CloseBrace
            || token->kind == FoamTokenKind::CloseParen)
        {
            report(
                FoamSeverity::Error,
                "IMPORT-PARSE-002",
                "missing ']' for the dimension set opened on line " + std::to_string(open.line),
                open);
            break;
        }
        if (token->kind == FoamTokenKind::OpenParen || token->kind == FoamTokenKind::OpenBrace
            || token->kind == FoamTokenKind::OpenBracket)
        {
            report(
                FoamSeverity::Error,
                "IMPORT-PARSE-006",
                "unexpected '" + std::string{foamTokenKindName(token->kind)}
                    + "' inside a dimension set",
                *token);
            take();
            continue;
        }

        const FoamToken atom = take();
        dimensions->addChild(makeLeaf(atom));
    }
    return dimensions;
}

void FoamParser::parseDirectiveInto(FoamNode& dictionary)
{
    const FoamToken directive = take();
    auto node = makeLeaf(directive);

    // Directives are preserved verbatim, never expanded or executed. The
    // import layer reports them so the UI can flag "this file needs OpenFOAM
    // to be complete" instead of silently dropping information.
    const std::string name = directiveName(directive.text);
    if (isIncludeDirective(name))
    {
        report(
            FoamSeverity::Warning,
            "IMPORT-DIR-001",
            "include directive '#" + name + "' is not expanded; the entry is kept verbatim",
            directive);
    }
    else if (isEvaluatedDirective(name))
    {
        report(
            FoamSeverity::Warning,
            "IMPORT-DIR-002",
            "directive '#" + name + "' is not evaluated; the entry is kept verbatim",
            directive);
    }
    else
    {
        report(
            FoamSeverity::Warning,
            "IMPORT-DIR-003",
            "unsupported directive '#" + name + "' is kept verbatim",
            directive);
    }

    for (;;)
    {
        const FoamToken* token = peek(0);
        if (!token)
        {
            break;
        }
        if (token->kind == FoamTokenKind::OpenBrace)
        {
            // `#codeStream { ... }` and friends: keep the block opaque.
            const FoamToken open = take();
            node->addChild(parseDictionaryBody(open));
            break;
        }
        if (token->kind == FoamTokenKind::Semicolon)
        {
            take();
            break;
        }
        if (token->line != directive.line)
        {
            break;
        }
        const FoamToken argument = take();
        node->addChild(makeLeaf(argument));
    }

    dictionary.addChild(std::move(node));
}

std::unique_ptr<FoamNode> FoamParser::makeLeaf(const FoamToken& token) const
{
    auto node = std::make_unique<FoamNode>(foamNodeKindOfToken(token), locationOf(token));
    node->setRawText(token.text);
    return node;
}

bool FoamParser::detectMissingSemicolon(const FoamToken& keyword, const FoamToken& lastAtom)
{
    if (!options_.recoverMissingSemicolon)
    {
        return false;
    }

    const FoamToken* next = peek(0);
    const FoamToken* after = peek(1);
    if (!next || !after)
    {
        return false;
    }
    if (!foamTokenIsWord(*next) || next->line <= lastAtom.line)
    {
        return false;
    }
    if (after->kind != FoamTokenKind::OpenBrace)
    {
        return false;
    }

    report(
        FoamSeverity::Error,
        "IMPORT-PARSE-001",
        "missing ';' after the value of '" + keyword.text + "': '" + next->text
            + "' on line " + std::to_string(next->line) + " starts a new entry",
        lastAtom);
    return true;
}


} // namespace ofgui::io
