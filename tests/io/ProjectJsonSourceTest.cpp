#include "core/CaseProject.h"
#include "core/CaseSourceInfo.h"
#include "core/DomainBox.h"
#include "core/MeshSettings.h"
#include "core/Vec3d.h"
#include "io/ProjectJsonDiagnostic.h"
#include "io/ProjectJsonRepository.h"
#include "io/ProjectJsonSchema.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QTemporaryDir>
#include <QtTest>

#include <filesystem>
#include <string>
#include <vector>

namespace ofgui::io {
namespace {

/// Lower-case hex SHA-256 used by the canned source records below.
std::string hexOf(unsigned char fill)
{
    return std::string(core::kSha256HexLength, static_cast<char>(fill));
}

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

/// Project with a fully populated source section (two files, distinct hashes).
core::CaseProject projectWithSource()
{
    core::CaseProject project;
    project.setName("importedCavity");
    project.setCaseDirectory("cases/cavity");
    project.domain().setMax(core::Vec3d(0.1, 0.1, 0.01));
    project.mesh().setCells(core::MeshSettings::CellCounts{20, 20, 1});

    core::CaseSourceInfo source;
    source.sourceRelativePath = "../cavity";
    source.startDirectory = "0";
    source.importedAt = "2026-10-27T14:30:00Z";
    source.files.push_back(core::SourceFileHash{"0/p", {}});
    source.files.push_back(core::SourceFileHash{"0/U", {}});
    source.files.push_back(core::SourceFileHash{"system/controlDict", {}});
    project.setSource(source);
    project.source().setFileHash("0/p", hexOf('0'));
    project.source().setFileHash("0/U", hexOf('f'));
    project.source().setFileHash("system/controlDict", "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
    return project;
}

/// @return empty string when the two source sections are equal.
QString describeSourceDifference(const core::CaseSourceInfo& loaded, const core::CaseSourceInfo& expected)
{
    const auto text = [](const std::string& value) { return QString::fromStdString(value); };

    if (text(loaded.sourceRelativePath) != text(expected.sourceRelativePath))
    {
        return QStringLiteral("sourceRelativePath: '%1' != '%2'")
            .arg(text(loaded.sourceRelativePath), text(expected.sourceRelativePath));
    }
    if (text(loaded.startDirectory) != text(expected.startDirectory))
    {
        return QStringLiteral("startDirectory: '%1' != '%2'")
            .arg(text(loaded.startDirectory), text(expected.startDirectory));
    }
    if (text(loaded.importedAt) != text(expected.importedAt))
    {
        return QStringLiteral("importedAt: '%1' != '%2'")
            .arg(text(loaded.importedAt), text(expected.importedAt));
    }
    if (loaded.files.size() != expected.files.size())
    {
        return QStringLiteral("source file count: %1 != %2")
            .arg(loaded.files.size())
            .arg(expected.files.size());
    }
    for (std::size_t index = 0; index < expected.files.size(); ++index)
    {
        const core::SourceFileHash& actual = loaded.files[index];
        const core::SourceFileHash& wanted = expected.files[index];
        if (text(actual.relativePath) != text(wanted.relativePath)
            || text(actual.sha256Hex()) != text(wanted.sha256Hex()))
        {
            return QStringLiteral("source file %1 ('%2') differs")
                .arg(index)
                .arg(text(wanted.relativePath));
        }
    }
    return {};
}

class ProjectJsonSourceTest : public QObject
{
    Q_OBJECT

private slots:
    void sourceRoundTrip();
    void sourceSectionIsAlwaysWritten();
    void secondSaveIsByteIdentical();
    void unknownSourceKeysSurvive();
    void absoluteSourcePathIsWarnedOnSave();
    void duplicateSourceFileIsWarnedOnSave();
    void hashesAreWrittenAsLowercaseHex();
};

void ProjectJsonSourceTest::sourceRoundTrip()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("cavity.ofgproj"));

    const core::CaseProject expected = projectWithSource();
    const ProjectSaveResult saved = ProjectJsonRepository::save(toPath(path), expected);
    QVERIFY2(saved.ok, saved.diagnosticsText().c_str());
    QVERIFY(!hasCode(saved.diagnostics, projectjson::kAbsoluteSourcePath));

    const ProjectLoadResult loaded = ProjectJsonRepository::load(toPath(path));
    QVERIFY2(loaded.ok, loaded.diagnosticsText().c_str());
    QCOMPARE(loaded.schemaVersion, projectjson::kCurrentProjectSchemaVersion);
    QVERIFY(!loaded.migrated);
    QVERIFY(loaded.project.hasSource());
    QCOMPARE(describeSourceDifference(loaded.project.source(), expected.source()), QString());
    QVERIFY(loaded.extras.isEmpty());
}

void ProjectJsonSourceTest::sourceSectionIsAlwaysWritten()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("new.ofgproj"));

    core::CaseProject project;  // no source: a brand new project
    project.setName("fresh");
    QVERIFY(ProjectJsonRepository::save(toPath(path), project).ok);

    const QJsonObject root = parseObject(readBytes(path));
    QVERIFY2(
        root.contains(projectjson::key(projectjson::kSource)),
        "the source section must always be present in a schemaVersion 2 document");
    const QJsonObject source = root.value(projectjson::key(projectjson::kSource)).toObject();
    QCOMPARE(source.value(projectjson::key(projectjson::kCaseRelativePath)).toString(), QString());
    QCOMPARE(source.value(projectjson::key(projectjson::kStartDirectory)).toString(), QString());
    QCOMPARE(source.value(projectjson::key(projectjson::kImportedAt)).toString(), QString());
    QVERIFY(source.value(projectjson::key(projectjson::kFiles)).toArray().isEmpty());

    const ProjectLoadResult loaded = ProjectJsonRepository::load(toPath(path));
    QVERIFY2(loaded.ok, loaded.diagnosticsText().c_str());
    QVERIFY(!loaded.project.hasSource());
}

void ProjectJsonSourceTest::secondSaveIsByteIdentical()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString first = directory.filePath(QStringLiteral("first.ofgproj"));
    const QString second = directory.filePath(QStringLiteral("second.ofgproj"));

    const core::CaseProject project = projectWithSource();
    QVERIFY(ProjectJsonRepository::save(toPath(first), project).ok);

    const ProjectLoadResult loaded = ProjectJsonRepository::load(toPath(first));
    QVERIFY2(loaded.ok, loaded.diagnosticsText().c_str());
    QVERIFY(ProjectJsonRepository::save(toPath(second), loaded.project, loaded.extras).ok);

    QCOMPARE(readBytes(second), readBytes(first));
}

void ProjectJsonSourceTest::unknownSourceKeysSurvive()
{
    // A document from a hypothetical newer build: unknown keys inside `source`
    // and inside a file entry must survive load + save unchanged.
    static const QByteArray document = QByteArrayLiteral(R"JSON({
    "schemaVersion": 2,
    "project": {"name": "future"},
    "domain": {"min": [0, 0, 0], "max": [1, 1, 1]},
    "mesh": {"cells": [4, 4, 4]},
    "solver": {"application": "icoFoam", "startTime": 0, "endTime": 1, "deltaT": 0.1, "writeInterval": 1},
    "source": {
        "caseRelativePath": "../cavity",
        "startDirectory": "0",
        "importedAt": "2026-10-27T14:30:00Z",
        "importedBy": "future-build",
        "files": [
            {
                "relativePath": "0/p",
                "sha256": "0000000000000000000000000000000000000000000000000000000000000000",
                "byteSize": 1104
            }
        ]
    }
})JSON");

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("future.ofgproj"));
    QVERIFY(writeBytes(path, document));

    const ProjectLoadResult loaded = ProjectJsonRepository::load(toPath(path));
    QVERIFY2(loaded.ok, loaded.diagnosticsText().c_str());
    QCOMPARE(QString::fromStdString(loaded.project.source().sourceRelativePath),
             QStringLiteral("../cavity"));
    QCOMPARE(static_cast<int>(loaded.project.source().fileCount()), 1);
    QVERIFY(hasCode(loaded.diagnostics, projectjson::kPreservedUnknownFields));

    const QString resaved = directory.filePath(QStringLiteral("resaved.ofgproj"));
    QVERIFY(ProjectJsonRepository::save(toPath(resaved), loaded.project, loaded.extras).ok);

    const QJsonObject root = parseObject(readBytes(resaved));
    const QJsonObject source = root.value(projectjson::key(projectjson::kSource)).toObject();
    QCOMPARE(source.value(projectjson::key("importedBy")).toString(), QStringLiteral("future-build"));
    const QJsonArray files = source.value(projectjson::key(projectjson::kFiles)).toArray();
    QCOMPARE(files.size(), 1);
    QCOMPARE(files.at(0).toObject().value(projectjson::key("byteSize")).toInt(), 1104);
}

void ProjectJsonSourceTest::absoluteSourcePathIsWarnedOnSave()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("absolute.ofgproj"));

    core::CaseProject project = projectWithSource();
    project.source().sourceRelativePath = "D:/cases/cavity";

    const ProjectSaveResult saved = ProjectJsonRepository::save(toPath(path), project);
    QVERIFY2(saved.ok, saved.diagnosticsText().c_str());
    QVERIFY(hasCode(saved.diagnostics, projectjson::kAbsoluteSourcePath));

    // Preserved verbatim (never silently dropped), but a reader rejects it: the
    // specification forbids absolute paths in a project file (D-025).
    const QJsonObject root = parseObject(readBytes(path));
    QCOMPARE(root.value(projectjson::key(projectjson::kSource))
                 .toObject()
                 .value(projectjson::key(projectjson::kCaseRelativePath))
                 .toString(),
             QStringLiteral("D:/cases/cavity"));

    const ProjectLoadResult loaded = ProjectJsonRepository::load(toPath(path));
    QVERIFY(!loaded.ok);
    QVERIFY(hasCode(loaded.diagnostics, projectjson::kAbsoluteSourcePath));
}

void ProjectJsonSourceTest::duplicateSourceFileIsWarnedOnSave()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("duplicate.ofgproj"));

    core::CaseProject project = projectWithSource();
    project.source().files.push_back(
        core::SourceFileHash{"0/p", {}});  // same path as the first record

    const ProjectSaveResult saved = ProjectJsonRepository::save(toPath(path), project);
    QVERIFY2(saved.ok, saved.diagnosticsText().c_str());
    QVERIFY(hasCode(saved.diagnostics, projectjson::kDuplicateSourceFile));

    const QJsonObject root = parseObject(readBytes(path));
    const QJsonArray files = root.value(projectjson::key(projectjson::kSource))
                                 .toObject()
                                 .value(projectjson::key(projectjson::kFiles))
                                 .toArray();
    QCOMPARE(files.size(), 3);  // the duplicate was dropped, not written twice
}

void ProjectJsonSourceTest::hashesAreWrittenAsLowercaseHex()
{
    const core::CaseProject project = projectWithSource();
    std::vector<ProjectJsonDiagnostic> diagnostics;
    const std::string text = ProjectJsonRepository::toJsonDocument(project, {}, diagnostics);

    QVERIFY(diagnostics.empty());
    QVERIFY(text.find("\"sha256\": \"0000000000000000000000000000000000000000000000000000000000000000\"")
            != std::string::npos);
    QVERIFY(text.find("\"sha256\": \"FFFFFFFF") == std::string::npos);
    QVERIFY(text.find("\"caseRelativePath\": \"../cavity\"") != std::string::npos);
    QVERIFY(text.find("\"importedAt\": \"2026-10-27T14:30:00Z\"") != std::string::npos);
}

} // namespace
} // namespace ofgui::io

QTEST_APPLESS_MAIN(ofgui::io::ProjectJsonSourceTest)

#include "ProjectJsonSourceTest.moc"
