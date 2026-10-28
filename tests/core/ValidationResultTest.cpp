#include "core/ValidationResult.h"

#include <QtTest>

#include <string>

namespace ofgui::core {
namespace {

class ValidationResultTest final : public QObject
{
    Q_OBJECT

private slots:
    void startsEmpty();
    void countsBySeverity();
    void separatesErrorsFromBlockingErrors();
    void filtersBySeverity();
    void summarisesCounts();
    void mergesOtherResults();
};

void ValidationResultTest::startsEmpty()
{
    const ValidationResult result;
    QVERIFY(result.empty());
    QCOMPARE(static_cast<int>(result.size()), 0);
    QVERIFY(!result.hasErrors());
    QVERIFY(!result.hasBlockingErrors());
    QVERIFY(result.firstBlockingError() == nullptr);
    QCOMPARE(QString::fromStdString(result.summary()), QStringLiteral("阻断 0、错误 0、警告 0、提示 0"));
}

void ValidationResultTest::countsBySeverity()
{
    ValidationResult result;
    result.add(ValidationIssue(Severity::Info, "I", "i"));
    result.add(ValidationIssue(Severity::Warning, "W1", "w1"));
    result.add(ValidationIssue(Severity::Warning, "W2", "w2"));
    result.add(ValidationIssue(Severity::Error, "E", "e"));
    result.add(ValidationIssue(Severity::BlockingError, "B", "b"));

    QCOMPARE(static_cast<int>(result.size()), 5);
    QCOMPARE(static_cast<int>(result.count(Severity::Info)), 1);
    QCOMPARE(static_cast<int>(result.count(Severity::Warning)), 2);
    QCOMPARE(static_cast<int>(result.count(Severity::Error)), 1);
    QCOMPARE(static_cast<int>(result.count(Severity::BlockingError)), 1);
}

void ValidationResultTest::separatesErrorsFromBlockingErrors()
{
    ValidationResult errorsOnly;
    errorsOnly.add(ValidationIssue(Severity::Error, "E", "e"));
    QVERIFY(errorsOnly.hasErrors());
    QVERIFY(!errorsOnly.hasBlockingErrors());
    QVERIFY(errorsOnly.firstBlockingError() == nullptr);

    errorsOnly.add(ValidationIssue(Severity::BlockingError, "B", "b"));
    QVERIFY(errorsOnly.hasBlockingErrors());
    QVERIFY(errorsOnly.firstBlockingError() != nullptr);
    QCOMPARE(QString::fromStdString(errorsOnly.firstBlockingError()->code()), QStringLiteral("B"));
}

void ValidationResultTest::filtersBySeverity()
{
    ValidationResult result;
    result.add(ValidationIssue(Severity::Info, "I", "i"));
    result.add(ValidationIssue(Severity::Warning, "W", "w"));
    result.add(ValidationIssue(Severity::Error, "E", "e"));
    result.add(ValidationIssue(Severity::BlockingError, "B", "b"));

    QCOMPARE(static_cast<int>(result.issuesWithSeverityAtLeast(Severity::Error).size()), 2);
    QCOMPARE(static_cast<int>(result.issuesWithSeverityAtLeast(Severity::Warning).size()), 3);
    QCOMPARE(static_cast<int>(result.issuesWithSeverityAtLeast(Severity::Info).size()), 4);
}

void ValidationResultTest::summarisesCounts()
{
    ValidationResult result;
    result.add(ValidationIssue(Severity::Info, "I", "i"));
    result.add(ValidationIssue(Severity::Warning, "W", "w"));
    result.add(ValidationIssue(Severity::Error, "E", "e"));
    result.add(ValidationIssue(Severity::BlockingError, "B", "b"));

    QCOMPARE(QString::fromStdString(result.summary()), QStringLiteral("阻断 1、错误 1、警告 1、提示 1"));
}

void ValidationResultTest::mergesOtherResults()
{
    ValidationResult first;
    first.add(ValidationIssue(Severity::Warning, "W", "w"));

    ValidationResult second;
    second.add(ValidationIssue(Severity::Error, "E", "e"));

    first.addAll(second);
    QCOMPARE(static_cast<int>(first.size()), 2);
    QVERIFY(first.hasErrors());

    first.clear();
    QVERIFY(first.empty());
}

} // namespace
} // namespace ofgui::core

QTEST_APPLESS_MAIN(ofgui::core::ValidationResultTest)

#include "ValidationResultTest.moc"
