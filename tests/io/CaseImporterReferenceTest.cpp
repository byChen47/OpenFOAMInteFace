#include "io/CaseImportResult.h"
#include "io/CaseImporter.h"
#include "io/ImportCodes.h"

#include "core/BoundaryCondition.h"
#include "core/BoundaryPatch.h"
#include "core/CaseProject.h"
#include "core/DomainBox.h"
#include "core/FieldDefinition.h"
#include "core/FieldValue.h"
#include "core/MeshSettings.h"
#include "core/SolverSettings.h"

#include <QtTest>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <string>

namespace ofgui::io {
namespace {

bool referenceTreeAvailable()
{
    std::error_code ec;
    const std::filesystem::path root(OFGUI_FOAM_REFERENCE_DIR);
    return std::filesystem::is_directory(root / "tutorials", ec) && !ec;
}

std::filesystem::path tutorialCase(const char* relative)
{
    return std::filesystem::path(OFGUI_FOAM_REFERENCE_DIR) / relative;
}

bool near(double left, double right, double tolerance = 1e-12)
{
    return std::abs(left - right) <= tolerance;
}

std::map<std::string, std::uintmax_t> snapshot(const std::filesystem::path& root)
{
    std::map<std::string, std::uintmax_t> files;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root))
    {
        if (entry.is_regular_file())
        {
            files[std::filesystem::relative(entry.path(), root).generic_string()] = entry.file_size();
        }
    }
    return files;
}

} // namespace

class CaseImporterReferenceTest : public QObject
{
    Q_OBJECT

private slots:
    void cavityTutorialMatchesTheSpec();
    void cavityTutorialIsNotModified();
    void multiBlockTutorialIsBlocking();
};

void CaseImporterReferenceTest::cavityTutorialMatchesTheSpec()
{
    if (!referenceTreeAvailable())
    {
        QSKIP("OpenFOAM-v2206 reference tree is not available");
    }

    const std::filesystem::path caseRoot = tutorialCase("tutorials/incompressible/icoFoam/cavity/cavity");
    const CaseImportResult result = CaseImporter().import(caseRoot);

    QVERIFY2(result.ok, qPrintable(QString::fromStdString(result.report.issues.empty()
                                                              ? std::string{}
                                                              : result.report.issues.front().format())));
    QCOMPARE(QString::fromStdString(result.report.startDirectory), QStringLiteral("0"));

    // Domain: 0.1 x 0.1 x 0.01 (scale 0.1 applied to the unit block).
    const core::DomainBox& domain = result.project.domain();
    QVERIFY(near(domain.min().x, 0.0));
    QVERIFY(near(domain.min().y, 0.0));
    QVERIFY(near(domain.min().z, 0.0));
    QVERIFY(near(domain.max().x, 0.1));
    QVERIFY(near(domain.max().y, 0.1));
    QVERIFY(near(domain.max().z, 0.01));

    const core::MeshSettings& mesh = result.project.mesh();
    QCOMPARE(mesh.cells()[0], 20);
    QCOMPARE(mesh.cells()[1], 20);
    QCOMPARE(mesh.cells()[2], 1);
    QVERIFY(near(mesh.scale(), 0.1));

    // Patches.
    QCOMPARE(result.project.patches().size(), std::size_t{3});
    QVERIFY(result.project.findPatchByName("movingWall") != nullptr);
    QVERIFY(result.project.findPatchByName("fixedWalls") != nullptr);
    QVERIFY(result.project.findPatchByName("frontAndBack") != nullptr);

    // Fields and boundary conditions (spec section 11.1).
    const core::FieldDefinition* pressure = result.project.findFieldByName("p");
    const core::FieldDefinition* velocity = result.project.findFieldByName("U");
    QVERIFY(pressure != nullptr);
    QVERIFY(velocity != nullptr);
    QCOMPARE(pressure->dimensions(), (core::FieldDefinition::Dimensions{0, 2, -2, 0, 0, 0, 0}));
    QCOMPARE(velocity->dimensions(), (core::FieldDefinition::Dimensions{0, 1, -1, 0, 0, 0, 0}));
    QVERIFY(pressure->internalField().isUniformScalar());
    QVERIFY(near(pressure->internalField().scalar(), 0.0));
    QVERIFY(velocity->internalField().isUniformVector());

    const core::BoundaryPatch* movingWall = result.project.findPatchByName("movingWall");
    const core::BoundaryCondition* velocityMovingWall = velocity->findBoundaryCondition(movingWall->id());
    QVERIFY(velocityMovingWall != nullptr);
    QCOMPARE(QString::fromStdString(velocityMovingWall->type()), QStringLiteral("fixedValue"));
    QVERIFY(velocityMovingWall->hasParameter("value"));
    QCOMPARE(QString::fromStdString(*velocityMovingWall->parameter("value")), QStringLiteral("uniform (1 0 0)"));

    // Transport and solver.
    // The v2206 cavity transportProperties carries only nu; transportModel is empty.
    QCOMPARE(QString::fromStdString(result.transport.transportModel), QString());
    QVERIFY(result.report.hasCode(importcodes::kTransportModelDefaulted));
    QVERIFY(near(result.transport.kinematicViscosity, 0.01));
    QCOMPARE(QString::fromStdString(result.project.solver().application()), QStringLiteral("icoFoam"));
    QVERIFY(near(result.project.solver().endTime(), 0.5));
    QVERIFY(near(result.project.solver().deltaT(), 0.005));

    // No blocking error and source metadata recorded.
    QVERIFY(!result.report.blocking());
    QVERIFY(!result.project.source().isEmpty());
    QVERIFY(result.project.source().findFile("system/controlDict") != nullptr);
}

void CaseImporterReferenceTest::cavityTutorialIsNotModified()
{
    if (!referenceTreeAvailable())
    {
        QSKIP("OpenFOAM-v2206 reference tree is not available");
    }

    const std::filesystem::path caseRoot = tutorialCase("tutorials/incompressible/icoFoam/cavity/cavity");
    const std::map<std::string, std::uintmax_t> before = snapshot(caseRoot);

    const CaseImportResult result = CaseImporter().import(caseRoot);
    QVERIFY(result.ok);

    const std::map<std::string, std::uintmax_t> after = snapshot(caseRoot);
    QCOMPARE(after, before);
}

void CaseImporterReferenceTest::multiBlockTutorialIsBlocking()
{
    if (!referenceTreeAvailable())
    {
        QSKIP("OpenFOAM-v2206 reference tree is not available");
    }

    const std::filesystem::path caseRoot = tutorialCase("tutorials/incompressible/simpleFoam/pitzDaily");
    if (!std::filesystem::exists(caseRoot / "system" / "blockMeshDict"))
    {
        QSKIP("pitzDaily tutorial is not available");
    }

    const CaseImportResult result = CaseImporter().import(caseRoot);
    QVERIFY(!result.ok);
    QVERIFY(result.report.blocking());
    QVERIFY2(result.report.hasCode(importcodes::kMeshMultipleBlocks) || result.report.hasCode(importcodes::kMeshNonHexBlock),
             qPrintable(result.report.issues.empty() ? QStringLiteral("<no issues>")
                                                    : QString::fromStdString(result.report.issues.front().format())));
}

} // namespace ofgui::io

QTEST_APPLESS_MAIN(ofgui::io::CaseImporterReferenceTest)

#include "CaseImporterReferenceTest.moc"
