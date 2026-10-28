#include "io/CaseImportResult.h"
#include "io/CaseImporter.h"
#include "io/ImportCodes.h"

#include "core/BoundaryCondition.h"
#include "core/BoundaryPatch.h"
#include "core/CaseProject.h"
#include "core/CaseSourceInfo.h"
#include "core/DomainBox.h"
#include "core/FaceId.h"
#include "core/FieldDefinition.h"
#include "core/FieldValue.h"
#include "core/MeshSettings.h"
#include "core/SolverSettings.h"

#include <QtTest>
#include <QTemporaryDir>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <string>

namespace ofgui::io {
namespace {

QString fixtureCaseRoot()
{
    return QString::fromUtf8(OFGUI_FOAM_FIXTURE_DIR) + QStringLiteral("/case");
}

bool near(double left, double right, double tolerance = 1e-12)
{
    return std::abs(left - right) <= tolerance;
}

const core::BoundaryPatch* patchNamed(const core::CaseProject& project, const std::string& name)
{
    return project.findPatchByName(name);
}

const core::BoundaryCondition* conditionOf(const core::CaseProject& project,
                                           const std::string& fieldName,
                                           const std::string& patchName)
{
    const core::FieldDefinition* field = project.findFieldByName(fieldName);
    const core::BoundaryPatch* patch = project.findPatchByName(patchName);
    if (field == nullptr || patch == nullptr)
    {
        return nullptr;
    }
    return field->findBoundaryCondition(patch->id());
}

std::map<std::string, std::string> fileSnapshot(const QString& root)
{
    std::map<std::string, std::string> snapshot;
    const std::filesystem::path base(root.toStdString());
    for (const auto& entry : std::filesystem::recursive_directory_iterator(base))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }
        const std::string relative = std::filesystem::relative(entry.path(), base).generic_string();
        std::ifstream stream(entry.path(), std::ios::binary);
        std::string content((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
        snapshot[relative] = std::to_string(entry.file_size()) + ":" + std::to_string(content.size()) + ":"
            + std::to_string(std::hash<std::string>{}(content));
    }
    return snapshot;
}

} // namespace

class CaseImporterTest : public QObject
{
    Q_OBJECT

private slots:
    void importsFixtureCase();
    void projectsDomainAndMesh();
    void projectsPatches();
    void projectsFieldsAndBoundaryConditions();
    void projectsSolverAndTransport();
    void recordsSourceHashes();
    void leavesTheCaseDirectoryUntouched();
    void readsFieldsFromZeroOrigWhenZeroIsMissing();
};

void CaseImporterTest::importsFixtureCase()
{
    const CaseImportResult result = CaseImporter().import(std::filesystem::path(fixtureCaseRoot().toStdString()));

    QVERIFY2(result.ok, qPrintable(result.report.issues.empty() ? QStringLiteral("no issues")
                                                               : QString::fromStdString(result.report.issues.front().format())));
    QVERIFY(!result.report.blocking());
    QCOMPARE(QString::fromStdString(result.report.startDirectory), QStringLiteral("0"));
    QVERIFY(!result.report.files.empty());
    QCOMPARE(result.report.findFile("0/p") != nullptr, true);
    QCOMPARE(result.report.findFile("system/controlDict") != nullptr, true);
    QVERIFY(!result.report.hasCode(importcodes::kCaseMissingControlDict));
    QVERIFY(!result.report.hasCode(importcodes::kCaseMissingStartDirectory));
}

void CaseImporterTest::projectsDomainAndMesh()
{
    const CaseImportResult result = CaseImporter().import(std::filesystem::path(fixtureCaseRoot().toStdString()));
    QVERIFY(result.ok);

    const core::DomainBox& domain = result.project.domain();
    QVERIFY(near(domain.min().x, 0.0));
    QVERIFY(near(domain.min().y, 0.0));
    QVERIFY(near(domain.min().z, 0.0));
    QVERIFY(near(domain.max().x, 0.1));
    QVERIFY(near(domain.max().y, 0.1));
    QVERIFY(near(domain.max().z, 0.01));
    QCOMPARE(QString::fromStdString(domain.unit()), QStringLiteral("m"));
    // One cell in z: the importer only suggests 2D (IMPORT-DOM-001); mode stays as modelled.
    QCOMPARE(domain.mode(), core::DomainMode::ThreeD);
    QVERIFY(result.report.hasCode("IMPORT-DOM-001"));
    QVERIFY(result.report.hasCode("IMPORT-DOM-002"));

    const core::MeshSettings& mesh = result.project.mesh();
    QCOMPARE(mesh.cells()[0], 20);
    QCOMPARE(mesh.cells()[1], 20);
    QCOMPARE(mesh.cells()[2], 1);
    QVERIFY(near(mesh.grading()[0], 1.0));
    QVERIFY(near(mesh.grading()[1], 1.0));
    QVERIFY(near(mesh.grading()[2], 1.0));
    QVERIFY(near(mesh.scale(), 0.1));
    QVERIFY(mesh.isValid());
}

void CaseImporterTest::projectsPatches()
{
    const CaseImportResult result = CaseImporter().import(std::filesystem::path(fixtureCaseRoot().toStdString()));
    QVERIFY(result.ok);

    QCOMPARE(result.project.patches().size(), std::size_t{3});
    QCOMPARE(patchNamed(result.project, "movingWall")->type(), core::PatchType::Wall);
    QCOMPARE(patchNamed(result.project, "fixedWalls")->type(), core::PatchType::Wall);
    QCOMPARE(patchNamed(result.project, "frontAndBack")->type(), core::PatchType::Empty);
    QCOMPARE(patchNamed(result.project, "movingWall")->faceCount(), std::size_t{1});
    QCOMPARE(patchNamed(result.project, "fixedWalls")->faceCount(), std::size_t{3});
    QCOMPARE(patchNamed(result.project, "frontAndBack")->faceCount(), std::size_t{2});
    // cavity: movingWall is the yMax face (quad 3 7 6 2 of blockMeshDict).
    QVERIFY(patchNamed(result.project, "movingWall")->hasFace(core::FaceId::YMax));
    QVERIFY(patchNamed(result.project, "fixedWalls")->hasFace(core::FaceId::XMin));
    QVERIFY(patchNamed(result.project, "frontAndBack")->hasFace(core::FaceId::ZMin));
    QVERIFY(patchNamed(result.project, "frontAndBack")->hasFace(core::FaceId::ZMax));
    QVERIFY(!result.report.hasCode(importcodes::kPatchRegexKey));
    QVERIFY(!result.report.hasCode(importcodes::kPatchMissingBoundaryCondition));
}

void CaseImporterTest::projectsFieldsAndBoundaryConditions()
{
    const CaseImportResult result = CaseImporter().import(std::filesystem::path(fixtureCaseRoot().toStdString()));
    QVERIFY(result.ok);

    const core::FieldDefinition* pressure = result.project.findFieldByName("p");
    const core::FieldDefinition* velocity = result.project.findFieldByName("U");
    QVERIFY(pressure != nullptr);
    QVERIFY(velocity != nullptr);
    QCOMPARE(pressure->fieldClass(), core::FieldClass::VolScalarField);
    QCOMPARE(velocity->fieldClass(), core::FieldClass::VolVectorField);

    const core::FieldDefinition::Dimensions expectedPressure{0, 2, -2, 0, 0, 0, 0};
    const core::FieldDefinition::Dimensions expectedVelocity{0, 1, -1, 0, 0, 0, 0};
    QCOMPARE(pressure->dimensions(), expectedPressure);
    QCOMPARE(velocity->dimensions(), expectedVelocity);

    QVERIFY(pressure->internalField().isUniformScalar());
    QVERIFY(near(pressure->internalField().scalar(), 0.0));
    QVERIFY(velocity->internalField().isUniformVector());
    QVERIFY(near(velocity->internalField().vector().x, 0.0));
    QVERIFY(near(velocity->internalField().vector().y, 0.0));
    QVERIFY(near(velocity->internalField().vector().z, 0.0));

    const core::BoundaryCondition* pressureWall = conditionOf(result.project, "p", "movingWall");
    QVERIFY(pressureWall != nullptr);
    QCOMPARE(QString::fromStdString(pressureWall->type()), QStringLiteral("zeroGradient"));

    const core::BoundaryCondition* velocityWall = conditionOf(result.project, "U", "movingWall");
    QVERIFY(velocityWall != nullptr);
    QCOMPARE(QString::fromStdString(velocityWall->type()), QStringLiteral("fixedValue"));
    QVERIFY(velocityWall->hasParameter("value"));
    QCOMPARE(QString::fromStdString(*velocityWall->parameter("value")), QStringLiteral("uniform (1 0 0)"));

    const core::BoundaryCondition* velocityBack = conditionOf(result.project, "U", "frontAndBack");
    QVERIFY(velocityBack != nullptr);
    QCOMPARE(QString::fromStdString(velocityBack->type()), QStringLiteral("empty"));

    // Every patch of p and U must be covered.
    QCOMPARE(pressure->boundaryConditions().size(), std::size_t{3});
    QCOMPARE(velocity->boundaryConditions().size(), std::size_t{3});
}

void CaseImporterTest::projectsSolverAndTransport()
{
    const CaseImportResult result = CaseImporter().import(std::filesystem::path(fixtureCaseRoot().toStdString()));
    QVERIFY(result.ok);

    const core::SolverSettings& solver = result.project.solver();
    QCOMPARE(QString::fromStdString(solver.application()), QStringLiteral("icoFoam"));
    QVERIFY(near(solver.startTime(), 0.0));
    QVERIFY(near(solver.endTime(), 0.5));
    QVERIFY(near(solver.deltaT(), 0.005));
    QCOMPARE(solver.writeInterval(), 20);

    // The v2206 cavity transportProperties has no transportModel key, only nu.
    QCOMPARE(QString::fromStdString(result.transport.transportModel), QString());
    QVERIFY(result.report.hasCode(importcodes::kTransportModelDefaulted));
    QVERIFY(result.transport.hasKinematicViscosity);
    QVERIFY(near(result.transport.kinematicViscosity, 0.01));

    // Read only dictionaries are flattened for the export mirror.
    QVERIFY(!result.readOnlySettings.empty());
    const auto hasPath = [&result](const std::string& path) {
        return std::any_of(result.readOnlySettings.begin(), result.readOnlySettings.end(),
                           [&path](const ImportedSetting& setting) { return setting.path == path; });
    };
    QVERIFY(hasPath("system/fvSolution.solvers.p.solver"));
    QVERIFY(hasPath("system/decomposeParDict.numberOfSubdomains"));
}

void CaseImporterTest::recordsSourceHashes()
{
    const CaseImportResult result = CaseImporter().import(std::filesystem::path(fixtureCaseRoot().toStdString()));
    QVERIFY(result.ok);

    const core::CaseSourceInfo& source = result.project.source();
    QVERIFY(!source.isEmpty());
    QCOMPARE(QString::fromStdString(source.startDirectory), QStringLiteral("0"));
    QCOMPARE(QString::fromStdString(source.sourceRelativePath), QStringLiteral("case"));
    QVERIFY(!source.importedAt.empty());
    QCOMPARE(source.importedAt.back(), QChar('Z'));
    QVERIFY(source.fileCount() >= 10);
    const core::SourceFileHash* controlDict = source.findFile("system/controlDict");
    QVERIFY(controlDict != nullptr);
    QCOMPARE(controlDict->sha256Hex().size(), std::size_t{64});
    const std::string controlDictHash = controlDict->sha256Hex();
    QCOMPARE(controlDictHash.size(), std::size_t{64});
    QVERIFY(std::all_of(controlDictHash.begin(), controlDictHash.end(),
                        [](char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'); }));
}

void CaseImporterTest::leavesTheCaseDirectoryUntouched()
{
    const QString root = fixtureCaseRoot();
    const std::map<std::string, std::string> before = fileSnapshot(root);

    const CaseImportResult result = CaseImporter().import(std::filesystem::path(root.toStdString()));
    QVERIFY(result.ok);

    const std::map<std::string, std::string> after = fileSnapshot(root);
    QCOMPARE(after.size(), before.size());
    QCOMPARE(after, before);
}

void CaseImporterTest::readsFieldsFromZeroOrigWhenZeroIsMissing()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const std::filesystem::path root(temporary.path().toStdString());
    const std::filesystem::path fixture(fixtureCaseRoot().toStdString());

    std::filesystem::copy(fixture / "system", root / "system",
                          std::filesystem::copy_options::recursive);
    std::filesystem::copy(fixture / "constant", root / "constant",
                          std::filesystem::copy_options::recursive);
    std::filesystem::create_directories(root / "0.orig");
    for (const char* name : {"p", "U"})
    {
        std::filesystem::copy_file(fixture / "0" / name, root / "0.orig" / name);
    }

    const CaseImportResult result = CaseImporter().import(root);
    QVERIFY(result.ok);
    QCOMPARE(QString::fromStdString(result.report.startDirectory), QStringLiteral("0.orig"));
    QVERIFY(result.report.findFile("0.orig/p") != nullptr);
    QVERIFY(result.project.findFieldByName("p") != nullptr);
}

} // namespace ofgui::io

QTEST_APPLESS_MAIN(ofgui::io::CaseImporterTest)

#include "CaseImporterTest.moc"
