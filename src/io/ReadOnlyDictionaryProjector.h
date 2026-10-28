#pragma once

#include "io/CaseProjectionContext.h"
#include "io/ImportedSetting.h"

#include <cstddef>
#include <string_view>
#include <vector>

namespace ofgui::io {

class FoamDictionary;

/// Flattens a dictionary into read-only keyword/value pairs.
///
/// Used for the dictionaries the GUI does not edit yet (`fvSchemes`,
/// `fvSolution`, `decomposeParDict`, `turbulenceProperties`) and for the parts of
/// `controlDict` outside `SolverSettings` (spec section 4.4). Nested dictionaries
/// become dotted paths; lists and multi token values keep their original text.
class ReadOnlyDictionaryProjector
{
public:
    /// @param maxEntries stop after this many pairs (a truncated file is still
    ///        usable); pass `0` for no limit.
    [[nodiscard]] static std::vector<ImportedSetting> flatten(
        const FoamDictionary& dictionary,
        std::string_view relativePath,
        std::size_t maxEntries = 4000);
};

} // namespace ofgui::io
