#pragma once

#include "core/BoundaryCondition.h"
#include "core/BoundaryPatch.h"
#include "core/CaseProject.h"
#include "core/DomainBox.h"
#include "core/FieldDefinition.h"
#include "core/FieldValue.h"
#include "core/MeshSettings.h"
#include "core/ObjectId.h"
#include "core/SolverSettings.h"
#include "core/Vec3d.h"

#include <QString>

#include <array>
#include <set>
#include <string>
#include <vector>

/// Shared sample model and comparison helpers for the `.ofgproj` tests.
///
/// The sample deliberately uses every part of the schema version 1 model: both
/// field classes, all three `FieldValue` kinds (plus an unset interior value),
/// custom patch colours and a hidden patch, and boundary conditions with and
/// without parameters.
namespace ofgui::io::projecttest {

inline bool realEquals(core::Real lhs, core::Real rhs)
{
    return core::Vec3d(lhs, 0.0, 0.0).isApprox(core::Vec3d(rhs, 0.0, 0.0));
}

/// Builds the reference project used by the round-trip tests.
inline core::CaseProject makeSampleProject()
{
    core::CaseProject project;
    project.setName("cavityPreprocessor");
    project.setCaseDirectory("cases/cavityPreprocessor");

    project.domain().setMin(core::Vec3d(0.0, 0.0, 0.0));
    project.domain().setMax(core::Vec3d(0.1, 0.1, 0.01));
    project.domain().setMode(core::DomainMode::ThreeD);
    project.domain().setUnit("m");

    project.mesh().setCells(core::MeshSettings::CellCounts{20, 20, 1});
    project.mesh().setGrading(core::MeshSettings::Grading{1.0, 1.0, 1.0});
    project.mesh().setScale(0.1);

    core::SolverSettings solver;
    solver.setApplication("icoFoam");
    solver.setStartTime(0.0);
    solver.setEndTime(0.5);
    solver.setDeltaT(0.005);
    solver.setWriteInterval(20);
    project.setSolver(solver);

    const core::ObjectId movingWall = project.addPatch("movingWall", core::PatchType::Wall);
    const core::ObjectId fixedWalls = project.addPatch("fixedWalls", core::PatchType::Wall);
    const core::ObjectId frontAndBack = project.addPatch("frontAndBack", core::PatchType::Empty);

    project.findPatch(movingWall)->setFaces({core::FaceId::YMax});
    project.findPatch(fixedWalls)->setFaces(
        {core::FaceId::XMin, core::FaceId::XMax, core::FaceId::YMin});
    project.findPatch(frontAndBack)->setFaces({core::FaceId::ZMin, core::FaceId::ZMax});
    project.findPatch(movingWall)->setColor({0.2, 0.4, 0.9});
    project.findPatch(fixedWalls)->setColor({0.9, 0.3, 0.1});
    project.findPatch(fixedWalls)->setVisible(false);

    const core::FieldDefinition::Dimensions pressureDimensions{0, 2, -2, 0, 0, 0, 0};
    const core::ObjectId pressure = project.addField(
        "p", core::FieldClass::VolScalarField, pressureDimensions);
    core::FieldDefinition* p = project.findField(pressure);
    p->setInternalField(core::FieldValue::uniformScalar(0.0));
    p->setBoundaryCondition(movingWall, core::BoundaryCondition("zeroGradient"));
    p->setBoundaryCondition(fixedWalls, core::BoundaryCondition("zeroGradient"));
    p->setBoundaryCondition(frontAndBack, core::BoundaryCondition("empty"));

    const core::FieldDefinition::Dimensions velocityDimensions{0, 1, -1, 0, 0, 0, 0};
    const core::ObjectId velocity = project.addField(
        "U", core::FieldClass::VolVectorField, velocityDimensions);
    core::FieldDefinition* u = project.findField(velocity);
    u->setInternalField(core::FieldValue::uniformVector(core::Vec3d(1.0, 0.0, 0.0)));

    core::BoundaryCondition movingWallCondition("fixedValue");
    movingWallCondition.setParameter("value", "uniform (1 0 0)");
    u->setBoundaryCondition(movingWall, movingWallCondition);
    u->setBoundaryCondition(fixedWalls, core::BoundaryCondition("noSlip"));
    core::BoundaryCondition emptyCondition("empty");
    emptyCondition.setParameter("value", "uniform 0");
    u->setBoundaryCondition(frontAndBack, emptyCondition);

    const core::FieldDefinition::Dimensions temperatureDimensions{0, 0, 0, 1, 0, 0, 0};
    const core::ObjectId temperature = project.addField(
        "T", core::FieldClass::VolScalarField, temperatureDimensions);
    core::FieldDefinition* t = project.findField(temperature);
    // No interior value yet: the writer must emit `"kind": "unset"` and the
    // reader must leave the default `FieldValue` untouched.
    t->setBoundaryCondition(movingWall, core::BoundaryCondition("zeroGradient"));

    return project;
}

/// Compares a loaded project with `makeSampleProject()` field by field.
///
/// @return empty string when equal, otherwise the first difference found. Using
///         a string instead of `QCOMPARE` keeps the failure readable even though
///         arrays of nested structs are involved.
inline QString describeDifference(const core::CaseProject& loaded, const core::CaseProject& expected)
{
    const auto fail = [](const QString& text) { return text; };

    if (loaded.name() != expected.name())
    {
        return fail(QStringLiteral("name: '%1' != '%2'")
                        .arg(QString::fromStdString(loaded.name()),
                             QString::fromStdString(expected.name())));
    }
    if (loaded.caseDirectory() != expected.caseDirectory())
    {
        return fail(QStringLiteral("caseDirectory: '%1' != '%2'")
                        .arg(QString::fromStdString(loaded.caseDirectory()),
                             QString::fromStdString(expected.caseDirectory())));
    }
    if (!loaded.domain().min().isApprox(expected.domain().min())
        || !loaded.domain().max().isApprox(expected.domain().max()))
    {
        return fail(QStringLiteral("domain extent differs"));
    }
    if (loaded.domain().mode() != expected.domain().mode())
    {
        return fail(QStringLiteral("domain mode differs"));
    }
    if (loaded.domain().unit() != expected.domain().unit())
    {
        return fail(QStringLiteral("domain unit: '%1' != '%2'")
                        .arg(QString::fromStdString(loaded.domain().unit()),
                             QString::fromStdString(expected.domain().unit())));
    }
    if (loaded.mesh().cells() != expected.mesh().cells())
    {
        return fail(QStringLiteral("mesh cells differ"));
    }
    for (std::size_t axis = 0; axis < 3; ++axis)
    {
        if (!realEquals(loaded.mesh().grading()[axis], expected.mesh().grading()[axis]))
        {
            return fail(QStringLiteral("mesh grading differs"));
        }
    }
    if (!realEquals(loaded.mesh().scale(), expected.mesh().scale()))
    {
        return fail(QStringLiteral("mesh scale differs"));
    }
    if (loaded.solver().application() != expected.solver().application())
    {
        return fail(QStringLiteral("solver application differs"));
    }
    if (!realEquals(loaded.solver().startTime(), expected.solver().startTime())
        || !realEquals(loaded.solver().endTime(), expected.solver().endTime())
        || !realEquals(loaded.solver().deltaT(), expected.solver().deltaT())
        || loaded.solver().writeInterval() != expected.solver().writeInterval())
    {
        return fail(QStringLiteral("solver settings differ"));
    }

    if (loaded.patches().size() != expected.patches().size())
    {
        return fail(QStringLiteral("patch count: %1 != %2")
                        .arg(loaded.patches().size())
                        .arg(expected.patches().size()));
    }
    for (std::size_t index = 0; index < expected.patches().size(); ++index)
    {
        const core::BoundaryPatch& actual = loaded.patches()[index];
        const core::BoundaryPatch& wanted = expected.patches()[index];
        if (actual.id() != wanted.id() || actual.name() != wanted.name()
            || actual.type() != wanted.type() || actual.faces() != wanted.faces()
            || actual.isVisible() != wanted.isVisible())
        {
            return fail(QStringLiteral("patch %1 ('%2') differs")
                            .arg(index)
                            .arg(QString::fromStdString(wanted.name())));
        }
        for (std::size_t channel = 0; channel < 3; ++channel)
        {
            if (!realEquals(actual.color()[channel], wanted.color()[channel]))
            {
                return fail(QStringLiteral("patch %1 colour differs")
                                .arg(QString::fromStdString(wanted.name())));
            }
        }
    }

    if (loaded.fields().size() != expected.fields().size())
    {
        return fail(QStringLiteral("field count: %1 != %2")
                        .arg(loaded.fields().size())
                        .arg(expected.fields().size()));
    }
    for (std::size_t index = 0; index < expected.fields().size(); ++index)
    {
        const core::FieldDefinition& actual = loaded.fields()[index];
        const core::FieldDefinition& wanted = expected.fields()[index];
        if (actual.id() != wanted.id() || actual.name() != wanted.name()
            || actual.fieldClass() != wanted.fieldClass()
            || actual.dimensions() != wanted.dimensions())
        {
            return fail(QStringLiteral("field %1 ('%2') differs")
                            .arg(index)
                            .arg(QString::fromStdString(wanted.name())));
        }
        if (actual.internalField().isValid() != wanted.internalField().isValid())
        {
            return fail(QStringLiteral("field '%1' internalField validity differs")
                            .arg(QString::fromStdString(wanted.name())));
        }
        if (wanted.internalField().isValid()
            && actual.internalField().kind() != wanted.internalField().kind())
        {
            return fail(QStringLiteral("field '%1' internalField kind differs")
                            .arg(QString::fromStdString(wanted.name())));
        }
        if (wanted.internalField().isUniformScalar()
            && !realEquals(actual.internalField().scalar(), wanted.internalField().scalar()))
        {
            return fail(QStringLiteral("field '%1' scalar differs")
                            .arg(QString::fromStdString(wanted.name())));
        }
        if (wanted.internalField().isUniformVector()
            && !actual.internalField().vector().isApprox(wanted.internalField().vector()))
        {
            return fail(QStringLiteral("field '%1' vector differs")
                            .arg(QString::fromStdString(wanted.name())));
        }
        if (wanted.internalField().isNonUniformRaw()
            && actual.internalField().rawText() != wanted.internalField().rawText())
        {
            return fail(QStringLiteral("field '%1' raw text differs")
                            .arg(QString::fromStdString(wanted.name())));
        }

        const auto& wantedConditions = wanted.boundaryConditions();
        const auto& actualConditions = actual.boundaryConditions();
        if (actualConditions.size() != wantedConditions.size())
        {
            return fail(QStringLiteral("field '%1' boundary condition count differs")
                            .arg(QString::fromStdString(wanted.name())));
        }
        for (const auto& [patchId, wantedCondition] : wantedConditions)
        {
            const auto found = actualConditions.find(patchId);
            if (found == actualConditions.end())
            {
                return fail(QStringLiteral("field '%1' misses a boundary condition")
                                .arg(QString::fromStdString(wanted.name())));
            }
            if (found->second.type() != wantedCondition.type()
                || found->second.parameters() != wantedCondition.parameters())
            {
                return fail(QStringLiteral("field '%1' boundary condition differs")
                                .arg(QString::fromStdString(wanted.name())));
            }
        }
    }

    return {};
}

} // namespace ofgui::io::projecttest
