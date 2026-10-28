#pragma once

#include "io/FoamDictionary.h"

#include <string>

namespace ofgui::io {

/// Re-emits a `system/fvSchemes` dictionary in the canonical layout, keeping
/// the entry order of the parsed source.
class FvSchemesWriter
{
public:
    static std::string write(const FoamDictionary& source);
};

} // namespace ofgui::io
