#pragma once

#include "io/FoamDictionary.h"
#include "io/FoamLexer.h"
#include "io/FoamParseOptions.h"
#include "io/FoamToken.h"

#include <cstddef>
#include <deque>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace ofgui::io {

class FoamNode;

/// Recursive-descent parser for the OpenFOAM v2206 dictionary subset described
/// in docs/CASE_IMPORT_SPEC.md §5.
///
/// Responsibilities are limited to structure: comments are dropped, directives
/// are kept as opaque nodes, `$variables` and `#{}` blocks are never expanded or
/// evaluated. Errors are collected as diagnostics with file/line/column and the
/// parser resynchronises on the next entry so one broken value does not lose the
/// rest of the file.
class FoamParser
{
public:
    FoamParser(std::string_view source, std::string fileLabel, FoamParseOptions options = {});

    /// Parse the whole source into a document. Safe to call once per parser.
    FoamDictionary parse();

private:
    const FoamToken* peek(std::size_t ahead = 0);
    FoamToken take();

    void parseEntryInto(FoamNode& dictionary);
    std::unique_ptr<FoamNode> parseDictionaryBody(const FoamToken& open);
    std::unique_ptr<FoamNode> parseValueSequence(const FoamToken& keyword);
    std::unique_ptr<FoamNode> parseList(const FoamToken& open);
    std::unique_ptr<FoamNode> parseDimensionSet(const FoamToken& open);
    void parseDirectiveInto(FoamNode& dictionary);

    std::unique_ptr<FoamNode> makeLeaf(const FoamToken& token) const;
    bool detectMissingSemicolon(const FoamToken& keyword, const FoamToken& lastAtom);
    void attachEntry(FoamNode& dictionary, std::unique_ptr<FoamNode> entry, const FoamToken& keyword);

    /// Nesting guard: every `{`/`(` level is counted and bounded by
    /// `FoamParseOptions::maxNestingDepth` so a pathological file cannot blow the
    /// stack (IMPORT-PARSE-009).
    void enterNesting(const FoamToken& open);
    void leaveNesting() noexcept;
    bool nestingExceeded() const noexcept;
    void skipNestedBody();

    FoamSourceLocation locationOf(const FoamToken& token) const;
    void report(FoamSeverity severity, std::string code, std::string message, const FoamToken& token);
    void reportAt(FoamSeverity severity, std::string code, std::string message, std::size_t byteOffset);
    std::string makeSnippet(std::size_t byteOffset) const;

    FoamLexer lexer_;
    FoamParseOptions options_;
    std::deque<FoamToken> buffer_;
    bool exhausted_{false};
    std::size_t depth_{0};
    std::vector<FoamDiagnostic> diagnostics_;
};

} // namespace ofgui::io
