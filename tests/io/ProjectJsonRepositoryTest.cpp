#include "io/ProjectJsonRepository.h"

#include "ProjectJsonRepositoryTestSupport.h"
#include "core/CaseProject.h"
#include "io/ProjectJsonDiagnostic.h"
#include "io/ProjectJsonExtras.h"
#include "io/ProjectJsonSchema.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
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

QJsonObject parseObject(const QByteArray& bytes)
{
    return QJsonDocument::fromJson(bytes).object();
}

class ProjectJsonRepositoryTest : public QObject
{
    Q_OBJECT

private slots:
    void saveAndLoadRoundTrip();
    void savedDocumentIsStableLfText();
    void saveReplacesExistingTarget();
    void unsetInternalFieldRoundTrips();
    void unknownFieldsSurviveResave();
};

void ProjectJsonRepositoryTest::saveAndLoadRoundTrip()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("case.ofgproj"));

    const core::CaseProject expected = projecttest::makeSampleProject();
    const ProjectSaveResult saved = ProjectJsonRepository::save(toPath(path), expected);
    QVERIFY2(saved.ok, saved.diagnosticsText().c_str());

    const ProjectLoadResult loaded = ProjectJsonRepository::load(toPath(path));
    QVERIFY2(loaded.ok, loaded.diagnosticsText().c_str());
    QCOMPARE(projecttest::describeDifference(loaded.project, expected), QString());
    QVERIFY(loaded.extras.isEmpty());

    // Round-tripping must be lossless: saving the loaded model again produces the
    // same bytes.
    std::vector<ProjectJsonDiagnostic> diagnostics;
    const std::string first = ProjectJsonRepository::toJsonDocument(expected, {}, diagnostics);
    const std::string second = ProjectJsonRepository::toJsonDocument(
        loaded.project, loaded.extras, diagnostics);
    QCOMPARE(QString::fromStdString(second), QString::fromStdString(first));
    QCOMPARE(readBytes(path), QByteArray(first.data(), static_cast<qsizetype>(first.size())));
}

void ProjectJsonRepositoryTest::savedDocumentIsStableLfText()
{
    const core::CaseProject project = projecttest::makeSampleProject();
    std::vector<ProjectJsonDiagnostic> diagnostics;
    const std::string text = ProjectJsonRepository::toJsonDocument(project, {}, diagnostics);

    QVERIFY(diagnostics.empty());
    QVERIFY2(!text.empty(), "the document must not be empty");
    QVERIFY(text.front() == '{');
    QVERIFY(text.back() == '\n');
    QCOMPARE(text.find('\r'), std::string::npos);
    QVERIFY(text.find("    \"schemaVersion\": 2,\n") != std::string::npos);
    QVERIFY(text.find("    \"source\": {\n") != std::string::npos);
    QVERIFY(text.find("\"application\": \"icoFoam\",\n") != std::string::npos);
    QVERIFY(text.find("\"kind\": \"unset\"") != std::string::npos);
    QVERIFY(text.find("uniform (1 0 0)") != std::string::npos);

    const QJsonObject object = parseObject(QByteArray(text.data(), static_cast<qsizetype>(text.size())));
    QCOMPARE(object.value(projectjson::key(projectjson::kSchemaVersion)).toInt(), 2);
}

void ProjectJsonRepositoryTest::saveReplacesExistingTarget()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("case.ofgproj"));

    core::CaseProject project = projecttest::makeSampleProject();
    QVERIFY(ProjectJsonRepository::save(toPath(path), project).ok);

    project.setName("renamedCase");
    QVERIFY(ProjectJsonRepository::save(toPath(path), project).ok);

    const ProjectLoadResult loaded = ProjectJsonRepository::load(toPath(path));
    QVERIFY2(loaded.ok, loaded.diagnosticsText().c_str());
    QCOMPARE(QString::fromStdString(loaded.project.name()), QStringLiteral("renamedCase"));

    // The temporary file must be renamed away, never left behind.
    const QStringList entries = QDir(directory.path()).entryList(QDir::Files | QDir::Hidden);
    QCOMPARE(entries, QStringList{QStringLiteral("case.ofgproj")});
}

void ProjectJsonRepositoryTest::unsetInternalFieldRoundTrips()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("case.ofgproj"));

    const core::CaseProject expected = projecttest::makeSampleProject();
    QVERIFY(ProjectJsonRepository::save(toPath(path), expected).ok);

    const ProjectLoadResult loaded = ProjectJsonRepository::load(toPath(path));
    QVERIFY2(loaded.ok, loaded.diagnosticsText().c_str());
    const auto* temperature = loaded.project.findFieldByName("T");
    QVERIFY(temperature != nullptr);
    QVERIFY(!temperature->internalField().isValid());
    QCOMPARE(projecttest::describeDifference(loaded.project, expected), QString());
}

void ProjectJsonRepositoryTest::unknownFieldsSurviveResave()
{
    // A document written by a hypothetical newer build: every level carries keys
    // this build does not model. They must come back unchanged after a load+save.
    static const QByteArray document = R"JSON({
    "schemaVersion": 1,
    "owner": "lab",
    "project": {
        "name": "legacy",
        "caseDirectory": "cases/legacy",
        "createdBy": "future-build"
    },
    "domain": {
        "min": [0, 0, 0],
        "max": [1, 1, 0.1],
        "mode": "twoD",
        "unit": "mm",
        "axes": "xyz"
    },
    "mesh": {
        "cells": [10, 12, 1],
        "grading": [1, 1, 1],
        "scale": 0.5,
        "boundaryLayers": {"layers": 3}
    },
    "solver": {
        "application": "icoFoam",
        "startTime": 0,
        "endTime": 2,
        "deltaT": 0.001,
        "writeInterval": 100,
        "residualControl": 0.0001
    },
    "patches": [
        {
            "id": "0000000000000001",
            "name": "inlet",
            "type": "patch",
            "faces": ["xMin"],
            "color": [0.1, 0.2, 0.3],
            "visible": true,
            "notes": "keep me"
        }
    ],
    "fields": [
        {
            "id": "0000000000000002",
            "name": "U",
            "class": "volVectorField",
            "dimensions": [0, 1, -1, 0, 0, 0, 0],
            "internalField": {"kind": "unset"},
            "displayUnits": "m/s",
            "boundaryField": [
                {
                    "patchId": "0000000000000001",
                    "type": "fixedValue",
                    "parameters": {"value": "uniform (1 0 0)"},
                    "conditionNote": "keep me too"
                }
            ]
        }
    ]
})JSON";

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString sourcePath = directory.filePath(QStringLiteral("legacy.ofgproj"));
    QVERIFY(writeBytes(sourcePath, document));

    const ProjectLoadResult loaded = ProjectJsonRepository::load(toPath(sourcePath));
    QVERIFY2(loaded.ok, loaded.diagnosticsText().c_str());
    QVERIFY(!loaded.extras.isEmpty());
    QVERIFY(hasCode(loaded.diagnostics, projectjson::kPreservedUnknownFields));

    const QByteArray original = readBytes(sourcePath);
    QCOMPARE(readBytes(sourcePath), original);  // loading never rewrites the file

    const QString savedPath = directory.filePath(QStringLiteral("resaved.ofgproj"));
    const ProjectSaveResult saved =
        ProjectJsonRepository::save(toPath(savedPath), loaded.project, loaded.extras);
    QVERIFY2(saved.ok, saved.diagnosticsText().c_str());

    const QJsonObject root = parseObject(readBytes(savedPath));
    QCOMPARE(root.value(projectjson::key("owner")).toString(), QStringLiteral("lab"));

    const QJsonObject project = root.value(projectjson::key(projectjson::kProject)).toObject();
    QCOMPARE(project.value(projectjson::key("createdBy")).toString(),
             QStringLiteral("future-build"));
    QCOMPARE(project.value(projectjson::key(projectjson::kName)).toString(),
             QStringLiteral("legacy"));

    const QJsonObject mesh = root.value(projectjson::key(projectjson::kMesh)).toObject();
    QVERIFY(mesh.contains(projectjson::key("boundaryLayers")));

    const QJsonArray patches = root.value(projectjson::key(projectjson::kPatches)).toArray();
    QCOMPARE(patches.size(), 1);
    QCOMPARE(patches.at(0).toObject().value(projectjson::key("notes")).toString(),
             QStringLiteral("keep me"));

    const QJsonArray fields = root.value(projectjson::key(projectjson::kFields)).toArray();
    QCOMPARE(fields.size(), 1);
    QCOMPARE(fields.at(0).toObject().value(projectjson::key("displayUnits")).toString(),
             QStringLiteral("m/s"));
    const QJsonArray conditions =
        fields.at(0).toObject().value(projectjson::key(projectjson::kBoundaryField)).toArray();
    QCOMPARE(conditions.size(), 1);
    QCOMPARE(conditions.at(0).toObject().value(projectjson::key("conditionNote")).toString(),
             QStringLiteral("keep me too"));

    // The resaved document must still load back into the same model.
    const ProjectLoadResult reloaded = ProjectJsonRepository::load(toPath(savedPath));
    QVERIFY2(reloaded.ok, reloaded.diagnosticsText().c_str());
    QCOMPARE(reloaded.project.patches().size(), 1);
    QCOMPARE(reloaded.project.fields().size(), 1);
}

} // namespace
} // namespace ofgui::io

QTEST_APPLESS_MAIN(ofgui::io::ProjectJsonRepositoryTest)

#include "ProjectJsonRepositoryTest.moc"
