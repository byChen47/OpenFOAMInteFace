#include "core/CaseProject.h"
#include "core/ProjectSchemaVersion.h"

#include <QtTest>

#include <string>
#include <vector>

namespace ofgui::core {
namespace {

class CaseProjectTest final : public QObject
{
    Q_OBJECT

private slots:
    void startsEmpty();
    void storesMetadata();
    void createsAndFindsPatches();
    void keepsPatchInsertionOrder();
    void removesPatchAndItsConditions();
    void movesFaceBetweenPatches();
    void createsAndFindsFields();
};

void CaseProjectTest::startsEmpty()
{
    const CaseProject project;
    QVERIFY(project.isEmpty());
    QVERIFY(project.patches().empty());
    QVERIFY(project.fields().empty());
    QCOMPARE(project.schemaVersion(), kCurrentSchemaVersion);
}

void CaseProjectTest::storesMetadata()
{
    CaseProject project("cavity");
    project.setName("cavity-2d");
    project.setCaseDirectory("cases/cavity");

    QCOMPARE(QString::fromStdString(project.name()), QStringLiteral("cavity-2d"));
    QCOMPARE(QString::fromStdString(project.caseDirectory()), QStringLiteral("cases/cavity"));

    DomainBox domain(Vec3d{0.0, 0.0, 0.0}, Vec3d{1.0, 2.0, 3.0});
    domain.setMode(DomainMode::TwoD);
    project.setDomain(domain);
    QCOMPARE(project.domain().mode(), DomainMode::TwoD);
    QVERIFY(project.domain().max().isApprox(Vec3d{1.0, 2.0, 3.0}));

    project.mesh().setCells({30, 20, 1});
    QCOMPARE(project.mesh().cellCount(0), 30);

    project.solver().setApplication("simpleFoam");
    QCOMPARE(QString::fromStdString(project.solver().application()), QStringLiteral("simpleFoam"));
}

void CaseProjectTest::createsAndFindsPatches()
{
    CaseProject project("cavity");
    const ObjectId walls = project.addPatch("walls", PatchType::Wall);
    const ObjectId inlet = project.addPatch("inlet");

    QVERIFY(walls.isValid());
    QVERIFY(inlet.isValid());
    QVERIFY(walls != inlet);
    QCOMPARE(static_cast<int>(project.patches().size()), 2);
    QVERIFY(!project.isEmpty());

    const BoundaryPatch* byId = project.findPatch(inlet);
    QVERIFY(byId != nullptr);
    QCOMPARE(QString::fromStdString(byId->name()), QStringLiteral("inlet"));
    QCOMPARE(byId->type(), PatchType::Patch);

    const BoundaryPatch* byName = project.findPatchByName("walls");
    QVERIFY(byName != nullptr);
    QCOMPARE(byName->type(), PatchType::Wall);

    const auto wallsId = project.patchIdByName("walls");
    QVERIFY(wallsId.has_value());
    QVERIFY(*wallsId == walls);
    QVERIFY(!project.patchIdByName("outlet").has_value());
    QVERIFY(project.findPatch(ObjectId{0xDEAD}) == nullptr);
}

void CaseProjectTest::keepsPatchInsertionOrder()
{
    CaseProject project("cavity");
    const ObjectId first = project.addPatch("first");
    const ObjectId second = project.addPatch("second");
    const ObjectId third = project.addPatch("third");

    const std::vector<ObjectId> ids = project.patchIds();
    QCOMPARE(static_cast<int>(ids.size()), 3);
    QVERIFY(ids.at(0) == first);
    QVERIFY(ids.at(1) == second);
    QVERIFY(ids.at(2) == third);
}

void CaseProjectTest::removesPatchAndItsConditions()
{
    CaseProject project("cavity");
    const ObjectId walls = project.addPatch("walls");
    const ObjectId inlet = project.addPatch("inlet");
    const ObjectId fieldId = project.addField("p", FieldClass::VolScalarField);

    FieldDefinition* field = project.findField(fieldId);
    field->setBoundaryCondition(walls, BoundaryCondition("zeroGradient"));
    field->setBoundaryCondition(inlet, BoundaryCondition("fixedValue"));

    QVERIFY(project.removePatch(walls));
    QVERIFY(!project.removePatch(walls));
    QVERIFY(project.findPatch(walls) == nullptr);
    QVERIFY(!project.findField(fieldId)->hasBoundaryCondition(walls));
    QVERIFY(project.findField(fieldId)->hasBoundaryCondition(inlet));
}

void CaseProjectTest::movesFaceBetweenPatches()
{
    CaseProject project("cavity");
    const ObjectId first = project.addPatch("first");
    const ObjectId second = project.addPatch("second");

    project.findPatch(first)->addFace(FaceId::XMin);
    project.findPatch(second)->addFace(FaceId::YMin);

    QVERIFY(project.moveFaceToPatch(FaceId::XMin, second));
    QVERIFY(!project.findPatch(first)->hasFace(FaceId::XMin));
    QVERIFY(project.findPatch(second)->hasFace(FaceId::XMin));
    QVERIFY(project.findPatch(second)->hasFace(FaceId::YMin));
    QVERIFY(!project.moveFaceToPatch(FaceId::XMin, ObjectId{0xBEEF}));
}

void CaseProjectTest::createsAndFindsFields()
{
    CaseProject project("cavity");
    const ObjectId p = project.addField(
        "p", FieldClass::VolScalarField, FieldDefinition::Dimensions{0, 2, -2, 0, 0, 0, 0});
    const ObjectId u = project.addField("U", FieldClass::VolVectorField);

    QVERIFY(p.isValid());
    QVERIFY(u.isValid());
    QVERIFY(p != u);
    QCOMPARE(static_cast<int>(project.fields().size()), 2);

    const FieldDefinition* pressure = project.findFieldByName("p");
    QVERIFY(pressure != nullptr);
    QCOMPARE(pressure->fieldClass(), FieldClass::VolScalarField);
    QCOMPARE(pressure->dimensions().at(1), 2);

    QVERIFY(project.findField(u) != nullptr);
    QVERIFY(project.findField(ObjectId{0x777}) == nullptr);

    QVERIFY(project.removeField(p));
    QVERIFY(!project.removeField(p));
    QCOMPARE(static_cast<int>(project.fields().size()), 1);
}

} // namespace
} // namespace ofgui::core

QTEST_APPLESS_MAIN(ofgui::core::CaseProjectTest)

#include "CaseProjectTest.moc"
