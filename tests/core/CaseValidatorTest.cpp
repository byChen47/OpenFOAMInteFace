#include "core/CaseProject.h"
#include "core/CaseValidator.h"

#include <QtTest>

#include <algorithm>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace ofgui::core {
namespace {

int countCode(const ValidationResult& result, std::string_view code)
{
    return static_cast<int>(std::count_if(result.issues().begin(), result.issues().end(),
        [code](const ValidationIssue& issue) { return issue.code() == code; }));
}

bool hasCode(const ValidationResult& result, std::string_view code)
{
    return countCode(result, code) > 0;
}

CaseProject makeValidProject()
{
    CaseProject project("cavity");
    project.setDomain(DomainBox(Vec3d{0.0, 0.0, 0.0}, Vec3d{0.1, 0.1, 0.1}));
    project.setMesh(MeshSettings(MeshSettings::CellCounts{20, 20, 20}));

    const ObjectId walls = project.addPatch("walls", PatchType::Wall);
    project.findPatch(walls)->setFaces({FaceId::XMin, FaceId::YMin, FaceId::ZMin, FaceId::ZMax});
    const ObjectId inlet = project.addPatch("inlet");
    project.findPatch(inlet)->setFaces({FaceId::XMax});
    const ObjectId outlet = project.addPatch("outlet");
    project.findPatch(outlet)->setFaces({FaceId::YMax});

    const ObjectId p = project.addField("p", FieldClass::VolScalarField, {0, 2, -2, 0, 0, 0, 0});
    project.findField(p)->setInternalField(FieldValue::uniformScalar(0.0));
    project.findField(p)->setBoundaryCondition(walls, BoundaryCondition("zeroGradient"));
    project.findField(p)->setBoundaryCondition(inlet, BoundaryCondition("zeroGradient"));
    project.findField(p)->setBoundaryCondition(outlet, BoundaryCondition("fixedValue"));

    const ObjectId u = project.addField("U", FieldClass::VolVectorField, {0, 1, -1, 0, 0, 0, 0});
    project.findField(u)->setInternalField(FieldValue::uniformVector(Vec3d{1.0, 0.0, 0.0}));
    project.findField(u)->setBoundaryCondition(walls, BoundaryCondition("noSlip"));
    project.findField(u)->setBoundaryCondition(inlet, BoundaryCondition("fixedValue"));
    project.findField(u)->setBoundaryCondition(outlet, BoundaryCondition("zeroGradient"));

    SolverSettings solver;
    solver.setApplication("icoFoam");
    solver.setStartTime(0.0);
    solver.setEndTime(0.5);
    solver.setDeltaT(0.005);
    solver.setWriteInterval(20);
    project.setSolver(solver);

    return project;
}

CaseProject makeTwoDProject(bool frontAndBackEmpty)
{
    CaseProject project("cavity2d");
    DomainBox domain(Vec3d{0.0, 0.0, 0.0}, Vec3d{1.0, 1.0, 0.01});
    domain.setMode(DomainMode::TwoD);
    project.setDomain(domain);
    project.setMesh(MeshSettings(MeshSettings::CellCounts{20, 20, 1}));

    const ObjectId frontAndBack =
        project.addPatch("frontAndBack", frontAndBackEmpty ? PatchType::Empty : PatchType::Patch);
    project.findPatch(frontAndBack)->setFaces({FaceId::ZMin, FaceId::ZMax});
    const ObjectId walls = project.addPatch("walls", PatchType::Wall);
    project.findPatch(walls)->setFaces({FaceId::XMin, FaceId::XMax, FaceId::YMin, FaceId::YMax});

    return project;
}

class CaseValidatorTest final : public QObject
{
    Q_OBJECT

private slots:
    void validProjectHasNoIssues();
    void reportsInvertedDomainAxis();
    void reportsNonFiniteDomain();
    void reportsNonPositiveCells();
    void rejectsTwoDWithSeveralThicknessCells();
    void rejectsTwoDWhenFrontAndBackAreNotEmpty();
    void reportsDuplicatePatchNames();
    void reportsInvalidPatchName();
    void reportsUnassignedFaces();
    void reportsMultiplyAssignedFaces();
    void reportsMissingBoundaryConditions();
    void reportsSolverProblems();
    void reportsMissingApplicationAsWarning();
    void emptyProjectReportsRootProblems();
};

void CaseValidatorTest::validProjectHasNoIssues()
{
    const ValidationResult result = CaseValidator{}.validate(makeValidProject());
    QVERIFY(result.empty());
    QVERIFY(!result.hasBlockingErrors());
    QVERIFY(!result.hasErrors());
}

void CaseValidatorTest::reportsInvertedDomainAxis()
{
    CaseProject project = makeValidProject();
    project.domain().setMax(Vec3d{0.1, 0.0, 0.1});

    const ValidationResult result = CaseValidator{}.validate(project);
    QVERIFY(result.hasBlockingErrors());
    QCOMPARE(countCode(result, "CASE-DOM-001"), 1);

    const ValidationIssue* issue = result.firstBlockingError();
    QVERIFY(issue != nullptr);
    QCOMPARE(QString::fromStdString(issue->targetField()), QStringLiteral("domain.max"));
    QVERIFY(!issue->suggestedFix().empty());
}

void CaseValidatorTest::reportsNonFiniteDomain()
{
    CaseProject project = makeValidProject();
    project.domain().setMax(Vec3d{std::numeric_limits<double>::quiet_NaN(), 0.1, 0.1});

    const ValidationResult result = CaseValidator{}.validate(project);
    QCOMPARE(countCode(result, "CASE-DOM-002"), 1);
    QCOMPARE(countCode(result, "CASE-DOM-001"), 0);
}

void CaseValidatorTest::reportsNonPositiveCells()
{
    CaseProject project = makeValidProject();
    project.mesh().setCells({0, 20, 20});

    const ValidationResult result = CaseValidator{}.validate(project);
    QVERIFY(result.hasBlockingErrors());
    QCOMPARE(countCode(result, "CASE-MESH-001"), 1);
    QCOMPARE(QString::fromStdString(result.firstBlockingError()->targetField()),
        QStringLiteral("mesh.cells.x"));
}

void CaseValidatorTest::rejectsTwoDWithSeveralThicknessCells()
{
    CaseProject project = makeTwoDProject(true);
    project.mesh().setCells({20, 20, 3});

    const ValidationResult result = CaseValidator{}.validate(project);
    QVERIFY(hasCode(result, "CASE-MESH-004"));
    QCOMPARE(countCode(result, "CASE-MESH-006"), 0);
}

void CaseValidatorTest::rejectsTwoDWhenFrontAndBackAreNotEmpty()
{
    const ValidationResult result = CaseValidator{}.validate(makeTwoDProject(false));
    QVERIFY(result.hasBlockingErrors());
    QCOMPARE(countCode(result, "CASE-PATCH-008"), 2);

    const ValidationResult emptyFrontAndBack = CaseValidator{}.validate(makeTwoDProject(true));
    QCOMPARE(countCode(emptyFrontAndBack, "CASE-PATCH-008"), 0);
    QCOMPARE(countCode(emptyFrontAndBack, "CASE-MESH-004"), 0);
}

void CaseValidatorTest::reportsDuplicatePatchNames()
{
    CaseProject project = makeValidProject();
    const auto outlet = project.patchIdByName("outlet");
    QVERIFY(outlet.has_value());
    project.findPatch(*outlet)->setName("inlet");

    const ValidationResult result = CaseValidator{}.validate(project);
    QVERIFY(result.hasBlockingErrors());
    QCOMPARE(countCode(result, "CASE-PATCH-004"), 1);
    QVERIFY(result.firstBlockingError() != nullptr);
}

void CaseValidatorTest::reportsInvalidPatchName()
{
    CaseProject project = makeValidProject();
    const auto walls = project.patchIdByName("walls");
    QVERIFY(walls.has_value());
    project.findPatch(*walls)->setName("1 walls");

    const ValidationResult result = CaseValidator{}.validate(project);
    QCOMPARE(countCode(result, "CASE-PATCH-003"), 1);
    QCOMPARE(countCode(result, "CASE-PATCH-004"), 0);
}

void CaseValidatorTest::reportsUnassignedFaces()
{
    CaseProject project = makeValidProject();
    const auto outlet = project.patchIdByName("outlet");
    QVERIFY(outlet.has_value());
    project.findPatch(*outlet)->setFaces({});

    const ValidationResult result = CaseValidator{}.validate(project);
    QVERIFY(result.hasBlockingErrors());
    QCOMPARE(countCode(result, "CASE-PATCH-005"), 1);
    QCOMPARE(countCode(result, "CASE-PATCH-007"), 1);
}

void CaseValidatorTest::reportsMultiplyAssignedFaces()
{
    CaseProject project = makeValidProject();
    const auto walls = project.patchIdByName("walls");
    QVERIFY(walls.has_value());
    project.findPatch(*walls)->addFace(FaceId::YMax);

    const ValidationResult result = CaseValidator{}.validate(project);
    QVERIFY(result.hasBlockingErrors());
    QCOMPARE(countCode(result, "CASE-PATCH-006"), 1);
    QCOMPARE(countCode(result, "CASE-PATCH-005"), 0);
}

void CaseValidatorTest::reportsMissingBoundaryConditions()
{
    CaseProject project = makeValidProject();
    const auto inlet = project.patchIdByName("inlet");
    const FieldDefinition* pressure = project.findFieldByName("p");
    QVERIFY(inlet.has_value());
    QVERIFY(pressure != nullptr);
    project.findField(pressure->id())->removeBoundaryCondition(*inlet);

    const ValidationResult result = CaseValidator{}.validate(project);
    QVERIFY(result.hasErrors());
    QCOMPARE(countCode(result, "CASE-FIELD-003"), 1);

    const auto found = std::find_if(result.issues().begin(), result.issues().end(),
        [](const ValidationIssue& issue) { return issue.code() == "CASE-FIELD-003"; });
    QVERIFY(found != result.issues().end());
    QVERIFY(found->message().find("inlet") != std::string::npos);
}

void CaseValidatorTest::reportsSolverProblems()
{
    CaseProject project = makeValidProject();
    project.solver().setEndTime(0.0);
    project.solver().setDeltaT(0.0);
    project.solver().setWriteInterval(0);

    const ValidationResult result = CaseValidator{}.validate(project);
    QCOMPARE(countCode(result, "CASE-SOLV-001"), 1);
    QCOMPARE(countCode(result, "CASE-SOLV-002"), 1);
    QCOMPARE(countCode(result, "CASE-SOLV-003"), 1);
    QCOMPARE(countCode(result, "CASE-SOLV-004"), 0);
}

void CaseValidatorTest::reportsMissingApplicationAsWarning()
{
    CaseProject project = makeValidProject();
    project.solver().setApplication("");

    const ValidationResult result = CaseValidator{}.validate(project);
    QCOMPARE(countCode(result, "CASE-SOLV-004"), 1);
    QVERIFY(!result.hasBlockingErrors());
    QVERIFY(!result.hasErrors());
}

void CaseValidatorTest::emptyProjectReportsRootProblems()
{
    const CaseProject project;
    const ValidationResult result = CaseValidator{}.validate(project);

    QVERIFY(result.hasBlockingErrors());
    QCOMPARE(countCode(result, "CASE-DOM-001"), 3);
    QCOMPARE(countCode(result, "CASE-PATCH-001"), 1);
    QCOMPARE(countCode(result, "CASE-SOLV-004"), 1);
}

} // namespace
} // namespace ofgui::core

QTEST_APPLESS_MAIN(ofgui::core::CaseValidatorTest)

#include "CaseValidatorTest.moc"
