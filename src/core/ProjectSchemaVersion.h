#pragma once

namespace ofgui::core {

/// Version of the `.ofgproj` document schema written by this build.
///
/// Migration code lives with the project JSON reader; this constant is the single
/// place that names the current version.
inline constexpr int kCurrentSchemaVersion = 1;

/// File name suffix of the GUI project file.
inline constexpr const char* kProjectFileExtension = ".ofgproj";

} // namespace ofgui::core
