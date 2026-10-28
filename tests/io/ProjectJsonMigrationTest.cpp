#include "core/CaseProject.h"
#include "core/CaseSourceInfo.h"
#include "io/ProjectJsonDiagnostic.h"
#include "io/ProjectJsonRepository.h"
#include "io/ProjectJsonSchema.h"

#include <QByteArray>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QTemporaryDir>
#include <QtTest>

#include <filesystem>
#include <string>
#include <vector>

namespace ofgui::io {
namespace {

std::filesystem::path toPath(const QString& text)
{
    return std::filesystem::path(text.toStdWString());
}

QByteArray readBytes(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        return {};
    }
    return file.readAll();
}

bool writeBytes(const QString& path, const QByteArray& bytes)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        return false;
    }
    return file.write(bytes) == bytes.size();
}

QJsonObject parseObject(const QByteArray& bytes)
{
    return QJsonDocument::fromJson(bytes).object();
}

bool hasCode(const std::vector<ProjectJsonDiagnostic>& diagnostics, const char* code)
{
    for (const ProjectJsonDiagnostic& diagnostic : diagnostics)
    {
        if (diagnostic.code == code)
        {
            return true;
        }
    }
    return false;
}

const ProjectJsonDiagnostic* firstWithCode(
    const std::vector<ProjectJsonDiagnostic>& diagnostics,
    const char* code)
{
    for (const ProjectJsonDiagnostic& diagnostic : diagnostics)
    {
        if (diagnostic.code == code)
        {
            return &diagnostic;
        }
    }
    return nullptr;
}

/// schemaVersion 1 document: the format written before the `source` section
/// existed. Only the model keys that carry no default are present.
QByteArray legacyDocument()
{
    return QByteArrayLiteral(R"JSON({
    "schemaVersion": 1,
    "project": {"name": "legacy", "caseDirectory": "cases/legacy"},
    "domain": {"min": [0, 0, 0], "max": [0.1, 0.1, 0.01], "mode": "twoD", "unit": "m"},
    "mesh": {"cells": [20, 20, 1], "grading": [1, 1, 1], "scale": 0.1},
    "solver": {
        "application": "icoFoam",
        "startTime": 0,
        "endTime": 0.5,
        "deltaT": 0.005,
        "writeInterval": 20
    }
})JSON");
}

/// schemaVersion 2 document whose `source` section is assembled from the given
/// pieces, so a single test can corrupt exactly one field.
QByteArray documentWithSource(
    const QByteArray& schemaVersion,
    const QByteArray& caseRelativePath,
    const QByteArray& files)
{
    QByteArray document = QByteArrayLiteral(R"JSON({
    "schemaVersion": )JSON");
    document += schemaVersion;
    document += QByteArrayLiteral(R"JSON(,
    "project": {"name": "sourced"},
    "domain": {"min": [0, 0, 0], "max": [1, 1, 1]},
    "mesh": {"cells": [4, 4, 4]},
    "solver": {
        "application": "icoFoam",
        "startTime": 0,
        "endTime": 1,
        "deltaT": 0.001,
        "writeInterval": 10
    },
    "source": {
        "caseRelativePath": )JSON");
    document += caseRelativePath;
    document += QByteArrayLiteral(R"JSON(,
        "startDirectory": "0",
        "importedAt": "2026-10-27T14:30:00Z",
        "files": )JSON");
    document += files;
    document += QByteArrayLiteral(R"JSON(
    }
})JSON");
    return document;
}

const QByteArray kZeroHash =
    QByteArrayLiteral("\"0000000000000000000000000000000000000000000000000000000000000000\"");

class ProjectJsonMigrationTest : public QObject
{
    Q_OBJECT

private slots:
    void legacyDocumentIsMigratedInMemory();
    void legacySourceSectionIsIgnored();
    void savingMigratedProjectWritesCurrentVersion();
    void emptyCaseRelativePathMeansNoSource();
    void shortHashIsRejected();
    void uppercaseHashIsRejected();
    void absoluteCaseRelativePathIsRejected();
    void absoluteFileRelativePathIsRejected();
    void duplicateSourceFileIsWarned();
    void missingSha256IsRejected();
};

void ProjectJsonMigrationTest::legacyDocumentIsMigratedInMemory()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("legacy.ofgproj"));
    const QByteArray document = legacyDocument();
    QVERIFY(writeBytes(path, document));

    const ProjectLoadResult loaded = ProjectJsonRepository::load(toPath(path));
    QVERIFY2(loaded.ok, loaded.diagnosticsText().c_str());
    QCOMPARE(loaded.schemaVersion, 1);
    QVERIFY2(loaded.migrated, "a schemaVersion 1 file must be reported as migrated");
    QVERIFY(hasCode(loaded.diagnostics, projectjson::kMigratedSchemaVersion));

    const ProjectJsonDiagnostic* info =
        firstWithCode(loaded.diagnostics, projectjson::kMigratedSchemaVersion);
    QVERIFY(info != nullptr);
    QCOMPARE(static_cast<int>(info->severity), static_cast<int>(ProjectJsonSeverity::Info));
    QVERIFY2(info->message.find("1") != std::string::npos, "the message must name the old version");
    QVERIFY2(info->message.find("2") != std::string::npos, "the message must name the new version");

    // The model itself is complete and carries no source section.
    QCOMPARE(QString::fromStdString(loaded.project.name()), QStringLiteral("legacy"));
    QVERIFY(!loaded.project.hasSource());
    QCOMPARE(QString::fromStdString(loaded.project.source().sourceRelativePath), QString());

    // A load never rewrites the file.
    QCOMPARE(readBytes(path), document);
}

void ProjectJsonMigrationTest::legacySourceSectionIsIgnored()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("legacySource.ofgproj"));

    // A v1 document with a `source` section cannot exist in practice, but a hand
    // edit could add one; version 1 semantics say the import origin is unknown.
    QByteArray document = documentWithSource(
        QByteArrayLiteral("1"),
        QByteArrayLiteral("\"../cavity\""),
        QByteArrayLiteral("[{\"relativePath\": \"0/p\", \"sha256\": ") + kZeroHash
            + QByteArrayLiteral("}]"));
    QVERIFY(writeBytes(path, document));

    const ProjectLoadResult loaded = ProjectJsonRepository::load(toPath(path));
    QVERIFY2(loaded.ok, loaded.diagnosticsText().c_str());
    QVERIFY(loaded.migrated);
    QVERIFY2(!loaded.project.hasSource(), "schemaVersion 1 documents have no source section");
    QCOMPARE(readBytes(path), document);
}

void ProjectJsonMigrationTest::savingMigratedProjectWritesCurrentVersion()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString legacyPath = directory.filePath(QStringLiteral("legacy.ofgproj"));
    QVERIFY(writeBytes(legacyPath, legacyDocument()));

    const ProjectLoadResult loaded = ProjectJsonRepository::load(toPath(legacyPath));
    QVERIFY2(loaded.ok, loaded.diagnosticsText().c_str());
    QVERIFY(loaded.migrated);

    const QString migratedPath = directory.filePath(QStringLiteral("migrated.ofgproj"));
    const ProjectSaveResult saved =
        ProjectJsonRepository::save(toPath(migratedPath), loaded.project, loaded.extras);
    QVERIFY2(saved.ok, saved.diagnosticsText().c_str());

    const QJsonObject root = parseObject(readBytes(migratedPath));
    QCOMPARE(root.value(projectjson::key(projectjson::kSchemaVersion)).toInt(), 2);
    QVERIFY(root.contains(projectjson::key(projectjson::kSource)));

    // Re-loading the migrated file is a plain schemaVersion 2 load: no migration.
    const ProjectLoadResult reloaded = ProjectJsonRepository::load(toPath(migratedPath));
    QVERIFY2(reloaded.ok, reloaded.diagnosticsText().c_str());
    QCOMPARE(reloaded.schemaVersion, 2);
    QVERIFY(!reloaded.migrated);
    QVERIFY(!hasCode(reloaded.diagnostics, projectjson::kMigratedSchemaVersion));
    QCOMPARE(QString::fromStdString(reloaded.project.name()), QStringLiteral("legacy"));
}

void ProjectJsonMigrationTest::emptyCaseRelativePathMeansNoSource()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("nosource.ofgproj"));

    // An empty `caseRelativePath` is how the specification spells "new project";
    // the `files` list of such a document carries no meaning.
    QByteArray document = documentWithSource(
        QByteArrayLiteral("2"),
        QByteArrayLiteral("\"\""),
        QByteArrayLiteral("[{\"relativePath\": \"0/p\", \"sha256\": ") + kZeroHash
            + QByteArrayLiteral("}]"));
    QVERIFY(writeBytes(path, document));

    const ProjectLoadResult loaded = ProjectJsonRepository::load(toPath(path));
    QVERIFY2(loaded.ok, loaded.diagnosticsText().c_str());
    QVERIFY(!loaded.migrated);
    QVERIFY(!loaded.project.hasSource());
    // The ignored file list is reported instead of disappearing silently.
    QVERIFY(hasCode(loaded.diagnostics, projectjson::kIgnoredSourceKeys));
    QCOMPARE(readBytes(path), document);
}

void ProjectJsonMigrationTest::shortHashIsRejected()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("shortHash.ofgproj"));

    QByteArray document = documentWithSource(
        QByteArrayLiteral("2"),
        QByteArrayLiteral("\"../cavity\""),
        QByteArrayLiteral("[{\"relativePath\": \"0/p\", \"sha256\": \"0123456789abcdef\"}]"));
    QVERIFY(writeBytes(path, document));

    const ProjectLoadResult loaded = ProjectJsonRepository::load(toPath(path));
    QVERIFY(!loaded.ok);
    const ProjectJsonDiagnostic* error =
        firstWithCode(loaded.diagnostics, projectjson::kInvalidSourceHash);
    QVERIFY2(error != nullptr, loaded.diagnosticsText().c_str());
    QVERIFY2(
        error->path.find("source.files[0].sha256") != std::string::npos,
        "the diagnostic must point at the offending hash");
    QCOMPARE(readBytes(path), document);
}

void ProjectJsonMigrationTest::uppercaseHashIsRejected()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("upperHash.ofgproj"));

    QByteArray document = documentWithSource(
        QByteArrayLiteral("2"),
        QByteArrayLiteral("\"../cavity\""),
        QByteArrayLiteral(
            "[{\"relativePath\": \"0/p\", \"sha256\": "
            "\"00000000000000000000000000000000000000000000000000000000000000FF\"}]"));
    QVERIFY(writeBytes(path, document));

    const ProjectLoadResult loaded = ProjectJsonRepository::load(toPath(path));
    QVERIFY(!loaded.ok);
    QVERIFY(hasCode(loaded.diagnostics, projectjson::kInvalidSourceHash));
    QCOMPARE(readBytes(path), document);
}

void ProjectJsonMigrationTest::absoluteCaseRelativePathIsRejected()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("absoluteCase.ofgproj"));

    QByteArray document = documentWithSource(
        QByteArrayLiteral("2"),
        QByteArrayLiteral("\"D:/6.developOpenFOAM/cavity\""),
        QByteArrayLiteral("[]"));
    QVERIFY(writeBytes(path, document));

    const ProjectLoadResult loaded = ProjectJsonRepository::load(toPath(path));
    QVERIFY(!loaded.ok);
    const ProjectJsonDiagnostic* error =
        firstWithCode(loaded.diagnostics, projectjson::kAbsoluteSourcePath);
    QVERIFY2(error != nullptr, loaded.diagnosticsText().c_str());
    QCOMPARE(static_cast<int>(error->severity), static_cast<int>(ProjectJsonSeverity::Error));
    QCOMPARE(readBytes(path), document);
}

void ProjectJsonMigrationTest::absoluteFileRelativePathIsRejected()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("absoluteFile.ofgproj"));

    QByteArray document = documentWithSource(
        QByteArrayLiteral("2"),
        QByteArrayLiteral("\"../cavity\""),
        QByteArrayLiteral("[{\"relativePath\": \"/etc/cavity/0/p\", \"sha256\": ") + kZeroHash
            + QByteArrayLiteral("}]"));
    QVERIFY(writeBytes(path, document));

    const ProjectLoadResult loaded = ProjectJsonRepository::load(toPath(path));
    QVERIFY(!loaded.ok);
    QVERIFY(hasCode(loaded.diagnostics, projectjson::kAbsoluteSourcePath));
    QCOMPARE(readBytes(path), document);
}

void ProjectJsonMigrationTest::duplicateSourceFileIsWarned()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("duplicate.ofgproj"));

    QByteArray document = documentWithSource(
        QByteArrayLiteral("2"),
        QByteArrayLiteral("\"../cavity\""),
        QByteArrayLiteral("[{\"relativePath\": \"0/p\", \"sha256\": ") + kZeroHash
            + QByteArrayLiteral("}, {\"relativePath\": \"0/p\", \"sha256\": ")
            + kZeroHash + QByteArrayLiteral("}]"));
    QVERIFY(writeBytes(path, document));

    const ProjectLoadResult loaded = ProjectJsonRepository::load(toPath(path));
    QVERIFY2(loaded.ok, loaded.diagnosticsText().c_str());
    const ProjectJsonDiagnostic* warning =
        firstWithCode(loaded.diagnostics, projectjson::kDuplicateSourceFile);
    QVERIFY2(warning != nullptr, loaded.diagnosticsText().c_str());
    QCOMPARE(static_cast<int>(warning->severity), static_cast<int>(ProjectJsonSeverity::Warning));
    QCOMPARE(static_cast<int>(loaded.project.source().fileCount()), 1);
    QCOMPARE(readBytes(path), document);
}

void ProjectJsonMigrationTest::missingSha256IsRejected()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("noHash.ofgproj"));

    QByteArray document = documentWithSource(
        QByteArrayLiteral("2"),
        QByteArrayLiteral("\"../cavity\""),
        QByteArrayLiteral("[{\"relativePath\": \"0/p\"}]"));
    QVERIFY(writeBytes(path, document));

    const ProjectLoadResult loaded = ProjectJsonRepository::load(toPath(path));
    QVERIFY(!loaded.ok);
    QVERIFY(hasCode(loaded.diagnostics, projectjson::kMissingField));
    QCOMPARE(readBytes(path), document);
}

} // namespace
} // namespace ofgui::io

QTEST_APPLESS_MAIN(ofgui::io::ProjectJsonMigrationTest)

#include "ProjectJsonMigrationTest.moc"
