#include "io/CaseImportResult.h"
#include "io/CaseImporter.h"
#include "io/writers/CaseExportOptions.h"
#include "io/writers/CaseExportPlan.h"
#include "io/writers/CaseExporter.h"

#include <core/CaseProject.h>

#include <QtTest>
#include <QTemporaryDir>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <sstream>
#include <string>

namespace ofgui::io {
namespace {

std::filesystem::path fixturesRoot()
{
    return std::filesystem::path{OFGUI_FOAM_FIXTURE_DIR};
}

std::filesystem::path goldenRoot()
{
    return fixturesRoot().parent_path() / "golden";
}

std::string readFile(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return buffer.str();
}

QString issueCodes(const CaseExportResult& result)
{
    QStringList codes;
    for (const core::ValidationIssue& issue : result.issues)
    {
        codes << QString::fromStdString(issue.code());
    }
    return codes.join(QStringLiteral(", "));
}

QString describeDifference(const std::string& produced, const std::string& expected)
{
    const std::size_t limit = std::min(produced.size(), expected.size());
    for (std::size_t index = 0; index < limit; ++index)
    {
        if (produced[index] != expected[index])
        {
            const std::size_t from = index > 60 ? index - 60 : 0;
            return QStringLiteral("first difference at byte %1\nproduced: [%2]\nexpected: [%3]")
                .arg(static_cast<qulonglong>(index))
                .arg(QString::fromUtf8(produced.substr(from, 160).c_str()))
                .arg(QString::fromUtf8(expected.substr(from, 160).c_str()));
        }
    }
    if (produced.size() != expected.size())
    {
        return QStringLiteral("produced %1 bytes, expected %2").arg(produced.size()).arg(expected.size());
    }
    return QStringLiteral("identical");
}

} // namespace

class CaseWriterGoldenTest : public QObject
{
    Q_OBJECT

private slots:
    void cavityCaseReproducesTutorialFiles();
    void generatedFilesAreLfWithSingleTrailingNewline();
    void exportedCaseCanBeImportedBack();
};

void CaseWriterGoldenTest::cavityCaseReproducesTutorialFiles()
{
    QTemporaryDir target;
    QVERIFY(target.isValid());

    CaseImporter importer;
    const CaseImportResult imported = importer.import(fixturesRoot() / "case");
    QVERIFY(imported.ok);

    CaseExportOptions options;
    options.targetDirectory = std::filesystem::path{target.path().toStdString()};
    options.transport.kinematicViscosity = imported.transport.kinematicViscosity;
    options.fvSchemesText = readFile(fixturesRoot() / "cavity/system/fvSchemes");
    options.fvSolutionText = readFile(fixturesRoot() / "cavity/system/fvSolution");

    CaseExporter exporter;
    const CaseExportResult result = exporter.exportProject(imported.project, options);
    QVERIFY2(result.ok, qPrintable(QStringLiteral("export failed: ") + issueCodes(result)));
    QCOMPARE(result.fileCount, std::size_t{7});

    // 0/p, 0/U and every dictionary the tutorial keeps identical must match the
    // v2206 snapshot byte for byte; fvSolution differs only by the documented
    // `$p;` expansion.
    const std::map<std::string, std::filesystem::path> expected{
        {"0/p", fixturesRoot() / "cavity/0/p"},
        {"0/U", fixturesRoot() / "cavity/0/U"},
        {"system/blockMeshDict", fixturesRoot() / "cavity/system/blockMeshDict"},
        {"system/controlDict", fixturesRoot() / "cavity/system/controlDict"},
        {"system/fvSchemes", fixturesRoot() / "cavity/system/fvSchemes"},
        {"constant/transportProperties", fixturesRoot() / "cavity/constant/transportProperties"},
        {"system/fvSolution", goldenRoot() / "cavity/system/fvSolution"},
    };

    for (const auto& [relativePath, expectedPath] : expected)
    {
        const std::string produced = readFile(options.targetDirectory / relativePath);
        const std::string wanted = readFile(expectedPath);
        QVERIFY2(
            produced == wanted,
            qPrintable(QStringLiteral("%1: %2").arg(
                QString::fromStdString(relativePath), describeDifference(produced, wanted))));
    }
}

void CaseWriterGoldenTest::generatedFilesAreLfWithSingleTrailingNewline()
{
    QTemporaryDir target;
    QVERIFY(target.isValid());

    CaseImporter importer;
    const CaseImportResult imported = importer.import(fixturesRoot() / "case");
    QVERIFY(imported.ok);

    CaseExportOptions options;
    options.targetDirectory = std::filesystem::path{target.path().toStdString()};
    options.transport.kinematicViscosity = imported.transport.kinematicViscosity;
    options.fvSchemesText = readFile(fixturesRoot() / "cavity/system/fvSchemes");
    options.fvSolutionText = readFile(fixturesRoot() / "cavity/system/fvSolution");

    CaseExporter exporter;
    const CaseExportResult result = exporter.exportProject(imported.project, options);
    QVERIFY(result.ok);

    for (const ExportFile& file : result.plan.files)
    {
        QVERIFY(!file.text.empty());
        QVERIFY2(file.text.find('\r') == std::string::npos, file.relativePath.c_str());
        QVERIFY(file.text.back() == '\n');
        QVERIFY(file.text.size() < 2 || file.text[file.text.size() - 2] != '\n');
        QVERIFY(file.text.substr(0, 3) != "\xEF\xBB\xBF");
    }
}

void CaseWriterGoldenTest::exportedCaseCanBeImportedBack()
{
    QTemporaryDir target;
    QVERIFY(target.isValid());

    CaseImporter importer;
    const CaseImportResult first = importer.import(fixturesRoot() / "case");
    QVERIFY(first.ok);

    CaseExportOptions options;
    options.targetDirectory = std::filesystem::path{target.path().toStdString()};
    options.transport.kinematicViscosity = first.transport.kinematicViscosity;
    options.fvSchemesText = readFile(fixturesRoot() / "cavity/system/fvSchemes");
    options.fvSolutionText = readFile(fixturesRoot() / "cavity/system/fvSolution");

    CaseExporter exporter;
    QVERIFY(exporter.exportProject(first.project, options).ok);

    const CaseImportResult second = importer.import(options.targetDirectory);
    QVERIFY(second.ok);
    QCOMPARE(second.report.hasCode("IMPORT-CASE-001"), false);

    QCOMPARE(second.project.patches().size(), first.project.patches().size());
    QCOMPARE(second.project.fields().size(), first.project.fields().size());
    QVERIFY(second.project.domain().min().isApprox(first.project.domain().min()));
    QVERIFY(second.project.domain().max().isApprox(first.project.domain().max()));
    QCOMPARE(second.project.mesh().cells(), first.project.mesh().cells());

    const core::FieldDefinition* pressure = second.project.findFieldByName("p");
    QVERIFY(pressure != nullptr);
    const std::array<int, 7> pressureDimensions{0, 2, -2, 0, 0, 0, 0};
    QCOMPARE(pressure->dimensions(), pressureDimensions);
}

} // namespace ofgui::io

QTEST_APPLESS_MAIN(ofgui::io::CaseWriterGoldenTest)
#include "CaseWriterGoldenTest.moc"
