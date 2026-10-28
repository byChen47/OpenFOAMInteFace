#pragma once

#include "io/FoamDictionary.h"

#include <string>

namespace ofgui::io {

/// Re-emits a `system/fvSolution` dictionary in the canonical layout.
///
/// Solver entries that are merged with `$name;` are expanded in place, so the
/// exported file spells out the inherited coefficients instead of relying on
/// the run-time merge.
class FvSolutionWriter
{
public:
    static std::string write(const FoamDictionary& source);
};

} // namespace ofgui::io
