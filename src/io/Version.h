#pragma once

#include "core/Version.h"

#include <string>
#include <string_view>

namespace ofgui::io {

/// Semantic version of the io module.
/// Bumped when the public IO API changes in an incompatible way.
inline constexpr core::ModuleVersion kIoModuleVersion{0, 1, 0};
inline constexpr std::string_view kIoModuleName{"ofgui_io"};

/// "ofgui_io 0.1.0" - reuses `core::formatModuleVersion` so the banner also
/// proves that the `ofgui_io -> ofgui_core` link edge exists.
std::string ioModuleBanner();

} // namespace ofgui::io
