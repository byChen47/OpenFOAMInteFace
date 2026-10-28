#pragma once

#include "io/FoamSourceLocation.h"

#include <cstddef>
#include <string>
#include <string_view>

namespace ofgui::io {

/// Token kinds produced by `FoamLexer`.
///
/// The alphabet mirrors OpenFOAM v2206 `token::tokenType` / `token::punctuationToken`
/// (OpenFOAM-v2206/src/OpenFOAM/db/IOstreams/token/token.H:121-165) with one
/// deliberate difference: every punctuation character keeps its own kind so the
/// parser never has to compare `text` to decide structure.
enum class FoamTokenKind
{
    Word,       ///< unquoted word (may contain balanced `(...)`, e.g. `div(phi,U)`)
    String,     ///< `"..."`, `text` holds the content without the quotes
    Number,     ///< integer / floating point / scientific notation, raw text kept
    Verbatim,   ///< `#{ ... #}`, `text` holds the bytes between the delimiters
    Variable,   ///< `$name`, `${name}`, `$.name`
    Expression, ///< `${{ ... }}` (never evaluated, kept as raw text)
    Directive,  ///< `#include`, `#includeEtc`, `#inputMode`, ... (never executed)

    Semicolon,
    OpenBrace,
    CloseBrace,
    OpenParen,
    CloseParen,
    OpenBracket,
    CloseBracket,
    Colon,
    Comma,
    Assign,
    Plus,
    Minus,
    Multiply,
    Divide,
};

/// One lexical token with its exact source text and position.
///
/// `text` is always a byte-exact slice of the source (quotes stripped for
/// strings, delimiters stripped for verbatim blocks), which is what makes
/// loss-free read-modify-write possible.
struct FoamToken
{
    FoamTokenKind kind{FoamTokenKind::Word};
    std::string text;
    std::size_t byteOffset{0};
    int line{0};
    int column{0};

    FoamSourceLocation location(std::string_view fileLabel) const;
    /// True for tokens that may start a dictionary entry (`word`, `"string"`, number).
    bool isKeywordCandidate() const;
    /// True for tokens that can stand alone as a value atom.
    bool isLeafValue() const;
    /// Punctuation character of this token, or '\0' if the token is not punctuation.
    char punctuation() const;
};

bool foamTokenIsPunctuation(FoamTokenKind kind);
char foamTokenPunctuationChar(FoamTokenKind kind);
std::string_view foamTokenKindName(FoamTokenKind kind);

/// Characters allowed inside an unquoted OpenFOAM word.
///
/// Mirrors `word::valid()` (OpenFOAM-v2206/src/OpenFOAM/primitives/strings/word/wordI.H:59):
/// everything except whitespace, both quotes, '/', ';', '{' and '}'.
bool isFoamWordChar(char c);

/// Locale-independent number parsing (`1`, `-2`, `.5`, `1e-06`, `+3.0`).
bool parseFoamReal(std::string_view text, double& value);
bool parseFoamInt(std::string_view text, long long& value);

} // namespace ofgui::io
