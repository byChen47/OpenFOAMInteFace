#include "io/AtomicFileWriter.h"
#include "io/CaseImportResult.h"
#include "io/CaseImporter.h"
#include "io/ImportCodes.h"
#include "io/writers/AtomicCaseDirectory.h"
#include "io/writers/CaseExportOptions.h"
#include "io/writers/CaseExportPlan.h"
#include "io/writers/CaseExporter.h"
#include "io/writers/ExportPlanBuilder.h"

#include <core/CaseProject.h>

#include <QtTest>
#include <QTemporaryDir>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>
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
    return std::string((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
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

bool hasParseIssue(const std::vector<core::ValidationIssue>& issues)
{
    return std::any_of(issues.begin(), issues.end(), [](const core::ValidationIssue& issue)
                       { return issue.code().rfind("IMPORT-PARSE-", 0) == 0; });
}

const ExportFile* findFile(const CaseExportPlan& plan, std::string_view relativePath)
{
    for (const ExportFile& file : plan.files)
    {
        if (file.relativePath == relativePath)
        {
            return &file;
        }
    }
    return nullptr;
}

CaseExportOptions optionsFor(const std::filesystem::path& target)
{
    CaseExportOptions options;
    options.targetDirectory = target;
    options.transport.kinematicViscosity = 0.01;
    return options;
}

CaseImportResult importFixture()
{
    return CaseImporter{}.import(fixturesRoot() / "case");
}

} // namespace

/// Regression tests for the io findings of review t14 (F-02, F-03, F-07, F-08).
class ExportSafetyTest : public QObject
{
    Q_OBJECT

private slots:
    void malformedSchemeTextIsReportedAndCopiedVerbatim();
    void malformedSchemeTextBlocksTheExport();
    void malformedSolutionTextIsReportedAndCopiedVerbatim();
    void defaultSchemeAndSolutionTextsProduceValidDictionaries();
    void writeFailureKeepsThePreviousTargetAndRemovesStaging();
    void mirrorFailureKeepsThePreviousTargetAndRemovesStaging();
    void emptyExistingTargetNeedsNoConfirmation();
    void fileWriterLeavesNoStagingArtifacts();
    void fileWriterReportsUnwritableTargetWithoutTouchingIt();
};

void ExportSafetyTest::malformedSchemeTextIsReportedAndCopiedVerbatim()
{
    const CaseImportResult imported = importFixture();
    QVERIFY(imported.ok);

    const std::string broken = "ddtSchemes\n{\n    default         Euler;\n";
    CaseExportOptions options = optionsFor(std::filesystem::path{"unused"});
    options.fvSchemesText = broken;

    std::vector<core::ValidationIssue> issues;
    const CaseExportPlan plan = ExportPlanBuilder::build(imported.project, options, issues);

    // The parser errors must surface instead of a silently empty dictionary.
    QVERIFY(hasParseIssue(issues));
    const ExportFile* file = findFile(plan, "system/fvSchemes");
    QVERIFY(file != nullptr);
    QCOMPARE(file->text, broken);
    QVERIFY(!file->text.empty());
}

void ExportSafetyTest::malformedSchemeTextBlocksTheExport()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());

    const CaseImportResult imported = importFixture();
    QVERIFY(imported.ok);

    const std::filesystem::path target = std::filesystem::path{workspace.path().toStdString()} / "case";
    CaseExportOptions options = optionsFor(target);
    options.fvSchemesText = "ddtSchemes\n{\n    default         Euler;\n";

    const CaseExportResult result = CaseExporter{}.exportProject(imported.project, options);

    QVERIFY(!result.ok);
    QVERIFY(hasParseIssue(result.issues));
    // Nothing may be written when a pre-write issue is an error.
    QVERIFY(!std::filesystem::exists(target));
}

void ExportSafetyTest::malformedSolutionTextIsReportedAndCopiedVerbatim()
{
    const CaseImportResult imported = importFixture();
    QVERIFY(imported.ok);

    const std::string broken = "solvers\n{\n    p\n    {\n        solver  PCG;\n";
    CaseExportOptions options = optionsFor(std::filesystem::path{"unused"});
    options.fvSolutionText = broken;

    std::vector<core::ValidationIssue> issues;
    const CaseExportPlan plan = ExportPlanBuilder::build(imported.project, options, issues);

    QVERIFY(hasParseIssue(issues));
    const ExportFile* file = findFile(plan, "system/fvSolution");
    QVERIFY(file != nullptr);
    QCOMPARE(file->text, broken);
}

void ExportSafetyTest::defaultSchemeAndSolutionTextsProduceValidDictionaries()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());

    const CaseImportResult imported = importFixture();
    QVERIFY(imported.ok);

    const std::filesystem::path target = std::filesystem::path{workspace.path().toStdString()} / "case";
    const CaseExportOptions options = optionsFor(target);

    // No caller supplied text: the built-in templates must parse and export.
    std::vector<core::ValidationIssue> issues;
    const CaseExportPlan plan = ExportPlanBuilder::build(imported.project, options, issues);
    QVERIFY(!hasParseIssue(issues));

    const std::pair<const char*, const char*> expected[]{
        {"system/fvSchemes", "fvSchemes"},
        {"system/fvSolution", "fvSolution"},
    };
    for (const auto& [relativePath, objectName] : expected)
    {
        const ExportFile* file = findFile(plan, relativePath);
        QVERIFY2(file != nullptr, relativePath);
        QVERIFY2(file->text.find("FoamFile") != std::string::npos, file->text.c_str());
        QVERIFY2(file->text.find("ascii") != std::string::npos, file->text.c_str());
        QVERIFY2(file->text.find(objectName) != std::string::npos, file->text.c_str());
    }

    const CaseExportResult result = CaseExporter{}.exportProject(imported.project, options);
    QString details;
    for (const core::ValidationIssue& issue : result.issues)
    {
        details += QString::fromStdString(issue.format()) + QLatin1Char('\n');
    }
    QVERIFY2(result.ok, qPrintable(details));
    QVERIFY(std::filesystem::exists(target / "system" / "fvSchemes"));
    QVERIFY(std::filesystem::exists(target / "system" / "fvSolution"));
}

void ExportSafetyTest::writeFailureKeepsThePreviousTargetAndRemovesStaging()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());

    const std::filesystem::path root{workspace.path().toStdString()};
    const std::filesystem::path target = root / "case";
    writeFile(target / "keep.txt", "old\n");

    CaseExportPlan plan;
    ExportFile first;
    first.relativePath = "system/blockMeshDict";
    first.text = "first\n";
    plan.files.push_back(first);

    // A child of an already planned file cannot be created, which fails the
    // staging write before anything is renamed into place.
    ExportFile impossible;
    impossible.relativePath = "system/blockMeshDict/child";
    impossible.text = "second\n";
    plan.files.push_back(impossible);

    AtomicCaseDirectory writer{target};
    const CaseWriteStatus status = writer.write(plan, true);

    QVERIFY(status != CaseWriteStatus::Success);
    QCOMPARE(readFile(target / "keep.txt"), std::string("old\n"));
    QVERIFY(writer.backupDirectory().empty());
    for (const std::string& name : directoryNames(root))
    {
        QVERIFY2(name.find(".tmp") == std::string::npos, name.c_str());
    }
}

void ExportSafetyTest::mirrorFailureKeepsThePreviousTargetAndRemovesStaging()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());

    const std::filesystem::path root{workspace.path().toStdString()};
    const std::filesystem::path target = root / "case";
    writeFile(target / "keep.txt", "old\n");

    CaseExportPlan plan;
    ExportFile copy;
    copy.relativePath = "constant/polyMesh/points";
    copy.action = ExportAction::Mirrored;
    copy.sourcePath = (root / "missing-source" / "points").string();
    plan.files.push_back(copy);

    AtomicCaseDirectory writer{target};
    const CaseWriteStatus status = writer.write(plan, true);

    QCOMPARE(status, CaseWriteStatus::MirrorFailed);
    QCOMPARE(readFile(target / "keep.txt"), std::string("old\n"));
    QVERIFY(writer.backupDirectory().empty());
    QVERIFY(!writer.message().empty());
    for (const std::string& name : directoryNames(root))
    {
        QVERIFY2(name.find(".tmp") == std::string::npos, name.c_str());
    }
}

void ExportSafetyTest::emptyExistingTargetNeedsNoConfirmation()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());

    const std::filesystem::path root{workspace.path().toStdString()};
    const std::filesystem::path target = root / "case";
    std::filesystem::create_directories(target);

    const CaseImportResult imported = importFixture();
    QVERIFY(imported.ok);

    const CaseExportResult result = CaseExporter{}.exportProject(imported.project, optionsFor(target));

    QVERIFY(result.ok);
    QVERIFY(result.backupDirectory.empty());
    QVERIFY(std::filesystem::exists(target / "system" / "blockMeshDict"));
    QCOMPARE(directoryNames(root), std::vector<std::string>{"case"});
}

void ExportSafetyTest::fileWriterLeavesNoStagingArtifacts()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());

    const std::filesystem::path root{workspace.path().toStdString()};
    const std::filesystem::path target = root / "project.ofgproj";
    writeFile(target, "old content\n");

    const AtomicWriteResult result = AtomicFileWriter{target}.write(std::string_view{"new content\n"});

    QVERIFY(result.ok());
    QCOMPARE(readFile(target), std::string("new content\n"));
    QCOMPARE(directoryNames(root), std::vector<std::string>{"project.ofgproj"});
}

void ExportSafetyTest::fileWriterReportsUnwritableTargetWithoutTouchingIt()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());

    const std::filesystem::path root{workspace.path().toStdString()};
    // `blocker` is a regular file, so the target's parent directory cannot be
    // created and the write must fail before any byte is written.
    writeFile(root / "blocker", "file\n");
    const std::filesystem::path target = root / "blocker" / "project.ofgproj";

    const AtomicWriteResult result = AtomicFileWriter{target}.write(std::string_view{"data\n"});

    QVERIFY(!result.ok());
    QVERIFY(!result.message.isEmpty());
    QVERIFY(!std::filesystem::exists(target));
    QCOMPARE(readFile(root / "blocker"), std::string("file\n"));
    QCOMPARE(directoryNames(root), std::vector<std::string>{"blocker"});
}

} // namespace ofgui::io

QTEST_APPLESS_MAIN(ofgui::io::ExportSafetyTest)
#include "ExportSafetyTest.moc"
