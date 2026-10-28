#pragma once

#include "io/CaseProjectionContext.h"

#include "core/BoundaryPatch.h"
#include "core/DomainBox.h"
#include "core/FaceId.h"
#include "core/MeshSettings.h"

#include <string>
#include <string_view>
#include <vector>

namespace ofgui::io {

class FoamDictionary;

/// One `boundary` entry of `blockMeshDict` projected into the model.
struct ProjectedPatch
{
    std::string name;
    core::PatchType type = core::PatchType::Patch;
    /// False when the type word is not one the GUI models.
    bool typeKnown = true;
    /// Faces matched by vertex set; empty when the quad could not be matched.
    std::vector<core::FaceId> faces;
    /// True for a quoted regular expression key (`".*"`); such keys never enter
    /// the model (spec section 6.3).
    bool regularExpression = false;
};

/// Result of projecting `system/blockMeshDict`.
struct BlockMeshProjection
{
    bool hasDomain = false;
    core::DomainBox domain;
    bool hasMesh = false;
    core::MeshSettings mesh;
    /// At least one `BlockingError` was reported (multi block, non hex, cells).
    bool blocking = false;
    std::vector<ProjectedPatch> patches;
};

/// Reads the single hex block of `blockMeshDict` and projects it into
/// `DomainBox`, `MeshSettings` and the patch name list (spec sections 4.4, 6.2).
class BlockMeshProjector
{
public:
    [[nodiscard]] static BlockMeshProjection project(
        const FoamDictionary& dictionary,
        const CaseProjectionContext& context);
};

} // namespace ofgui::io
