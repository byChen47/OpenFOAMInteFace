#include "io/writers/BlockMeshDictWriter.h"

#include "io/writers/BlockMeshFaces.h"
#include "io/writers/FoamLayout.h"
#include "io/writers/FoamNumberFormat.h"

#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace ofgui::io {
namespace {

constexpr int kTopLevel = 0;
constexpr int kBlockIndent = 1;
constexpr int kFaceIndent = 3;

/// OpenFOAM hexahedron vertex order: (min,min,min) first, z layer last.
struct GridCorner
{
    int ix;
    int iy;
    int iz;
};

constexpr std::array<GridCorner, 8> kHexCorners{{
    {0, 0, 0},
    {1, 0, 0},
    {1, 1, 0},
    {0, 1, 0},
    {0, 0, 1},
    {1, 0, 1},
    {1, 1, 1},
    {0, 1, 1},
}};

std::string faceQuad(const std::array<int, 4>& vertices)
{
    return "(" + formatFoamInt(vertices[0]) + " " + formatFoamInt(vertices[1]) + " " + formatFoamInt(vertices[2])
        + " " + formatFoamInt(vertices[3]) + ")";
}

} // namespace

std::string BlockMeshDictWriter::write(const core::CaseProject& project)
{
    const core::DomainBox& domain = project.domain();
    const core::MeshSettings& mesh = project.mesh();

    const core::Real scale = mesh.scale() > 0.0 ? mesh.scale() : 1.0;
    const core::Vec3d min = domain.min();
    const core::Vec3d max = domain.max();

    std::string text = foamFileHeader("dictionary", "blockMeshDict");
    text += foamEntryLine(kTopLevel, "scale", formatFoamReal(mesh.scale()), 8);
    text += "\n\n";

    text += "vertices\n(\n";
    for (const GridCorner& corner : kHexCorners)
    {
        const core::Vec3d point{
            (corner.ix == 0 ? min.x : max.x) / scale,
            (corner.iy == 0 ? min.y : max.y) / scale,
            (corner.iz == 0 ? min.z : max.z) / scale};
        text += foamIndent(kBlockIndent);
        text += formatFoamPoint(point);
        text += '\n';
    }
    text += ");\n\n";

    const core::MeshSettings::CellCounts& cells = mesh.cells();
    text += "blocks\n(\n";
    text += foamIndent(kBlockIndent);
    text += "hex (0 1 2 3 4 5 6 7) (" + formatFoamInt(cells[0]) + " " + formatFoamInt(cells[1]) + " "
        + formatFoamInt(cells[2]) + ") simpleGrading (";
    const core::MeshSettings::Grading& grading = mesh.grading();
    text += formatFoamReal(grading[0]) + " " + formatFoamReal(grading[1]) + " " + formatFoamReal(grading[2]);
    text += ")\n);\n\n";

    text += "edges\n(\n);\n\n";

    text += "boundary\n(\n";
    for (const core::BoundaryPatch& patch : project.patches())
    {
        text += foamIndent(kBlockIndent);
        text += patch.name();
        text += '\n';
        text += foamIndent(kBlockIndent);
        text += "{\n";
        text += foamIndent(kBlockIndent + 1);
        text += "type ";
        text += core::patchTypeName(patch.type());
        text += ";\n";
        text += foamIndent(kBlockIndent + 1);
        text += "faces\n";
        text += foamIndent(kBlockIndent + 1);
        text += "(\n";
        for (const core::FaceId face : patch.faces())
        {
            text += foamIndent(kFaceIndent);
            text += faceQuad(hexFaceQuad(face));
            text += '\n';
        }
        text += foamIndent(kBlockIndent + 1);
        text += ");\n";
        text += foamIndent(kBlockIndent);
        text += "}\n";
    }
    text += ");\n\n\n";

    text += foamTrailer();
    return text;
}

} // namespace ofgui::io
