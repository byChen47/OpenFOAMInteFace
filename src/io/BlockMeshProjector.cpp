#include "io/BlockMeshProjector.h"

#include "io/BlockMeshPatchProjector.h"
#include "io/BlockMeshValueReader.h"

#include "io/CaseImportOptions.h"
#include "io/FoamDictionary.h"
#include "io/FoamNode.h"
#include "io/FoamToken.h"
#include "io/FoamValue.h"
#include "io/ImportCodes.h"
#include "io/ImportIssueFactory.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace ofgui::io {
namespace {

using core::FaceId;

/// Block kinds `blockMesh` accepts; a word from this set starts a new `blocks` entry.
bool startsBlockDescription(const FoamNode& node)
{
    if (!node.isLeaf())
    {
        return false;
    }
    const std::string& text = node.rawText();
    return text == "hex" || text == "wedge" || text == "prism" || text == "tet" || text == "tetWedge"
        || text == "pyramid" || text == "polyLine" || text == "splitHex" || text == "splitWedge"
        || text == "splitPrism";
}


struct Vertices
{
    bool ok = false;
    std::vector<std::array<double, 3>> points;
    std::array<double, 3> min{};
    std::array<double, 3> max{};
};

Vertices readVertices(const FoamNode* node, double scaleX, double scaleY, double scaleZ)
{
    Vertices result;
    if (node == nullptr || !node->isList() || node->childCount() != 8)
    {
        return result;
    }
    const std::array<double, 3> scale{scaleX, scaleY, scaleZ};
    for (const auto& child : node->children())
    {
        std::vector<double> point;
        if (!blockMeshRealList(child.get(), point) || point.size() != 3)
        {
            return result;
        }
        std::array<double, 3> scaled{point[0] * scale[0], point[1] * scale[1], point[2] * scale[2]};
        result.points.push_back(scaled);
    }
    for (std::size_t axis = 0; axis < 3; ++axis)
    {
        double low = result.points.front()[axis];
        double high = low;
        for (const auto& point : result.points)
        {
            low = std::min(low, point[axis]);
            high = std::max(high, point[axis]);
        }
        result.min[axis] = low;
        result.max[axis] = high;
    }
    result.ok = true;
    return result;
}

/// True when every axis takes exactly two distinct coordinate values.
bool formsCuboid(const Vertices& vertices)
{
    for (std::size_t axis = 0; axis < 3; ++axis)
    {
        std::set<double> distinct;
        for (const auto& point : vertices.points)
        {
            distinct.insert(point[axis]);
        }
        if (distinct.size() != 2)
        {
            return false;
        }
    }
    return true;
}

} // namespace

BlockMeshProjection BlockMeshProjector::project(
    const FoamDictionary& dictionary, const CaseProjectionContext& context)
{
    BlockMeshProjection projection;
    const std::string& file = context.relativePath;

    // scale: scalar or three values (spec section 4.4).
    double scale = 1.0;
    std::array<double, 3> scaleAxes{1.0, 1.0, 1.0};
    if (const FoamNode* scaleNode = dictionary.find("scale"))
    {
        std::vector<double> values;
        if (blockMeshRealList(scaleNode, values) && values.size() == 3)
        {
            scaleAxes = {values[0], values[1], values[2]};
            scale = scaleAxes[2];
            if (values[0] != values[1] || values[1] != values[2])
            {
                context.issues.push_back(makeImportIssue(
                    importcodes::kMeshAnisotropicScale,
                    file,
                    scaleNode->location().line,
                    scaleNode->location().column,
                    "scale",
                    FoamValue(*scaleNode).text()));
            }
        }
        else
        {
            double scalar = 0.0;
            if (blockMeshToReal(scaleNode, scalar))
            {
                scale = scalar;
                scaleAxes = {scalar, scalar, scalar};
            }
            else
            {
                context.issues.push_back(makeImportIssue(
                    importcodes::kMeshAnisotropicScale,
                    file,
                    scaleNode->location().line,
                    scaleNode->location().column,
                    "scale",
                    FoamValue(*scaleNode).text()));
            }
        }
    }
    else
    {
        context.issues.push_back(makeImportIssue(importcodes::kMeshMissingScale, file, 0, 0, "scale"));
    }

    // vertices
    //
    // A bad vertex list must not stop the projection: the block list below still
    // decides whether the case is single block (importable) or multi block
    // (blocking), and that verdict is what the import report needs.
    const FoamNode* verticesNode = dictionary.find("vertices");
    Vertices vertices = readVertices(verticesNode, scaleAxes[0], scaleAxes[1], scaleAxes[2]);
    if (verticesNode == nullptr || !verticesNode->isList() || verticesNode->childCount() != 8)
    {
        context.issues.push_back(makeImportIssue(
            importcodes::kMeshVertexCount,
            file,
            verticesNode != nullptr ? verticesNode->location().line : 0,
            verticesNode != nullptr ? verticesNode->location().column : 0,
            "vertices"));
    }
    else if (!vertices.ok)
    {
        context.issues.push_back(makeImportIssue(
            importcodes::kMeshVertexCount,
            file,
            verticesNode->location().line,
            verticesNode->location().column,
            "vertices"));
    }
    else if (!formsCuboid(vertices))
    {
        context.issues.push_back(makeImportIssue(
            importcodes::kMeshNotACuboid,
            file,
            verticesNode->location().line,
            verticesNode->location().column,
            "vertices"));
    }
    else
    {
        projection.domain =
            core::DomainBox(core::Vec3d(vertices.min[0], vertices.min[1], vertices.min[2]),
                            core::Vec3d(vertices.max[0], vertices.max[1], vertices.max[2]));
        projection.hasDomain = true;
    }

    // blocks: exactly one hex block
    const FoamNode* blocksNode = dictionary.find("blocks");
    std::size_t hexBlocks = 0;
    std::array<int, 3> cells{10, 10, 10};
    std::array<double, 3> grading{1.0, 1.0, 1.0};
    if (blocksNode != nullptr && blocksNode->isList())
    {
        // The parser keeps every atom inside a list as its own child, so one block
        // description (`hex (verts) (cells) simpleGrading (g)`) spans several
        // siblings; group them by their leading block-kind word.
        std::vector<std::vector<const FoamNode*>> groups;
        for (const auto& child : blocksNode->children())
        {
            if (child == nullptr)
            {
                continue;
            }
            if (groups.empty() || startsBlockDescription(*child))
            {
                groups.emplace_back();
            }
            groups.back().push_back(child.get());
        }

        for (const std::vector<const FoamNode*>& group : groups)
        {
            if (group.empty())
            {
                continue;
            }
            const FoamNode& kindNode = *group.front();
            const std::string kind = kindNode.rawText();
            if (kind != "hex")
            {
                context.issues.push_back(makeImportIssue(
                    importcodes::kMeshNonHexBlock,
                    file,
                    kindNode.location().line,
                    kindNode.location().column,
                    "blocks",
                    FoamValue(kindNode).text()));
                projection.blocking = true;
                continue;
            }
            ++hexBlocks;
            if (hexBlocks > 1)
            {
                continue;
            }
            if (group.size() >= 3)
            {
                std::vector<int> blockCells;
                if (blockMeshIntList(group[2], blockCells) && blockCells.size() == 3)
                {
                    cells = {blockCells[0], blockCells[1], blockCells[2]};
                }
            }
            for (std::size_t index = 3; index + 1 < group.size(); ++index)
            {
                if (!group[index]->isLeaf())
                {
                    continue;
                }
                const std::string keyword = group[index]->rawText();
                if (keyword == "simpleGrading" && group[index + 1]->isList())
                {
                    std::vector<double> values;
                    if (blockMeshRealList(group[index + 1], values))
                    {
                        if (values.size() == 3)
                        {
                            grading = {values[0], values[1], values[2]};
                        }
                        else
                        {
                            context.issues.push_back(makeImportIssue(
                                importcodes::kMeshGradingShape,
                                file,
                                group[index + 1]->location().line,
                                group[index + 1]->location().column,
                                "blocks",
                                FoamValue(*group[index + 1]).text()));
                        }
                    }
                    break;
                }
                if (keyword == "edgeGrading")
                {
                    context.issues.push_back(makeImportIssue(
                        importcodes::kMeshGradingShape,
                        file,
                        group[index]->location().line,
                        group[index]->location().column,
                        "blocks",
                        keyword));
                    break;
                }
            }
        }
    }
    if (hexBlocks > 1)
    {
        context.issues.push_back(makeImportIssue(
            importcodes::kMeshMultipleBlocks,
            file,
            blocksNode->location().line,
            blocksNode->location().column,
            "blocks",
            hexBlocks > 0 ? std::to_string(hexBlocks) + " hex blocks" : std::string{}));
        projection.blocking = true;
    }
    if (cells[0] <= 0 || cells[1] <= 0 || cells[2] <= 0)
    {
        context.issues.push_back(makeImportIssue(
            importcodes::kMeshInvalidCells, file, blocksNode ? blocksNode->location().line : 0, 0, "blocks"));
        projection.blocking = true;
    }
    else
    {
        projection.mesh =
            core::MeshSettings(core::MeshSettings::CellCounts{cells[0], cells[1], cells[2]}, grading, scale);
        projection.hasMesh = true;
    }

    // Domain mode suggestion: a single cell layer in z, or a box whose
    // thickness is negligible, is meant to be two-dimensional. The suggestion is
    // reported only; the imported mode is not changed (spec 6.2).
    if (projection.hasDomain)
    {
        const bool singleLayer = cells[2] == 1;
        const bool thin = projection.domain.suggestMode() == core::DomainMode::TwoD;
        if (singleLayer || thin)
        {
            context.issues.push_back(makeImportIssue(importcodes::kDomainSuggestedTwoD,
                                                     file,
                                                     verticesNode != nullptr
                                                         ? verticesNode->location().line
                                                         : 0,
                                                     0,
                                                     "vertices"));
        }
    }
    context.issues.push_back(makeImportIssue(importcodes::kDomainAssumedMetre, file, 0, 0, "domain"));

    // boundary: patch names, types and face ownership
    projectBlockMeshPatches(dictionary, file, context, projection);

    // edges are supported for reading only; the box stays the vertex bounding box
    if (const FoamNode* edges = dictionary.find("edges"); edges != nullptr && edges->isList() && edges->childCount() > 0)
    {
        context.issues.push_back(makeImportIssue(
            importcodes::kMeshEdgesPresent, file, edges->location().line, edges->location().column, "edges"));
    }

    return projection;
}

} // namespace ofgui::io