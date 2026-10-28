#pragma once

#include "io/FoamDiagnostic.h"
#include "io/FoamToken.h"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ofgui::io {

/// Byte-level tokenizer for OpenFOAM v2206 dictionaries.
///
/// The lexer owns no AST and performs no expansion: `$variables`, `#directives`
/// and `#{ ... #}` blocks are recognised as single tokens so the parser can keep
/// them verbatim. Lexing is strictly forward-only and never throws; problems are
/// reported as diagnostics and lexing continues at the next token boundary.
///
/// Line/column information is derived from a pre-computed line index, so token
/// positions cost O(log lines) and no state has to be rewound on error.
class FoamLexer
{
public:
    explicit FoamLexer(std::string_view source, std::string fileLabel = std::string{});

    /// Next token, or `std::nullopt` at end of input.
    std::optional<FoamToken> next();

    const std::vector<FoamDiagnostic>& diagnostics() const noexcept;
    std::string_view source() const noexcept;
    const std::string& fileLabel() const noexcept;
    std::size_t byteOffset() const noexcept;

private:
    void buildLineIndex();
    std::size_t lineOf(std::size_t byteOffset) const;
    std::size_t columnOf(std::size_t byteOffset) const;

    void skipTrivia();
    std::optional<FoamToken> readString();
    std::optional<FoamToken> readDirective();
    std::optional<FoamToken> readVerbatim();
    std::optional<FoamToken> readVariable();
    std::optional<FoamToken> readNumberOrWord();
    std::optional<FoamToken> readWord();

    FoamToken makeToken(FoamTokenKind kind, std::size_t begin, std::string text) const;
    void report(FoamSeverity severity, std::string code, std::string message, std::size_t byteOffset);
    std::string makeSnippet(std::size_t byteOffset) const;

    std::string_view source_;
    std::string fileLabel_;
    std::size_t pos_{0};
    std::vector<std::size_t> lineStarts_;
    std::vector<FoamDiagnostic> diagnostics_;
};

} // namespace ofgui::io
