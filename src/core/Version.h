#pragma once

#include <string>
#include <string_view>

namespace ofgui::core {

/// Semantic version of the core module.
/// Bumped when the public model API changes in an incompatible way.
struct ModuleVersion
{
    int major = 0;
    int minor = 0;
    int patch = 0;
};

inline constexpr ModuleVersion kCoreModuleVersion{0, 1, 0};
inline constexpr std::string_view kCoreModuleName{"ofgui_core"};

/// "major.minor.patch", the only place that formats a ModuleVersion.
std::string formatModuleVersion(const ModuleVersion& version);

/// "ofgui_core 0.1.0" - used by diagnostics and tests to prove the module links.
std::string coreModuleBanner();

} // namespace ofgui::core
