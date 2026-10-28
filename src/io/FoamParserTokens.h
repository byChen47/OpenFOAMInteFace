#pragma once

#include "io/FoamNode.h"
#include "io/FoamToken.h"

namespace ofgui::io {

/// Maps a lexer token to the AST node kind used for a leaf value.
inline FoamNodeKind foamNodeKindOfToken(const FoamToken& token)
{
    switch (token.kind)
    {
    case FoamTokenKind::String:
        return FoamNodeKind::String;
    case FoamTokenKind::Number:
        return FoamNodeKind::Number;
    case FoamTokenKind::Variable:
        return FoamNodeKind::Variable;
    case FoamTokenKind::Expression:
        return FoamNodeKind::Expression;
    case FoamTokenKind::Verbatim:
        return FoamNodeKind::Verbatim;
    case FoamTokenKind::Directive:
        return FoamNodeKind::Directive;
    default:
        return FoamNodeKind::Word;
    }
}

/// True for tokens that may introduce a dictionary entry or a keyworded sub-dictionary.
inline bool foamTokenIsWord(const FoamToken& token)
{
    return token.kind == FoamTokenKind::Word;
}

} // namespace ofgui::io
