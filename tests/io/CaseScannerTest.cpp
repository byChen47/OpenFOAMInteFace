#include "io/CaseDirectoryScan.h"
#include "io/CaseImportOptions.h"
#include "io/CaseScanner.h"

#include <QtTest>
#include <QTemporaryDir>

#include <filesystem>
#include <fstream>
#include <string>

namespace ofgui::io {
namespace {

QString fixtureCaseRoot()
{
    return QString::fromUtf8(OFGUI_FOAM_FIXTURE_DIR) + QStringLiteral("/case");
}

bool containsEntry(const CaseDirectoryScan& scan, const std::string& path)
{
    for (const CaseScannedEntry& entry : scan.entries)
    {
        if (entry.relativePath == path)
        {
            return true;
        }
    }
    return false;
}

const CaseScannedEntry* findEntry(const CaseDirectoryScan& scan, const std::string& path)
{
    for (const CaseScannedEntry& entry : scan.entries)
    {
        if (entry.relativePath == path)
        {
            return &entry;
        }
    }
    return nullptr;
}

} // namespace

class CaseScannerTest : public QObject
{
    Q_OBJECT

private slots:
    void scansFixtureCase();
    void classifiesEntriesByLocation();
    void reportsPolyMeshCompleteness();
    void missingRootIsReported();
    void entryCapTruncatesTheListing();
    void startDirectoryPrefersZero();
    void startDirectoryFallsBackToZeroOrig();
};

void CaseScannerTest::scansFixtureCase()
{
    const CaseDirectoryScan scan = CaseScanner().scan(std::filesystem::path(fixtureCaseRoot().toStdString()));

    QVERIFY(scan.hasCaseRoot);
    QVERIFY(scan.hasStartDirectory);
    QCOMPARE(QString::fromStdString(scan.startDirectory), QStringLiteral("0"));
    QVERIFY(scan.hasConstantDirectory);
    QVERIFY(scan.hasSystemDirectory);
    QVERIFY(scan.hasControlDict);
    QVERIFY(scan.hasBlockMeshDict);
    QVERIFY(scan.hasFvSchemes);
    QVERIFY(scan.hasFvSolution);
    QVERIFY(scan.hasDecomposeParDict);
    QVERIFY(scan.hasTransportProperties);
    QVERIFY(!scan.hasTurbulenceProperties);
    QVERIFY(containsEntry(scan, "0/p"));
    QVERIFY(containsEntry(scan, "0/U"));
    QVERIFY(containsEntry(scan, "constant/transportProperties"));
    QVERIFY(containsEntry(scan, "system/controlDict"));
}

void CaseScannerTest::classifiesEntriesByLocation()
{
    const CaseDirectoryScan scan = CaseScanner().scan(std::filesystem::path(fixtureCaseRoot().toStdString()));

    const CaseScannedEntry* field = findEntry(scan, "0/p");
    QVERIFY(field != nullptr);
    QVERIFY(caseEntryKindName(field->kind) == "field");
    QVERIFY(field->byteSize > 0);
    QVERIFY(!field->directory);
    QVERIFY(!field->gzip);

    const CaseScannedEntry* dictionary = findEntry(scan, "system/controlDict");
    QVERIFY(dictionary != nullptr);
    QVERIFY(caseEntryKindName(dictionary->kind) == "dictionary");

    const CaseScannedEntry* mesh = findEntry(scan, "constant/polyMesh");
    QVERIFY(mesh != nullptr);
    QVERIFY(caseEntryKindName(mesh->kind) == "mesh-dir");
    QVERIFY(mesh->directory);

    // Paths are case relative and always use '/'.
    for (const CaseScannedEntry& entry : scan.entries)
    {
        QVERIFY(!entry.relativePath.empty());
        QVERIFY(entry.relativePath.front() != '/');
        QVERIFY(entry.relativePath.find('\\') == std::string::npos);
    }
}

void CaseScannerTest::reportsPolyMeshCompleteness()
{
    const CaseDirectoryScan scan = CaseScanner().scan(std::filesystem::path(fixtureCaseRoot().toStdString()));

    QVERIFY(scan.hasPolyMesh);
    QVERIFY(scan.polyMeshComplete);

    std::filesystem::path incomplete = std::filesystem::path(fixtureCaseRoot().toStdString()) / "constant" / "polyMesh";
    QVERIFY(std::filesystem::exists(incomplete / "points"));
}

void CaseScannerTest::missingRootIsReported()
{
    const CaseDirectoryScan scan =
        CaseScanner().scan(std::filesystem::path(fixtureCaseRoot().toStdString()) / "does-not-exist");

    QVERIFY(!scan.hasCaseRoot);
    QVERIFY(!scan.hasStartDirectory);
    QVERIFY(!scan.hasControlDict);
    QVERIFY(scan.entries.empty());
}

void CaseScannerTest::entryCapTruncatesTheListing()
{
    CaseImportOptions options;
    options.maxFiles = 2;
    const CaseDirectoryScan scan =
        CaseScanner(options).scan(std::filesystem::path(fixtureCaseRoot().toStdString()));

    QVERIFY(scan.truncated);
    QCOMPARE(scan.entries.size(), std::size_t{2});
    // The flags are computed from direct probes, not from the truncated listing.
    QVERIFY(scan.hasControlDict);
    QVERIFY(scan.hasBlockMeshDict);
}

void CaseScannerTest::startDirectoryPrefersZero()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const std::filesystem::path root(temporary.path().toStdString());
    std::filesystem::create_directories(root / "0");
    std::filesystem::create_directories(root / "0.orig");
    std::ofstream(root / "0" / "p") << "// 0\n";
    std::ofstream(root / "0.orig" / "p") << "// 0.orig\n";

    const CaseDirectoryScan scan = CaseScanner().scan(root);
    QCOMPARE(QString::fromStdString(scan.startDirectory), QStringLiteral("0"));
    QVERIFY(containsEntry(scan, "0/p"));
    // Only the chosen start directory is listed; 0.orig is just recorded by name.
    QVERIFY(!containsEntry(scan, "0.orig/p"));
}

void CaseScannerTest::startDirectoryFallsBackToZeroOrig()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const std::filesystem::path root(temporary.path().toStdString());
    std::filesystem::create_directories(root / "0.orig");
    std::ofstream(root / "0.orig" / "p") << "// 0.orig\n";

    const CaseDirectoryScan scan = CaseScanner().scan(root);
    QCOMPARE(QString::fromStdString(scan.startDirectory), QStringLiteral("0.orig"));
    QVERIFY(scan.hasStartDirectory);
}

} // namespace ofgui::io

QTEST_APPLESS_MAIN(ofgui::io::CaseScannerTest)

#include "CaseScannerTest.moc"
