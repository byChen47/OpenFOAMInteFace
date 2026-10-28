#include "ui/DefaultCaseTemplate.h"

#include "core/BoundaryCondition.h"
#include "core/DomainBox.h"
#include "core/FieldDefinition.h"
#include "core/FieldValue.h"
#include "core/MeshSettings.h"
#include "core/ObjectId.h"
#include "core/SolverSettings.h"
#include "core/Vec3d.h"

#include <array>
#include <set>
#include <string>
#include <utility>

namespace ofgui {
namespace {

constexpr std::array<int, 7> kPressureDimensions{0, 2, -2, 0, 0, 0, 0};
constexpr std::array<int, 7> kVelocityDimensions{0, 1, -1, 0, 0, 0, 0};

core::Vec3d vertex(core::Real x, core::Real y, core::Real z)
{
    core::Vec3d point;
    point.x = x;
    point.y = y;
    point.z = z;
    return point;
}

core::ObjectId addPatch(
    core::CaseProject& project,
    std::string name,
    core::PatchType type,
    std::set<core::FaceId> faces)
{
    const core::ObjectId id = project.addPatch(std::move(name), type);
    core::BoundaryPatch* patch = project.findPatch(id);
    if (patch != nullptr)
    {
        patch->setFaces(std::move(faces));
    }
    return id;
}

void setBoundaryCondition(
    core::FieldDefinition& field,
    const core::ObjectId& patchId,
    core::BoundaryCondition condition)
{
    field.setBoundaryCondition(patchId, std::move(condition));
}

} // namespace

DefaultCaseTemplate makeDefaultCavityTemplate(std::string_view name)
{
    DefaultCaseTemplate result;
    core::CaseProject& project = result.project;
    project = core::CaseProject(std::string{name});

    // Geometry of the v2206 cavity tutorial: a thin 2D slab.
    core::DomainBox domain{vertex(0.0, 0.0, 0.0), vertex(0.1, 0.1, 0.01)};
    domain.setMode(core::DomainMode::TwoD);
    project.setDomain(domain);
    project.setMesh(core::MeshSettings({20, 20, 1}));

    core::SolverSettings& solver = project.solver();
    solver.setApplication("icoFoam");
    solver.setStartTime(0.0);
    solver.setEndTime(0.5);
    solver.setDeltaT(0.005);
    solver.setWriteInterval(20);

    const core::ObjectId fixedWalls = addPatch(
        project,
        "fixedWalls",
        core::PatchType::Wall,
        {core::FaceId::XMin, core::FaceId::XMax, core::FaceId::YMin});
    const core::ObjectId movingWall =
        addPatch(project, "movingWall", core::PatchType::Wall, {core::FaceId::YMax});
    const core::ObjectId frontAndBack = addPatch(
        project,
        "frontAndBack",
        core::PatchType::Empty,
        {core::FaceId::ZMin, core::FaceId::ZMax});

    const core::ObjectId pressureId = project.addField(
        "p", core::FieldClass::VolScalarField, core::FieldDefinition::Dimensions{kPressureDimensions});
    const core::ObjectId velocityId = project.addField(
        "U",
        core::FieldClass::VolVectorField,
        core::FieldDefinition::Dimensions{kVelocityDimensions});

    core::FieldDefinition& pressure = *project.findField(pressureId);
    pressure.setInternalField(core::FieldValue::uniformScalar(0.0));
    setBoundaryCondition(pressure, fixedWalls, core::BoundaryCondition{"zeroGradient"});
    setBoundaryCondition(pressure, movingWall, core::BoundaryCondition{"zeroGradient"});
    setBoundaryCondition(pressure, frontAndBack, core::BoundaryCondition{"empty"});

    core::FieldDefinition& velocity = *project.findField(velocityId);
    velocity.setInternalField(core::FieldValue::uniformVector(vertex(0.0, 0.0, 0.0)));
    setBoundaryCondition(velocity, fixedWalls, core::BoundaryCondition{"noSlip"});
    core::BoundaryCondition movingWallCondition{"fixedValue"};
    movingWallCondition.setParameter("value", "uniform (1 0 0)");
    setBoundaryCondition(velocity, movingWall, std::move(movingWallCondition));
    setBoundaryCondition(velocity, frontAndBack, core::BoundaryCondition{"empty"});

    result.transport.transportModel = "Newtonian";
    result.transport.kinematicViscosity = 0.01;
    return result;
}

} // namespace ofgui
