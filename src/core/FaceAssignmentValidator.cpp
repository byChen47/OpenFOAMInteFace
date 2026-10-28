#include "core/FaceAssignmentValidator.h"

#include <algorithm>

namespace ofgui::core {

FaceAssignmentValidator::Coverage FaceAssignmentValidator::coverage(
    const std::vector<BoundaryPatch>& patches)
{
    Coverage coverage;
    for (std::size_t index = 0; index < kAllFaceIds.size(); ++index)
    {
        coverage[index].face = kAllFaceIds[index];
    }

    for (const BoundaryPatch& patch : patches)
    {
        for (const FaceId face : patch.faces())
        {
            coverage[faceIndex(face)].patchIds.push_back(patch.id());
        }
    }

    return coverage;
}

std::vector<FaceId> FaceAssignmentValidator::unassignedFaces(const std::vector<BoundaryPatch>& patches)
{
    std::vector<FaceId> faces;
    for (const FaceCoverage& face : coverage(patches))
    {
        if (!face.isAssigned())
        {
            faces.push_back(face.face);
        }
    }

    return faces;
}

std::vector<FaceId> FaceAssignmentValidator::multiplyAssignedFaces(
    const std::vector<BoundaryPatch>& patches)
{
    std::vector<FaceId> faces;
    for (const FaceCoverage& face : coverage(patches))
    {
        if (face.isAmbiguous())
        {
            faces.push_back(face.face);
        }
    }

    return faces;
}

bool FaceAssignmentValidator::isFullyAssigned(const std::vector<BoundaryPatch>& patches)
{
    const Coverage faces = coverage(patches);
    return std::all_of(faces.begin(), faces.end(), [](const FaceCoverage& face) {
        return face.isAssigned() && !face.isAmbiguous();
    });
}

} // namespace ofgui::core
