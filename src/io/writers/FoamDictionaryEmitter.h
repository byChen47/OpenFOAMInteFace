#pragma once

#include "io/FoamNode.h"

#include <string>

namespace ofgui::io {

struct FoamEmitOptions
{
    /// Replace a bare `$name;` entry by the entries of the sibling dictionary
    /// `name` (OpenFOAM's merge mechanism). Used for `fvSolution` solvers.
    bool expandVariableMerges{false};

    /// Put one blank line between the entries of a nested dictionary. The
    /// scheme/solution dictionaries keep their entries adjacent, so this stays
    /// off by default.
    bool blankLineBetweenEntries{false};
};

/// Text of a leaf node, including the delimiters of strings and verbatim
/// blocks (`"name"`, `#{ ... #}`).
std::string emitFoamLeafText(const FoamNode& node);

/// Canonical text of a dictionary body (no enclosing braces), entries
/// separated by one blank line, indented from `level`.
std::string emitFoamDictionary(const FoamNode& dictionary, int level, const FoamEmitOptions& options = {});

/// Canonical text of one entry (keyword + value + `;`), or of a nested
/// dictionary block, at `level`.
std::string emitFoamEntry(const FoamNode& entry, int level, const FoamEmitOptions& options = {});

} // namespace ofgui::io
