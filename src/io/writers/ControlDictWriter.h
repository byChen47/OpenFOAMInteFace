#pragma once

#include <core/CaseProject.h>

#include <string>

namespace ofgui::io {

/// Writes `system/controlDict` for a project.
///
/// The project model only carries the time controls; the remaining v2206 keys
/// are written with the values the incompressible tutorials use so that the
/// output stays a complete, runnable controlDict.
class ControlDictWriter
{
public:
    static std::string write(const core::CaseProject& project);
};

} // namespace ofgui::io
