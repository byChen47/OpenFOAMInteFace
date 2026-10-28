#include "core/SourceLocation.h"
#include "core/ValidationIssue.h"

#include <QtTest>

#include <string>

namespace ofgui::core {
namespace {

class ValidationIssueTest final : public QObject
{
    Q_OBJECT

private slots:
    void defaultIsInfoFromModel();
    void storesAllFields();
    void onlyBlockingErrorBlocks();
    void ordersSeverities();
    void formatsFieldAndFix();
    void formatsLocationAndNodePath();
    void formatsWithoutOptionalParts();
    void formatsSourceLocationText();
};

void ValidationIssueTest::defaultIsInfoFromModel()
{
    const ValidationIssue issue;
    QCOMPARE(issue.severity(), Severity::Info);
    QVERIFY(issue.code().empty());
    QVERIFY(issue.message().empty());
    QVERIFY(!issue.targetObjectId().has_value());
    QVERIFY(!issue.location().has_value());
    QCOMPARE(QString::fromStdString(issue.origin()), QStringLiteral("model"));
}

void ValidationIssueTest::storesAllFields()
{
    ValidationIssue issue(Severity::Error, "CASE-TEST-001", "出错了");
    issue.setTargetObjectId(ObjectId{0x42});
    issue.setTargetField("domain.min");
    issue.setSuggestedFix("修正数值");
    issue.setLocation(SourceLocation("system/blockMeshDict", 12, 5));
    issue.setNodePath("blockMeshDict.blocks[0]");
    issue.setSnippet("vertices");
    issue.setOrigin("import");

    QCOMPARE(issue.severity(), Severity::Error);
    QCOMPARE(QString::fromStdString(issue.code()), QStringLiteral("CASE-TEST-001"));
    QVERIFY(issue.targetObjectId().has_value());
    QCOMPARE(static_cast<qulonglong>(issue.targetObjectId()->value()), 0x42ULL);
    QCOMPARE(QString::fromStdString(issue.targetField()), QStringLiteral("domain.min"));
    QCOMPARE(issue.location()->line(), 12);
    QCOMPARE(QString::fromStdString(issue.nodePath()), QStringLiteral("blockMeshDict.blocks[0]"));
    QCOMPARE(QString::fromStdString(issue.snippet()), QStringLiteral("vertices"));
    QCOMPARE(QString::fromStdString(issue.origin()), QStringLiteral("import"));

    issue.clearTargetObjectId();
    issue.clearLocation();
    QVERIFY(!issue.targetObjectId().has_value());
    QVERIFY(!issue.location().has_value());
}

void ValidationIssueTest::onlyBlockingErrorBlocks()
{
    QVERIFY(ValidationIssue(Severity::BlockingError, "A", "a").isBlocking());
    QVERIFY(!ValidationIssue(Severity::Error, "B", "b").isBlocking());
    QVERIFY(!ValidationIssue(Severity::Warning, "C", "c").isBlocking());
    QVERIFY(!ValidationIssue(Severity::Info, "D", "d").isBlocking());
}

void ValidationIssueTest::ordersSeverities()
{
    const ValidationIssue info(Severity::Info, "A", "a");
    const ValidationIssue warning(Severity::Warning, "B", "b");
    const ValidationIssue error(Severity::Error, "C", "c");
    const ValidationIssue blocking(Severity::BlockingError, "D", "d");

    QVERIFY(!info.isAtLeast(Severity::Warning));
    QVERIFY(warning.isAtLeast(Severity::Warning));
    QVERIFY(warning.isAtLeast(Severity::Info));
    QVERIFY(error.isAtLeast(Severity::Error));
    QVERIFY(!error.isAtLeast(Severity::BlockingError));
    QVERIFY(blocking.isAtLeast(Severity::BlockingError));
    QCOMPARE(QString::fromStdString(std::string{severityName(Severity::BlockingError)}),
        QStringLiteral("BlockingError"));
}

void ValidationIssueTest::formatsFieldAndFix()
{
    ValidationIssue issue(Severity::Error, "CASE-MESH-001", "message");
    issue.setTargetField("mesh.cells.z");
    issue.setSuggestedFix("fix it");

    QCOMPARE(QString::fromStdString(issue.format()),
        QStringLiteral("Error CASE-MESH-001: message [field: mesh.cells.z] [fix: fix it]"));
}

void ValidationIssueTest::formatsLocationAndNodePath()
{
    ValidationIssue issue(Severity::BlockingError, "IMPORT-PARSE-001", "unexpected token");
    issue.setLocation(SourceLocation("system/blockMeshDict", 12, 5));
    issue.setNodePath("blocks[0]");

    QCOMPARE(QString::fromStdString(issue.format()),
        QStringLiteral(
            "BlockingError IMPORT-PARSE-001: unexpected token [at: system/blockMeshDict:12:5] "
            "[node: blocks[0]]"));
}

void ValidationIssueTest::formatsWithoutOptionalParts()
{
    const ValidationIssue issue(Severity::Info, "CASE-INFO-001", "hello");
    QCOMPARE(QString::fromStdString(issue.format()), QStringLiteral("Info CASE-INFO-001: hello"));
}

void ValidationIssueTest::formatsSourceLocationText()
{
    QVERIFY(!SourceLocation{}.isValid());
    QCOMPARE(QString::fromStdString(SourceLocation{}.toString()), QString());

    const SourceLocation fileOnly("constant/transportProperties");
    QVERIFY(fileOnly.isValid());
    QCOMPARE(QString::fromStdString(fileOnly.toString()), QStringLiteral("constant/transportProperties"));

    QCOMPARE(QString::fromStdString(SourceLocation("0/p", 3).toString()), QStringLiteral("0/p:3"));
    QCOMPARE(QString::fromStdString(SourceLocation("0/p", 3, 7).toString()), QStringLiteral("0/p:3:7"));
    QCOMPARE(QString::fromStdString(SourceLocation("0/p", 0, 7).toString()), QStringLiteral("0/p"));
}

} // namespace
} // namespace ofgui::core

QTEST_APPLESS_MAIN(ofgui::core::ValidationIssueTest)

#include "ValidationIssueTest.moc"
