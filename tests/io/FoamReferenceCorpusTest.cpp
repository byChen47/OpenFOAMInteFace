#include "io/FoamDiagnostic.h"
#include "io/FoamDictionary.h"
#include "io/FoamNode.h"

#include <QtTest>

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QStringList>

#include <filesystem>
#include <string>

namespace ofgui::io {
namespace {

QString referenceRoot()
{
    return QStringLiteral(OFGUI_FOAM_REFERENCE_DIR);
}

QString referencePath(const char* relative)
{
    return referenceRoot() + QLatin1Char('/') + QString::fromUtf8(relative);
}

std::filesystem::path nativePath(const QString& path)
{
    return std::filesystem::path{path.toStdWString()};
}

FoamDictionary parseReferenceFile(const QString& path)
{
    return FoamDictionary::parseFile(nativePath(path));
}

QString firstErrorText(const FoamDictionary& dictionary)
{
    for (const FoamDiagnostic& diagnostic : dictionary.diagnostics())
    {
        if (diagnostic.severity == FoamSeverity::Error)
        {
            return QStringLiteral("%1: %2")
                .arg(QString::fromStdString(diagnostic.code), QString::fromStdString(diagnostic.message));
        }
    }
    return QStringLiteral("(no error diagnostic)");
}

bool referenceTreeAvailable()
{
    return QDir{referenceRoot()}.exists();
}

const char* const kCavityFiles[] = {
    "tutorials/incompressible/icoFoam/cavity/cavity/0/p",
    "tutorials/incompressible/icoFoam/cavity/cavity/0/U",
    "tutorials/incompressible/icoFoam/cavity/cavity/constant/transportProperties",
    "tutorials/incompressible/icoFoam/cavity/cavity/system/blockMeshDict",
    "tutorials/incompressible/icoFoam/cavity/cavity/system/controlDict",
    "tutorials/incompressible/icoFoam/cavity/cavity/system/fvSchemes",
    "tutorials/incompressible/icoFoam/cavity/cavity/system/fvSolution",
};

const char* const kCuratedFiles[] = {
    "tutorials/incompressible/simpleFoam/pipeCyclic/0.orig/U",
    "tutorials/incompressible/simpleFoam/pipeCyclic/0.orig/p",
    "tutorials/incompressible/simpleFoam/pipeCyclic/system/controlDict",
    "tutorials/incompressible/simpleFoam/pitzDaily/system/blockMeshDict",
    "tutorials/incompressible/simpleFoam/pitzDaily/system/fvSolution",
    "tutorials/basic/overLaplacianDyMFoam/heatTransfer/0.orig/pointDisplacement",
    "tutorials/multiphase/interFoam/laminar/damBreak/damBreak/system/blockMeshDict",
    "tutorials/multiphase/interFoam/laminar/damBreak/damBreak/system/fvSolution",
    "tutorials/incompressible/icoFoam/elbow/system/controlDict",
    "tutorials/incompressible/pimpleFoam/laminar/planarCouette/system/controlDict",
};

class FoamReferenceCorpusTest : public QObject
{
    Q_OBJECT

private slots:
    void referenceTreeIsAvailable();
    void cavityTutorialDictionaries();
    void curatedTutorialDictionaries();
    void wideIncompressibleScan();
};

void FoamReferenceCorpusTest::referenceTreeIsAvailable()
{
    if (!referenceTreeAvailable())
    {
        QSKIP("OpenFOAM-v2206 reference tree is not available in this checkout");
    }
    QVERIFY(QDir{referenceRoot()}.exists());
}

void FoamReferenceCorpusTest::cavityTutorialDictionaries()
{
    if (!referenceTreeAvailable())
    {
        QSKIP("OpenFOAM-v2206 reference tree is not available in this checkout");
    }

    for (const char* relative : kCavityFiles)
    {
        const QString path = referencePath(relative);
        QVERIFY2(QFileInfo::exists(path), qPrintable(QStringLiteral("missing: %1").arg(path)));
        const FoamDictionary dictionary = parseReferenceFile(path);
        QVERIFY2(
            dictionary.ok(),
            qPrintable(QStringLiteral("%1 -> %2").arg(path, dictionary.diagnosticsText().c_str())));
        QVERIFY(dictionary.hasFoamFileHeader());
    }
}

void FoamReferenceCorpusTest::curatedTutorialDictionaries()
{
    if (!referenceTreeAvailable())
    {
        QSKIP("OpenFOAM-v2206 reference tree is not available in this checkout");
    }

    QStringList skipped;
    int parsed = 0;
    for (const char* relative : kCuratedFiles)
    {
        const QString path = referencePath(relative);
        if (!QFileInfo::exists(path))
        {
            skipped << QString::fromUtf8(relative);
            continue;
        }
        const FoamDictionary dictionary = parseReferenceFile(path);
        QVERIFY2(
            dictionary.ok(),
            qPrintable(QStringLiteral("%1 -> %2").arg(path, dictionary.diagnosticsText().c_str())));
        ++parsed;
    }

    if (!skipped.isEmpty())
    {
        qWarning("curated corpus skipped %d missing file(s)", static_cast<int>(skipped.size()));
    }
    QVERIFY2(parsed >= 7, qPrintable(QStringLiteral("parsed only %1 curated file(s)").arg(parsed)));
}

void FoamReferenceCorpusTest::wideIncompressibleScan()
{
    if (!referenceTreeAvailable())
    {
        QSKIP("OpenFOAM-v2206 reference tree is not available in this checkout");
    }

    const QDir root{referencePath("tutorials/incompressible")};
    if (!root.exists())
    {
        QSKIP("tutorials/incompressible is not available in this checkout");
    }

    const QStringList dictNames{
        QStringLiteral("controlDict"),
        QStringLiteral("fvSchemes"),
        QStringLiteral("fvSolution"),
        QStringLiteral("blockMeshDict"),
        QStringLiteral("transportProperties"),
        QStringLiteral("decomposeParDict"),
    };

    constexpr int kMaxFiles = 250;
    constexpr qint64 kMaxBytes = 2 * 1024 * 1024;

    int parsed = 0;
    QStringList failures;
    QDirIterator iterator{root.absolutePath(), QDir::Files, QDirIterator::Subdirectories};
    while (iterator.hasNext() && parsed < kMaxFiles)
    {
        const QString path = iterator.next();
        if (!dictNames.contains(QFileInfo{path}.fileName()))
        {
            continue;
        }
        if (QFileInfo{path}.size() > kMaxBytes)
        {
            continue;
        }
        const FoamDictionary dictionary = parseReferenceFile(path);
        ++parsed;
        if (!dictionary.ok())
        {
            failures << QStringLiteral("%1 -> %2").arg(path, firstErrorText(dictionary));
        }
    }

    QVERIFY2(
        failures.isEmpty(),
        qPrintable(QStringLiteral("%1 of %2 file(s) failed:\n%3")
                       .arg(failures.size())
                       .arg(parsed)
                       .arg(failures.join(QLatin1Char('\n')))));
    QVERIFY2(parsed >= 20, qPrintable(QStringLiteral("scan parsed only %1 file(s)").arg(parsed)));
}

} // namespace
} // namespace ofgui::io

QTEST_APPLESS_MAIN(ofgui::io::FoamReferenceCorpusTest)

#include "FoamReferenceCorpusTest.moc"
