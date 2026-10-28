#include "io/Version.h"

#include <string>

namespace ofgui::io {

std::string ioModuleBanner()
{
    return std::string{kIoModuleName} + ' ' + core::formatModuleVersion(kIoModuleVersion);
}

} // namespace ofgui::io
