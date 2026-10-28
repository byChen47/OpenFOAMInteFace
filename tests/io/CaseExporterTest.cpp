#include "io/CaseImportResult.h"
#include "io/CaseImporter.h"
#include "io/ImportCodes.h"
#include "io/writers/CaseExportOptions.h"
#include "io/writers/CaseExportPlan.h"
#include "io/writers/CaseExporter.h"
#include "io/writers/ExportManifestSchema.h"

#include <core/CaseProject.h>

#include <QtTest>
#include <QTemporaryDir>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace ofgui::io {
namespace {

std::filesystem::path fixturesRoot()
{
    return std::filesystem::path{OFGUI_FOAM_FIXTURE_DIR};
}

std::string readFile(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    std::string text((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    return text;
}

void writeFile(const std::filesystem::path& path, const std::string& text)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    stream << text;
}

std::vector<std::string> directoryNames(const std::filesystem::path& directory)
{
    std::vector<std::string> names;
    if (!std::filesystem::is_directory(directory))
    {
        return names;
    }
    for (const auto& entry : std::filesystem::directory_iterator(directory))
    {
        names.push_back(entry.path().filename().string());
    }
    std::sort(names.begin(), names.end());
    return names;
}

CaseImportResult importFixture()
{
    CaseImporter importer;
    return importer.import(fixturesRoot() / "case");
}

CaseExportOptions optionsFor(const std::filesystem::path& target)
{
    CaseExportOptions options;
    options.targetDirectory = target;
    options.transport.kinematicViscosity = 0.01;
    return options;
}

bool hasCode(const CaseExportResult& result, std::string_view code)
{
    return std::any_of(result.issues.begin(), result.issues.end(), [code](const core::ValidationIssue& issue)
                       { return issue.code() == code; });
}

} // namespace

class CaseExporterTest : public QObject
{
    Q_OBJECT

private slots:
    void rejectsProjectThatFailsValidation();
    void rejectsTargetInsideSourceCase();
    void refusesExistingTargetWithoutConfirmation();
    void replacesExistingTargetAndKeepsBackup();
    void writesOnlyInsideTheTargetDirectory();
    void planListsTheExpectedRelativePaths();
    void mirrorsPolyMeshAndDecomposeParDict();
    void manifestDescribesTheFinishedExport();
    void skippedListNamesRuntimeProducts();
    void refusesSecondExportWithoutConfirmation();
};

void CaseExporterTest::rejectsProjectThatFailsValidation()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());

    const std::filesystem::path target = std::filesystem::path{workspace.path().toStdString()} / "case";
    const core::CaseProject empty;

    CaseExporter exporter;
    const CaseExportResult result = exporter.exportProject(empty, optionsFor(target));

    QVERIFY(!result.ok);
    QVERIFY(result.hasBlockingIssues());
    QVERIFY(hasCode(result, importcodes::kExportRejectedByValidation));
    QVERIFY(!std::filesystem::exists(target));
}

void CaseExporterTest::rejectsTargetInsideSourceCase()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());

    const CaseImportResult imported = importFixture();
    QVERIFY(imported.ok);

    CaseExportOptions options = optionsFor(std::filesystem::path{workspace.path().toStdString()} / "export");
    options.sourceCaseRoot = fixturesRoot().generic_string();
    options.targetDirectory = fixturesRoot() / "case" / "exported";

    CaseExporter exporter;
    const CaseExportResult result = exporter.exportProject(imported.project, options);

    QVERIFY(!result.ok);
    QVERIFY(hasCode(result, importcodes::kExportTargetInsideSource));
    QVERIFY(!std::filesystem::exists(options.targetDirectory));
}

void CaseExporterTest::refusesExistingTargetWithoutConfirmation()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());

    const std::filesystem::path target = std::filesystem::path{workspace.path().toStdString()} / "case";
    writeFile(target / "keep.txt", "untouched\n");

    const CaseImportResult imported = importFixture();
    QVERIFY(imported.ok);

    CaseExporter exporter;
    const CaseExportResult result = exporter.exportProject(imported.project, optionsFor(target));

    QVERIFY(!result.ok);
    QVERIFY(hasCode(result, importcodes::kExportTargetExists));
    QCOMPARE(readFile(target / "keep.txt"), std::string("untouched\n"));
    QVERIFY(!std::filesystem::exists(target / "system"));
}

void CaseExporterTest::replacesExistingTargetAndKeepsBackup()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());

    const std::filesystem::path root{workspace.path().toStdString()};
    const std::filesystem::path target = root / "case";
    writeFile(target / "keep.txt", "old\n");

    const CaseImportResult imported = importFixture();
    QVERIFY(imported.ok);

    CaseExportOptions options = optionsFor(target);
    options.allowExistingTarget = true;

    CaseExporter exporter;
    const CaseExportResult result = exporter.exportProject(imported.project, options);

    QVERIFY(result.ok);
    QVERIFY(!result.backupDirectory.empty());
    QCOMPARE(readFile(result.backupDirectory / "keep.txt"), std::string("old\n"));
    QVERIFY(std::filesystem::exists(target / "system/blockMeshDict"));
    QVERIFY(!std::filesystem::exists(target / "keep.txt"));

    // The temporary directory must not survive a successful export.
    for (const std::string& name : directoryNames(root))
    {
        QVERIFY2(name.find(".tmp") == std::string::npos, name.c_str());
    }
}

void CaseExporterTest::writesOnlyInsideTheTargetDirectory()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());

    const std::filesystem::path root{workspace.path().toStdString()};
    const std::filesystem::path target = root / "case";

    const CaseImportResult imported = importFixture();
    QVERIFY(imported.ok);

    CaseExporter exporter;
    const CaseExportResult result = exporter.exportProject(imported.project, optionsFor(target));
    QVERIFY(result.ok);

    QCOMPARE(directoryNames(root), std::vector<std::string>{"case"});
    QVERIFY(std::filesystem::exists(target / "0/p"));
    QVERIFY(std::filesystem::exists(target / "0/U"));
    QVERIFY(std::filesystem::exists(target / "constant/transportProperties"));
    QVERIFY(std::filesystem::exists(target / "system/controlDict"));
}

void CaseExporterTest::planListsTheExpectedRelativePaths()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());

    const CaseImportResult imported = importFixture();
    QVERIFY(imported.ok);

    CaseExporter exporter;
    const CaseExportResult result = exporter.exportProject(
        imported.project, optionsFor(std::filesystem::path{workspace.path().toStdString()} / "case"));
    QVERIFY(result.ok);
    QVERIFY(result.totalBytes > 0);

    std::vector<std::string> paths;
    for (const ExportFile& file : result.plan.files)
    {
        paths.push_back(file.relativePath);
    }
    std::sort(paths.begin(), paths.end());

    const std::vector<std::string> expected{
        "0/U",
        "0/p",
        "constant/transportProperties",
        "system/blockMeshDict",
        "system/controlDict",
        "system/fvSchemes",
        "system/fvSolution",
    };
    QCOMPARE(paths, expected);
}

void CaseExporterTest::mirrorsPolyMeshAndDecomposeParDict()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());

    const CaseImportResult imported = importFixture();
    QVERIFY(imported.ok);

    CaseExportOptions options = optionsFor(std::filesystem::path{workspace.path().toStdString()} / "case");
    options.sourceCaseRoot = (fixturesRoot() / "case").generic_string();

    CaseExporter exporter;
    const CaseExportResult result = exporter.exportProject(imported.project, options);
    QVERIFY(result.ok);

    // The mesh payload is duplicated byte for byte, not regenerated.
    for (const char* name : {"points", "faces", "owner", "neighbour", "boundary"})
    {
        const std::filesystem::path relative = std::filesystem::path{"constant"} / "polyMesh" / name;
        QVERIFY(std::filesystem::exists(options.targetDirectory / relative));
        QCOMPARE(readFile(options.targetDirectory / relative), readFile(fixturesRoot() / "case" / relative));
    }
    QVERIFY(std::filesystem::exists(options.targetDirectory / "system" / "decomposeParDict"));

    QCOMPARE(result.mirroredCount, std::size_t{5});
    QCOMPARE(result.copiedCount, std::size_t{1});
    QCOMPARE(result.writtenCount, std::size_t{7});
}

void CaseExporterTest::manifestDescribesTheFinishedExport()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());

    const CaseImportResult imported = importFixture();
    QVERIFY(imported.ok);

    CaseExportOptions options = optionsFor(std::filesystem::path{workspace.path().toStdString()} / "case");
    options.sourceCaseRoot = (fixturesRoot() / "case").generic_string();

    CaseExporter exporter;
    const CaseExportResult result = exporter.exportProject(imported.project, options);
    QVERIFY(result.ok);

    const std::filesystem::path manifestPath =
        options.targetDirectory / std::filesystem::path{exportmanifest::kFileName};
    QVERIFY(std::filesystem::exists(manifestPath));

    const std::string text = readFile(manifestPath);
    QVERIFY(text.find('\r') == std::string::npos);
    QJsonParseError error{};
    const QJsonDocument document = QJsonDocument::fromJson(QByteArray::fromStdString(text), &error);
    QCOMPARE(error.error, QJsonParseError::NoError);
    QVERIFY(document.isObject());

    const QJsonObject root = document.object();
    const QJsonArray files = root.value(QLatin1String{exportmanifest::kFilesKey}).toArray();
    QCOMPARE(static_cast<std::size_t>(files.size()), result.plan.files.size());

    // Every planned file appears with its relative path, and the manifest does
    // not describe itself.
    std::vector<std::string> listed;
    for (const QJsonValue& value : files)
    {
        const std::string path =
            value.toObject().value(QLatin1String{exportmanifest::kRelativePathKey}).toString().toStdString();
        listed.push_back(path);
        QVERIFY(path != exportmanifest::kFileName);
    }
    QVERIFY(std::find(listed.begin(), listed.end(), std::string{"0/p"}) != listed.end());
    QVERIFY(std::find(listed.begin(), listed.end(), std::string{"constant/polyMesh/points"}) != listed.end());

    const QJsonObject counts = root.value(QLatin1String{exportmanifest::kCountsKey}).toObject();
    QCOMPARE(counts.value(QStringLiteral("written")).toInt(), static_cast<int>(result.writtenCount));
    QCOMPARE(counts.value(QStringLiteral("mirrored")).toInt(), static_cast<int>(result.mirroredCount));
    QCOMPARE(counts.value(QStringLiteral("copied")).toInt(), static_cast<int>(result.copiedCount));
}

void CaseExporterTest::skippedListNamesRuntimeProducts()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());

    const std::filesystem::path root{workspace.path().toStdString()};
    const std::filesystem::path source = root / "source";
    // Reuse the fixture tree and add runtime products on a copy.
    std::filesystem::copy(fixturesRoot() / "case", source,
                          std::filesystem::copy_options::recursive);
    writeFile(source / "Allrun", "#!/bin/sh\n");
    writeFile(source / "log.blockMesh", "log\n");
    writeFile(source / "postProcessing" / "forces" / "0" / "forces.dat", "dat\n");

    CaseImportResult imported = CaseImporter{}.import(source);
    QVERIFY(imported.ok);

    CaseExportOptions options = optionsFor(root / "case");
    options.sourceCaseRoot = source.generic_string();

    const CaseExportResult result = CaseExporter{}.exportProject(imported.project, options);
    QVERIFY(result.ok);

    QVERIFY(!std::filesystem::exists(options.targetDirectory / "Allrun"));
    QVERIFY(!std::filesystem::exists(options.targetDirectory / "postProcessing"));

    const std::string text =
        readFile(options.targetDirectory / std::filesystem::path{exportmanifest::kFileName});
    const QJsonArray skipped = QJsonDocument::fromJson(QByteArray::fromStdString(text))
                                   .object()
                                   .value(QLatin1String{exportmanifest::kSkippedKey})
                                   .toArray();
    std::vector<std::string> skippedPaths;
    for (const QJsonValue& value : skipped)
    {
        skippedPaths.push_back(
            value.toObject().value(QLatin1String{exportmanifest::kRelativePathKey}).toString().toStdString());
    }
    QVERIFY(std::find(skippedPaths.begin(), skippedPaths.end(), std::string{"Allrun"}) != skippedPaths.end());
    QVERIFY(std::find(skippedPaths.begin(), skippedPaths.end(), std::string{"log.blockMesh"})
            != skippedPaths.end());
}

void CaseExporterTest::refusesSecondExportWithoutConfirmation()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());

    const std::filesystem::path target = std::filesystem::path{workspace.path().toStdString()} / "case";
    const CaseImportResult imported = importFixture();
    QVERIFY(imported.ok);

    CaseExporter exporter;
    QVERIFY(exporter.exportProject(imported.project, optionsFor(target)).ok);

    const std::string manifestBefore =
        readFile(target / std::filesystem::path{exportmanifest::kFileName});
    const CaseExportResult second = exporter.exportProject(imported.project, optionsFor(target));

    QVERIFY(!second.ok);
    QVERIFY(hasCode(second, importcodes::kExportTargetExists));
    QCOMPARE(readFile(target / std::filesystem::path{exportmanifest::kFileName}), manifestBefore);
}

} // namespace ofgui::io

QTEST_APPLESS_MAIN(ofgui::io::CaseExporterTest)
#include "CaseExporterTest.moc"
