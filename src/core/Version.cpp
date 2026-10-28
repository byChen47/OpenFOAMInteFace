#include "core/Version.h"

#include <string>

namespace ofgui::core {

std::string formatModuleVersion(const ModuleVersion& version)
{
    return std::to_string(version.major) + '.' + std::to_string(version.minor) + '.'
           + std::to_string(version.patch);
}

std::string coreModuleBanner()
{
    return std::string{kCoreModuleName} + ' ' + formatModuleVersion(kCoreModuleVersion);
}

} // namespace ofgui::core
