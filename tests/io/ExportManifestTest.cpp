#include "io/writers/CaseExportPlan.h"
#include "io/writers/ExportManifest.h"
#include "io/writers/ExportManifestSchema.h"

#include <QtTest>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

#include <string>
#include <vector>

namespace ofgui::io {
namespace {

CaseExportPlan samplePlan()
{
    CaseExportPlan plan;

    ExportFile written;
    written.relativePath = "system/controlDict";
    written.text = "controlDict text\n";
    written.action = ExportAction::Written;
    written.byteSize = written.text.size();
    plan.files.push_back(written);

    ExportFile copied;
    copied.relativePath = "0/T";
    copied.sourcePath = std::filesystem::path{"source/0/T"};
    copied.action = ExportAction::Copied;
    copied.byteSize = 120;
    plan.files.push_back(copied);

    ExportFile mirrored;
    mirrored.relativePath = "constant/polyMesh/points";
    mirrored.sourcePath = std::filesystem::path{"source/constant/polyMesh/points"};
    mirrored.action = ExportAction::Mirrored;
    mirrored.byteSize = 4096;
    plan.files.push_back(mirrored);

    plan.skipped.push_back(ExportSkippedFile{"log.blockMesh", "运行时产物"});
    plan.skipped.push_back(ExportSkippedFile{"Allrun", "运行时产物"});

    return plan;
}

QJsonObject manifestObject(const CaseExportPlan& plan)
{
    const std::string text = ExportManifest::build(plan, "2026-10-27T14:30:00Z");
    QJsonParseError error{};
    const QJsonDocument document = QJsonDocument::fromJson(QByteArray::fromStdString(text), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject())
    {
        return QJsonObject{};
    }
    return document.object();
}

} // namespace

class ExportManifestTest : public QObject
{
    Q_OBJECT

private slots:
    void manifestIsValidJsonWithVersionAndTimestamp();
    void manifestListsEveryFileSortedWithActionAndBytes();
    void manifestListsSkippedSourcesWithReason();
    void manifestCountsMatchThePlan();
    void manifestUsesLfAndEndsWithNewline();
    void manifestNeverListsItself();
};

void ExportManifestTest::manifestIsValidJsonWithVersionAndTimestamp()
{
    const QJsonObject root = manifestObject(samplePlan());
    QVERIFY(!root.isEmpty());

    QCOMPARE(root.value(QLatin1String{exportmanifest::kSchemaVersionKey}).toInt(),
             exportmanifest::kSchemaVersion);
    QCOMPARE(root.value(QLatin1String{exportmanifest::kGeneratedAtKey}).toString(),
             QStringLiteral("2026-10-27T14:30:00Z"));
}

void ExportManifestTest::manifestListsEveryFileSortedWithActionAndBytes()
{
    const QJsonObject root = manifestObject(samplePlan());

    const QJsonArray files = root.value(QLatin1String{exportmanifest::kFilesKey}).toArray();
    QCOMPARE(files.size(), 3);

    std::vector<QString> paths;
    for (const QJsonValue& value : files)
    {
        paths.push_back(value.toObject().value(QLatin1String{exportmanifest::kRelativePathKey}).toString());
    }
    QCOMPARE(paths.at(0), QStringLiteral("0/T"));
    QCOMPARE(paths.at(1), QStringLiteral("constant/polyMesh/points"));
    QCOMPARE(paths.at(2), QStringLiteral("system/controlDict"));

    const QJsonObject copied = files.at(0).toObject();
    QCOMPARE(copied.value(QLatin1String{exportmanifest::kActionKey}).toString(), QStringLiteral("copied"));
    QCOMPARE(copied.value(QLatin1String{exportmanifest::kBytesKey}).toInt(), 120);

    const QJsonObject mirrored = files.at(1).toObject();
    QCOMPARE(mirrored.value(QLatin1String{exportmanifest::kActionKey}).toString(), QStringLiteral("mirrored"));
    QCOMPARE(mirrored.value(QLatin1String{exportmanifest::kBytesKey}).toInt(), 4096);

    const QJsonObject written = files.at(2).toObject();
    QCOMPARE(written.value(QLatin1String{exportmanifest::kActionKey}).toString(), QStringLiteral("written"));
    QCOMPARE(written.value(QLatin1String{exportmanifest::kBytesKey}).toInt(),
             static_cast<int>(std::string{"controlDict text\n"}.size()));
}

void ExportManifestTest::manifestListsSkippedSourcesWithReason()
{
    const QJsonObject root = manifestObject(samplePlan());

    const QJsonArray skipped = root.value(QLatin1String{exportmanifest::kSkippedKey}).toArray();
    QCOMPARE(skipped.size(), 2);
    QCOMPARE(skipped.at(0).toObject().value(QLatin1String{exportmanifest::kRelativePathKey}).toString(),
             QStringLiteral("Allrun"));
    QCOMPARE(skipped.at(1).toObject().value(QLatin1String{exportmanifest::kRelativePathKey}).toString(),
             QStringLiteral("log.blockMesh"));

    for (const QJsonValue& value : skipped)
    {
        QVERIFY(!value.toObject().value(QLatin1String{exportmanifest::kReasonKey}).toString().isEmpty());
    }
}

void ExportManifestTest::manifestCountsMatchThePlan()
{
    const CaseExportPlan plan = samplePlan();
    const QJsonObject root = manifestObject(plan);

    const QJsonObject counts = root.value(QLatin1String{exportmanifest::kCountsKey}).toObject();
    QCOMPARE(counts.value(QStringLiteral("written")).toInt(), 1);
    QCOMPARE(counts.value(QStringLiteral("copied")).toInt(), 1);
    QCOMPARE(counts.value(QStringLiteral("mirrored")).toInt(), 1);
    QCOMPARE(counts.value(QStringLiteral("skipped")).toInt(), 2);
}

void ExportManifestTest::manifestUsesLfAndEndsWithNewline()
{
    const std::string text = ExportManifest::build(samplePlan(), "2026-10-27T14:30:00Z");

    QVERIFY(!text.empty());
    QVERIFY(text.find('\r') == std::string::npos);
    QVERIFY(text.back() == '\n');
    // Utf-8 without a byte order mark.
    QVERIFY(text.size() < 3 || static_cast<unsigned char>(text[0]) != 0xEF);
    QVERIFY(text.find("    \"schemaVersion\": 1,\n") != std::string::npos);
}

void ExportManifestTest::manifestNeverListsItself()
{
    // The staging directory write happens after the plan is final: the manifest
    // is not part of the plan and therefore cannot describe itself.
    const CaseExportPlan plan = samplePlan();
    const std::string text = ExportManifest::build(plan, "2026-10-27T14:30:00Z");

    QCOMPARE(plan.find(exportmanifest::kFileName), nullptr);
    QVERIFY(text.find(exportmanifest::kFileName) == std::string::npos);
}

} // namespace ofgui::io

QTEST_APPLESS_MAIN(ofgui::io::ExportManifestTest)
#include "ExportManifestTest.moc"
