#pragma once

#include <core/Basics.h>

#include <string>

namespace ofgui::io {

/// Physics settings for `constant/transportProperties` that are not part of the
/// geometric project model.
struct TransportModelSettings
{
    /// Empty means "not specified": only `nu` is written, which is what the
    /// incompressible tutorials rely on.
    std::string transportModel;

    /// `nu` as a plain number.
    core::Real kinematicViscosity{1e-05};

    /// When non-empty this text replaces the formatted number, so a source
    /// dictionary written as `nu nu [0 2 -1 0 0 0 0] 0.01;` round-trips.
    std::string kinematicViscosityText;
};

} // namespace ofgui::io
