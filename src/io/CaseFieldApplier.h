#pragma once

#include "io/BlockMeshProjector.h"
#include "io/CaseFieldFile.h"
#include "io/ImportReport.h"

#include "core/CaseProject.h"

#include <vector>

namespace ofgui::io {

/// Creates the patch list and the editable fields of `project`.
///
/// Order matters: the patch list is complete before any boundary condition is
/// attached, so a patch that only appears in a field file is still addressable
/// (spec sections 6.3 and 6.4).
class CaseFieldApplier
{
public:
    /// @param blockMesh nullable; when present its patch names, types and faces
    ///        define the patch list, otherwise patch names come from the fields.
    static void apply(
        const std::vector<FieldFile>& fields,
        const BlockMeshProjection* blockMesh,
        core::CaseProject& project,
        ImportReport& report);
};

} // namespace ofgui::io
