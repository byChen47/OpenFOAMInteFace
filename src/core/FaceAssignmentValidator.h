#pragma once

#include "core/BoundaryPatch.h"
#include "core/FaceId.h"
#include "core/ObjectId.h"

#include <array>
#include <vector>

namespace ofgui::core {

/// Answers "which patch owns which face" for a patch list.
///
/// The model deliberately allows a face to be unassigned or shared, so the
/// invariant "every face belongs to exactly one patch" is a validation result,
/// not a constructor precondition. Keeping the query in its own class stops
/// `CaseValidator` from growing a second responsibility.
class FaceAssignmentValidator
{
public:
    struct FaceCoverage
    {
        FaceId face = FaceId::XMin;
        /// Patches claiming this face, in patch-list order. Empty when unassigned.
        std::vector<ObjectId> patchIds;

        [[nodiscard]] bool isAssigned() const noexcept
        {
            return !patchIds.empty();
        }

        [[nodiscard]] bool isAmbiguous() const noexcept
        {
            return patchIds.size() > 1;
        }
    };

    using Coverage = std::array<FaceCoverage, 6>;

    /// Coverage of `kAllFaceIds` in enum order.
    [[nodiscard]] static Coverage coverage(const std::vector<BoundaryPatch>& patches);

    /// Faces claimed by no patch, in enum order.
    [[nodiscard]] static std::vector<FaceId> unassignedFaces(const std::vector<BoundaryPatch>& patches);

    /// Faces claimed by more than one patch, in enum order.
    [[nodiscard]] static std::vector<FaceId> multiplyAssignedFaces(
        const std::vector<BoundaryPatch>& patches);

    /// True when every face is claimed by exactly one patch.
    [[nodiscard]] static bool isFullyAssigned(const std::vector<BoundaryPatch>& patches);
};

} // namespace ofgui::core
