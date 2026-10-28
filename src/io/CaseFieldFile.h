#pragma once

#include "io/FieldProjector.h"

#include <string>

namespace ofgui::io {

/// One field dictionary read from the start time directory.
struct FieldFile
{
    /// Case relative POSIX path, e.g. `0/p`.
    std::string relativePath;
    ProjectedField projection;
    /// False when the dictionary could not be parsed (the file is then reported
    /// as unsupported but still listed).
    bool parsed = false;
    /// `FoamFile.format binary;` — the GUI never edits such a file.
    bool binary = false;
    /// At least one `#include`-style directive was found (spec section 5.5).
    bool partial = false;
    /// `FoamFile` header missing or unreadable.
    bool headerMissing = false;
};

} // namespace ofgui::io
