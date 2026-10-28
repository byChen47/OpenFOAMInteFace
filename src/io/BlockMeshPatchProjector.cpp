#include "io/BlockMeshPatchProjector.h"

#include "io/FoamDictionary.h"
#include "io/FoamNode.h"
#include "io/BlockMeshValueReader.h"
#include "io/ImportCodes.h"
#include "io/ImportIssueFactory.h"

#include "core/FaceId.h"

#include <algorithm>
#include <array>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace ofgui::io {
namespace {

using core::FaceId;

/// Vertex ids of the six canonical block faces (blockMesh hex ordering).
constexpr std::array<std::array<int, 4>, 6> kBlockFaces{{
    {{0, 3, 4, 7}}, // xMin
    {{1, 2, 5, 6}}, // xMax
    {{0, 1, 4, 5}}, // yMin
    {{2, 3, 6, 7}}, // yMax
    {{0, 1, 2, 3}}, // zMin
    {{4, 5, 6, 7}}, // zMax
}};

/// The `faces` quad of one patch, matched against the six block faces.
std::optional<FaceId> matchFace(const std::array<int, 4>& quad)
{
    std::array<int, 4> sorted = quad;
    std::sort(sorted.begin(), sorted.end());
    for (std::size_t index = 0; index < kBlockFaces.size(); ++index)
    {
        std::array<int, 4> candidate = kBlockFaces[index];
        std::sort(candidate.begin(), candidate.end());
        if (candidate == sorted)
        {
            return static_cast<FaceId>(index);
        }
    }
    return std::nullopt;
}

std::string faceKey(FaceId face)
{
    return std::string(core::faceName(face));
}

} // namespace

void projectBlockMeshPatches(const FoamDictionary& dictionary,
                             const std::string& file,
                             const CaseProjectionContext& context,
                             BlockMeshProjection& projection)
{
    const FoamNode* boundaryNode = dictionary.find("boundary");
    std::map<std::string, FaceId> faceOwner;
    if (boundaryNode != nullptr && boundaryNode->isList())
    {
        for (const auto& patchNode : boundaryNode->children())
        {
            if (patchNode == nullptr || !patchNode->isDictionary())
            {
                continue;
            }
            ProjectedPatch patch;
            patch.name = patchNode->keyword();
            patch.regularExpression = patchNode->keywordIsString();
            if (patch.regularExpression)
            {
                context.issues.push_back(makeImportIssue(importcodes::kPatchRegexKey,
                                                         file,
                                                         patchNode->location().line,
                                                         patchNode->location().column,
                                                         patchNode->keyword(),
                                                         patch.name));
                projection.patches.push_back(std::move(patch));
                continue;
            }
            if (patch.name.empty())
            {
                context.issues.push_back(makeImportIssue(
                    importcodes::kPatchEmptyName, file, patchNode->location().line, 0, "boundary"));
                continue;
            }

            const FoamNode* typeNode = patchNode->findChild("type");
            const std::string typeText = typeNode != nullptr ? typeNode->rawText() : std::string{};
            if (const std::optional<core::PatchType> type = core::patchTypeFromName(typeText))
            {
                patch.type = *type;
            }
            else
            {
                patch.typeKnown = false;
                context.issues.push_back(makeImportIssue(
                    importcodes::kPatchUnknownType,
                    file,
                    typeNode != nullptr ? typeNode->location().line : patchNode->location().line,
                    typeNode != nullptr ? typeNode->location().column : 0,
                    patch.name + ".type",
                    typeText));
            }

            if (const FoamNode* facesNode = patchNode->findChild("faces");
                facesNode != nullptr && facesNode->isList())
            {
                for (const auto& quadNode : facesNode->children())
                {
                    std::vector<int> quad;
                    if (quadNode == nullptr || !blockMeshIntList(quadNode.get(), quad) || quad.size() != 4)
                    {
                        continue;
                    }
                    const std::array<int, 4> ids{quad[0], quad[1], quad[2], quad[3]};
                    const std::optional<FaceId> face = matchFace(ids);
                    if (!face.has_value())
                    {
                        continue;
                    }
                    const std::string key = faceKey(*face);
                    const auto existing = faceOwner.find(key);
                    if (existing != faceOwner.end())
                    {
                        context.issues.push_back(makeImportIssue(importcodes::kPatchFaceConflict,
                                                                 file,
                                                                 patchNode->location().line,
                                                                 patchNode->location().column,
                                                                 patch.name + ".faces",
                                                                 key));
                        continue;
                    }
                    faceOwner.emplace(key, *face);
                    patch.faces.push_back(*face);
                }
            }
            projection.patches.push_back(std::move(patch));
        }
    }
}

} // namespace ofgui::io
