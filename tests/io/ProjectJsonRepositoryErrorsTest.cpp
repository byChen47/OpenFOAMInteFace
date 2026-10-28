#include "io/ProjectJsonRepository.h"

#include "core/CaseProject.h"
#include "io/ProjectJsonDiagnostic.h"
#include "io/ProjectJsonSchema.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
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

bool writeBytes(const QString& path, const QByteArray& bytes)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        return false;
    }
    return file.write(bytes) == bytes.size();
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

std::size_t countCode(const std::vector<ProjectJsonDiagnostic>& diagnostics, const char* code)
{
    std::size_t count = 0;
    for (const ProjectJsonDiagnostic& diagnostic : diagnostics)
    {
        if (diagnostic.code == code)
        {
            ++count;
        }
    }
    return count;
}

/// Smallest document schema version 1 accepts: only the model fields that carry
/// no default are present.
QByteArray minimalDocument()
{
    return QByteArrayLiteral(R"JSON({
    "schemaVersion": 1,
    "project": {"name": "minimal"},
    "domain": {"min": [0, 0, 0], "max": [1, 1, 1]},
    "mesh": {"cells": [4, 4, 4]},
    "solver": {
        "application": "icoFoam",
        "startTime": 0,
        "endTime": 1,
        "deltaT": 0.001,
        "writeInterval": 10
    }
})JSON");
}

class ProjectJsonRepositoryErrorsTest : public QObject
{
    Q_OBJECT

private slots:
    void missingFileIsRejected();
    void directoryIsRejected();
    void malformedJsonIsRejected();
    void rootArrayIsRejected();
    void missingSchemaVersionIsRejected();
    void unsupportedSchemaVersionIsRejected();
    void invalidSchemaVersionIsRejected();
    void invalidObjectIdIsRejected();
    void duplicateObjectIdIsRejected();
    void unknownEnumWordsFallBack();
    void wrongArraySizeIsRejected();
    void absoluteCaseDirectoryIsWarned();
    void saveToDirectoryIsRejected();
    void utf8BomIsAccepted();
    void failedLoadKeepsModelEmpty();
};

void ProjectJsonRepositoryErrorsTest::missingFileIsRejected()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("absent.ofgproj"));

    const ProjectLoadResult loaded = ProjectJsonRepository::load(toPath(path));
    QVERIFY(!loaded.ok);
    QVERIFY(hasCode(loaded.diagnostics, projectjson::kCannotOpen));
    QVERIFY(!QFileInfo::exists(path));
}

void ProjectJsonRepositoryErrorsTest::directoryIsRejected()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const ProjectLoadResult loaded = ProjectJsonRepository::load(toPath(directory.path()));
    QVERIFY(!loaded.ok);
    QVERIFY(hasCode(loaded.diagnostics, projectjson::kCannotOpen));
}

void ProjectJsonRepositoryErrorsTest::malformedJsonIsRejected()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("broken.ofgproj"));
    const QByteArray broken = QByteArrayLiteral("{ \"schemaVersion\": 1, oops }");
    QVERIFY(writeBytes(path, broken));

    const ProjectLoadResult loaded = ProjectJsonRepository::load(toPath(path));
    QVERIFY(!loaded.ok);
    QVERIFY(hasCode(loaded.diagnostics, projectjson::kMalformedJson));
    QVERIFY2(
        loaded.diagnosticsText().find("byte offset") != std::string::npos,
        "the parse error must report the byte offset");
    QCOMPARE(readBytes(path), broken);  // a rejected file is never touched
}

void ProjectJsonRepositoryErrorsTest::rootArrayIsRejected()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("array.ofgproj"));
    QVERIFY(writeBytes(path, QByteArrayLiteral("[1, 2, 3]")));

    const ProjectLoadResult loaded = ProjectJsonRepository::load(toPath(path));
    QVERIFY(!loaded.ok);
    QVERIFY(hasCode(loaded.diagnostics, projectjson::kRootNotObject));
}

void ProjectJsonRepositoryErrorsTest::missingSchemaVersionIsRejected()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("noversion.ofgproj"));
    QVERIFY(writeBytes(path, QByteArrayLiteral("{\"project\": {\"name\": \"x\"}}")));

    const ProjectLoadResult loaded = ProjectJsonRepository::load(toPath(path));
    QVERIFY(!loaded.ok);
    QVERIFY(hasCode(loaded.diagnostics, projectjson::kMissingSchemaVersion));
}

void ProjectJsonRepositoryErrorsTest::unsupportedSchemaVersionIsRejected()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("future.ofgproj"));

    QByteArray future = minimalDocument();
    future.replace(QByteArrayLiteral("\"schemaVersion\": 1"), QByteArrayLiteral("\"schemaVersion\": 3"));
    QVERIFY(writeBytes(path, future));

    const ProjectLoadResult loaded = ProjectJsonRepository::load(toPath(path));
    QVERIFY(!loaded.ok);
    QVERIFY(hasCode(loaded.diagnostics, projectjson::kUnsupportedSchemaVersion));
    QVERIFY2(
        loaded.diagnosticsText().find("left unchanged") != std::string::npos,
        "the message must state that the file was not modified");
    QCOMPARE(readBytes(path), future);
}

void ProjectJsonRepositoryErrorsTest::invalidSchemaVersionIsRejected()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("stringversion.ofgproj"));

    QByteArray document = minimalDocument();
    document.replace(QByteArrayLiteral("\"schemaVersion\": 1"), QByteArrayLiteral("\"schemaVersion\": \"1\""));
    QVERIFY(writeBytes(path, document));

    const ProjectLoadResult loaded = ProjectJsonRepository::load(toPath(path));
    QVERIFY(!loaded.ok);
    QVERIFY(hasCode(loaded.diagnostics, projectjson::kInvalidSchemaVersion));
}

void ProjectJsonRepositoryErrorsTest::invalidObjectIdIsRejected()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("badid.ofgproj"));

    static const QByteArray document = QByteArrayLiteral(R"JSON({
    "schemaVersion": 1,
    "project": {"name": "badid"},
    "domain": {"min": [0, 0, 0], "max": [1, 1, 1]},
    "mesh": {"cells": [4, 4, 4]},
    "solver": {"application": "icoFoam", "startTime": 0, "endTime": 1, "deltaT": 0.1, "writeInterval": 1},
    "patches": [{"id": "not-an-id", "name": "walls", "type": "wall"}]
})JSON");
    QVERIFY(writeBytes(path, document));

    const ProjectLoadResult loaded = ProjectJsonRepository::load(toPath(path));
    QVERIFY(!loaded.ok);
    QVERIFY(hasCode(loaded.diagnostics, projectjson::kInvalidValue));
    QVERIFY2(
        loaded.diagnosticsText().find("patches[0].id") != std::string::npos,
        "the diagnostic must point at the offending JSON path");
}

void ProjectJsonRepositoryErrorsTest::duplicateObjectIdIsRejected()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("duplicate.ofgproj"));

    static const QByteArray document = QByteArrayLiteral(R"JSON({
    "schemaVersion": 1,
    "project": {"name": "duplicate"},
    "domain": {"min": [0, 0, 0], "max": [1, 1, 1]},
    "mesh": {"cells": [4, 4, 4]},
    "solver": {"application": "icoFoam", "startTime": 0, "endTime": 1, "deltaT": 0.1, "writeInterval": 1},
    "patches": [
        {"id": "0000000000000001", "name": "first", "type": "wall"},
        {"id": "0000000000000001", "name": "second", "type": "wall"}
    ]
})JSON");
    QVERIFY(writeBytes(path, document));

    const ProjectLoadResult loaded = ProjectJsonRepository::load(toPath(path));
    QVERIFY(!loaded.ok);
    QVERIFY(hasCode(loaded.diagnostics, projectjson::kDuplicateId));
}

void ProjectJsonRepositoryErrorsTest::unknownEnumWordsFallBack()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("enums.ofgproj"));

    static const QByteArray document = QByteArrayLiteral(R"JSON({
    "schemaVersion": 1,
    "project": {"name": "enums"},
    "domain": {"min": [0, 0, 0], "max": [1, 1, 1], "mode": "fourD"},
    "mesh": {"cells": [4, 4, 4]},
    "solver": {"application": "icoFoam", "startTime": 0, "endTime": 1, "deltaT": 0.1, "writeInterval": 1},
    "patches": [{"id": "0000000000000001", "name": "walls", "type": "absorbing"}],
    "fields": [{
        "id": "0000000000000002",
        "name": "k",
        "class": "volTensorField",
        "dimensions": [0, 2, -2, 0, 0, 0, 0],
        "internalField": {"kind": "unset"}
    }]
})JSON");
    QVERIFY(writeBytes(path, document));

    const ProjectLoadResult loaded = ProjectJsonRepository::load(toPath(path));
    QVERIFY2(loaded.ok, loaded.diagnosticsText().c_str());
    QCOMPARE(static_cast<int>(countCode(loaded.diagnostics, projectjson::kUnknownEnumWord)), 3);
    QVERIFY(loaded.project.domain().mode() == core::DomainMode::ThreeD);
    QCOMPARE(static_cast<int>(loaded.project.patches().size()), 1);
    QVERIFY(loaded.project.patches().front().type() == core::PatchType::Patch);
    QCOMPARE(static_cast<int>(loaded.project.fields().size()), 1);
    QVERIFY(loaded.project.fields().front().fieldClass() == core::FieldClass::VolScalarField);
}

void ProjectJsonRepositoryErrorsTest::wrongArraySizeIsRejected()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("badarray.ofgproj"));

    QByteArray document = minimalDocument();
    document.replace(QByteArrayLiteral("\"cells\": [4, 4, 4]"), QByteArrayLiteral("\"cells\": [4, 4]"));
    QVERIFY(writeBytes(path, document));

    const ProjectLoadResult loaded = ProjectJsonRepository::load(toPath(path));
    QVERIFY(!loaded.ok);
    QVERIFY(hasCode(loaded.diagnostics, projectjson::kInvalidValue));
    QVERIFY2(
        loaded.diagnosticsText().find("mesh.cells") != std::string::npos,
        "the diagnostic must name the offending key");
    QVERIFY(loaded.project.patches().empty());
}

void ProjectJsonRepositoryErrorsTest::absoluteCaseDirectoryIsWarned()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("absolute.ofgproj"));

    core::CaseProject project;
    project.setName("absolute");
    project.setCaseDirectory("D:/cases/absolute");
    project.domain().setMax(core::Vec3d(1.0, 1.0, 1.0));

    const ProjectSaveResult saved = ProjectJsonRepository::save(toPath(path), project);
    QVERIFY2(saved.ok, saved.diagnosticsText().c_str());
    QVERIFY(hasCode(saved.diagnostics, projectjson::kAbsoluteCaseDirectory));

    // The value is preserved verbatim; the warning is advisory only.
    const ProjectLoadResult loaded = ProjectJsonRepository::load(toPath(path));
    QVERIFY2(loaded.ok, loaded.diagnosticsText().c_str());
    QCOMPARE(QString::fromStdString(loaded.project.caseDirectory()),
             QStringLiteral("D:/cases/absolute"));
}

void ProjectJsonRepositoryErrorsTest::saveToDirectoryIsRejected()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString subdirectory = directory.filePath(QStringLiteral("target"));
    QVERIFY(QDir(directory.path()).mkdir(QStringLiteral("target")));

    const ProjectSaveResult saved =
        ProjectJsonRepository::save(toPath(subdirectory), core::CaseProject{});
    QVERIFY(!saved.ok);
    QVERIFY(hasCode(saved.diagnostics, projectjson::kTargetNotWritable));

    // Nothing may be created next to (or inside) the rejected target.
    QVERIFY(QDir(directory.path()).entryList(QDir::Files | QDir::Hidden).isEmpty());
    QVERIFY(QDir(subdirectory).entryList(QDir::Files | QDir::Hidden).isEmpty());
}

void ProjectJsonRepositoryErrorsTest::utf8BomIsAccepted()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("bom.ofgproj"));

    QByteArray document = minimalDocument();
    document.prepend(QByteArrayLiteral("\xEF\xBB\xBF"));
    QVERIFY(writeBytes(path, document));

    const ProjectLoadResult loaded = ProjectJsonRepository::load(toPath(path));
    QVERIFY2(loaded.ok, loaded.diagnosticsText().c_str());
    QVERIFY(hasCode(loaded.diagnostics, projectjson::kBomRemoved));
    QCOMPARE(QString::fromStdString(loaded.project.name()), QStringLiteral("minimal"));
}

void ProjectJsonRepositoryErrorsTest::failedLoadKeepsModelEmpty()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("halfbad.ofgproj"));

    // `project` is valid, but the patch list is broken: the load must fail and
    // must not expose the partially populated model.
    static const QByteArray document = QByteArrayLiteral(R"JSON({
    "schemaVersion": 1,
    "project": {"name": "halfvalid"},
    "domain": {"min": [0, 0, 0], "max": [1, 1, 1]},
    "mesh": {"cells": [4, 4, 4]},
    "solver": {"application": "icoFoam", "startTime": 0, "endTime": 1, "deltaT": 0.1, "writeInterval": 1},
    "patches": [{"id": "0000000000000000000000000000000001", "name": "walls", "type": "wall"}]
})JSON");
    QVERIFY(writeBytes(path, document));

    const ProjectLoadResult loaded = ProjectJsonRepository::load(toPath(path));
    QVERIFY(!loaded.ok);
    QVERIFY(loaded.project.name().empty());
    QVERIFY(loaded.project.patches().empty());
}

} // namespace
} // namespace ofgui::io

QTEST_APPLESS_MAIN(ofgui::io::ProjectJsonRepositoryErrorsTest)

#include "ProjectJsonRepositoryErrorsTest.moc"
