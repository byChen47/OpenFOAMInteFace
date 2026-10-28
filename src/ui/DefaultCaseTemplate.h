#pragma once

#include "core/CaseProject.h"
#include "io/writers/TransportModelSettings.h"

#include <string_view>

namespace ofgui {

/// A ready-to-export project used for the first launch and for "新建工程".
///
/// The template mirrors the v2206 `icoFoam/cavity` tutorial in model form:
/// 0.1 x 0.1 x 0.01 m, 20 x 20 x 1 cells, `fixedWalls` / `movingWall` /
/// `frontAndBack` patches, `p` and `U`, and `icoFoam` timing. The UI therefore
/// starts with a model that passes `core::CaseValidator`, so the export workflow
/// can be exercised end to end without any manual file editing.
struct DefaultCaseTemplate
{
    core::CaseProject project;
    io::TransportModelSettings transport;
};

/// Builds the template project named `name`.
[[nodiscard]] DefaultCaseTemplate makeDefaultCavityTemplate(std::string_view name);

} // namespace ofgui
