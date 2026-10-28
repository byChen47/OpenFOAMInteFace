#include "io/CaseImportOptions.h"
#include "io/CaseImportResult.h"
#include "io/CaseImporter.h"
#include "io/ImportCodes.h"

#include "core/ValidationIssue.h"

#include <QtTest>
#include <QTemporaryDir>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace ofgui::io {
namespace {

constexpr const char* kControlDict = R"(// controlDict fixture
FoamFile
{
    version     2.0;
    format      ascii;
    class       dictionary;
    object      controlDict;
}
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

application     icoFoam;

startFrom       startTime;

startTime       0;

stopAt          endTime;

endTime         1;

deltaT          0.001;

writeControl    timeStep;

writeInterval   100;
)";

constexpr const char* kSingleBlockMesh = R"(FoamFile
{
    version     2.0;
    format      ascii;
    class       dictionary;
    object      blockMeshDict;
}

scale   0.1;

vertices
(
    (0 0 0)
    (1 0 0)
    (1 1 0)
    (0 1 0)
    (0 0 0.1)
    (1 0 0.1)
    (1 1 0.1)
    (0 1 0.1)
);

blocks
(
    hex (0 1 2 3 4 5 6 7) (10 10 1) simpleGrading (1 1 1)
);

boundary
(
);
)";

constexpr const char* kTwoBlockMesh = R"(FoamFile
{
    version     2.0;
    format      ascii;
    class       dictionary;
    object      blockMeshDict;
}

scale   0.1;

vertices
(
    (0 0 0)
    (1 0 0)
    (1 1 0)
    (0 1 0)
    (0 0 0.1)
    (1 0 0.1)
    (1 1 0.1)
    (0 1 0.1)
);

blocks
(
    hex (0 1 2 3 4 5 6 7) (10 10 1) simpleGrading (1 1 1)
    hex (1 2 3 0 5 6 7 4) (10 10 1) simpleGrading (1 1 1)
);

boundary
(
);
)";

constexpr const char* kTriBlockMesh = R"(FoamFile
{
    version     2.0;
    format      ascii;
    class       dictionary;
    object      blockMeshDict;
}

scale   0.1;

vertices
(
    (0 0 0)
    (1 0 0)
    (1 1 0)
    (0 1 0)
    (0 0 0.1)
    (1 0 0.1)
    (1 1 0.1)
    (0 1 0.1)
);

blocks
(
    wedge (0 1 2 3 4 5 6 7) (10 10 1) simpleGrading (1 1 1)
);

boundary
(
);
)";

void writeFile(const std::filesystem::path& path, const std::string& content)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    stream << content;
}

std::filesystem::path makeMinimalCase(const QTemporaryDir& temporary)
{
    const std::filesystem::path root(temporary.path().toStdString());
    writeFile(root / "system" / "controlDict", kControlDict);
    writeFile(root / "system" / "blockMeshDict", kSingleBlockMesh);
    return root;
}

QString firstIssueText(const CaseImportResult& result)
{
    if (result.report.issues.empty())
    {
        return QStringLiteral("<none>");
    }
    return QString::fromStdString(result.report.issues.front().format());
}

} // namespace

class CaseImporterErrorsTest : public QObject
{
    Q_OBJECT

private slots:
    void emptyDirectoryIsBlocking();
    void missingCaseRootIsBlocking();
    void corruptedControlDictReportsParseError();
    void missingOptionalFilesAreWarnings();
    void twoBlocksAreBlocking();
    void nonHexBlockIsBlocking();
    void issuesCarryOriginAndRelativePaths();
    void issuesAreOrderedBySeverityThenLocation();
};

void CaseImporterErrorsTest::emptyDirectoryIsBlocking()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());

    const CaseImportResult result = CaseImporter().import(std::filesystem::path(temporary.path().toStdString()));
    QVERIFY2(!result.ok, "an empty directory must not import");
    QVERIFY(result.report.blocking());
    QVERIFY2(result.report.hasCode(importcodes::kCaseMissingControlDict), qPrintable(firstIssueText(result)));
    QVERIFY(result.report.hasCode(importcodes::kCaseMissingStartDirectory));
    QVERIFY(!result.report.hasCode(importcodes::kCaseRootMissing));
    QVERIFY(result.project.patches().empty());
    QVERIFY(result.project.fields().empty());
}

void CaseImporterErrorsTest::missingCaseRootIsBlocking()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const std::filesystem::path missing = std::filesystem::path(temporary.path().toStdString()) / "nope";

    const CaseImportResult result = CaseImporter().import(missing);
    QVERIFY(!result.ok);
    QVERIFY(result.report.blocking());
    QVERIFY(result.report.hasCode(importcodes::kCaseRootMissing));
}

void CaseImporterErrorsTest::corruptedControlDictReportsParseError()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const std::filesystem::path root(temporary.path().toStdString());
    writeFile(root / "system" / "controlDict", "FoamFile { this is not a dictionary\n");

    const CaseImportResult result = CaseImporter().import(root);
    QVERIFY(!result.ok);
    QVERIFY(result.report.hasCode("IMPORT-PARSE-001") || result.report.hasCode("IMPORT-PARSE-002"));

    const ImportedFile* file = result.report.findFile("system/controlDict");
    QVERIFY(file != nullptr);
    QVERIFY(file->hasParseErrors);
    QCOMPARE(file->level, SupportLevel::ReadOnly);

    // The parse error keeps its position and the case relative path.
    bool foundPositioned = false;
    for (const core::ValidationIssue& issue : result.report.issues)
    {
        if (issue.code().rfind("IMPORT-PARSE-", 0) == 0 && issue.location().has_value())
        {
            QCOMPARE(QString::fromStdString(issue.location()->file()), QStringLiteral("system/controlDict"));
            QVERIFY(issue.location()->line() >= 1);
            foundPositioned = true;
        }
    }
    QVERIFY(foundPositioned);
}

void CaseImporterErrorsTest::missingOptionalFilesAreWarnings()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const std::filesystem::path root(temporary.path().toStdString());
    writeFile(root / "system" / "controlDict", kControlDict);

    const CaseImportResult result = CaseImporter().import(root);
    QVERIFY2(result.ok, qPrintable(firstIssueText(result)));
    QVERIFY(!result.report.blocking());
    QVERIFY(result.report.hasCode(importcodes::kCaseMissingConstant));
    QVERIFY(result.report.hasCode(importcodes::kCaseMissingStartDirectory));
    // Missing blockMeshDict leaves the domain empty but is not blocking.
    QVERIFY(!result.project.domain().hasPositiveExtent());
}

void CaseImporterErrorsTest::twoBlocksAreBlocking()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const std::filesystem::path root(temporary.path().toStdString());
    writeFile(root / "system" / "controlDict", kControlDict);
    writeFile(root / "system" / "blockMeshDict", kTwoBlockMesh);

    const CaseImportResult result = CaseImporter().import(root);
    QVERIFY(!result.ok);
    QVERIFY(result.report.blocking());
    QVERIFY2(result.report.hasCode(importcodes::kMeshMultipleBlocks), qPrintable(firstIssueText(result)));
}

void CaseImporterErrorsTest::nonHexBlockIsBlocking()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const std::filesystem::path root(temporary.path().toStdString());
    writeFile(root / "system" / "controlDict", kControlDict);
    writeFile(root / "system" / "blockMeshDict", kTriBlockMesh);

    const CaseImportResult result = CaseImporter().import(root);
    QVERIFY(!result.ok);
    QVERIFY(result.report.blocking());
    QVERIFY2(result.report.hasCode(importcodes::kMeshNonHexBlock), qPrintable(firstIssueText(result)));
}

void CaseImporterErrorsTest::issuesCarryOriginAndRelativePaths()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const std::filesystem::path root = makeMinimalCase(temporary);

    const CaseImportResult result = CaseImporter().import(root);
    QVERIFY2(result.ok, qPrintable(firstIssueText(result)));
    QVERIFY(!result.report.issues.empty());

    for (const core::ValidationIssue& issue : result.report.issues)
    {
        QCOMPARE(QString::fromStdString(issue.origin()), QStringLiteral("import"));
        QVERIFY(!issue.message().empty());
        if (issue.location().has_value())
        {
            const std::string& file = issue.location()->file();
            QVERIFY(file.find(':') == std::string::npos || file.size() > 1);
            QVERIFY(file.empty() || file.front() != '/');
            QVERIFY(file.find('\\') == std::string::npos);
            QVERIFY(issue.location()->line() >= 0);
        }
    }
}

void CaseImporterErrorsTest::issuesAreOrderedBySeverityThenLocation()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const std::filesystem::path root(temporary.path().toStdString());
    writeFile(root / "system" / "controlDict", kControlDict);
    writeFile(root / "system" / "blockMeshDict", kTwoBlockMesh);

    const CaseImportResult result = CaseImporter().import(root);
    QVERIFY(!result.report.issues.empty());

    const auto rank = [](core::Severity severity) {
        switch (severity)
        {
        case core::Severity::Info:
            return 0;
        case core::Severity::Warning:
            return 1;
        case core::Severity::Error:
            return 2;
        case core::Severity::BlockingError:
            return 3;
        }
        return 0;
    };

    for (std::size_t index = 1; index < result.report.issues.size(); ++index)
    {
        const core::ValidationIssue& previous = result.report.issues[index - 1];
        const core::ValidationIssue& current = result.report.issues[index];
        const int previousRank = rank(previous.severity());
        const int currentRank = rank(current.severity());
        QVERIFY(previousRank >= currentRank);
        if (previousRank == currentRank)
        {
            const std::string previousFile = previous.location().has_value() ? previous.location()->file() : std::string{};
            const std::string currentFile = current.location().has_value() ? current.location()->file() : std::string{};
            QVERIFY(previousFile <= currentFile);
        }
    }
    // The blocking mesh error comes first.
    QCOMPARE(result.report.issues.front().severity(), core::Severity::BlockingError);
}

} // namespace ofgui::io

QTEST_APPLESS_MAIN(ofgui::io::CaseImporterErrorsTest)

#include "CaseImporterErrorsTest.moc"
