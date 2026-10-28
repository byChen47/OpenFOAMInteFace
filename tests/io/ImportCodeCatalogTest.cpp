#include "io/ImportCodes.h"
#include "io/ImportIssueFactory.h"

#include <core/ValidationIssue.h>

#include <QtTest>

#include <array>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace ofgui::io {
namespace {

bool isKnownSeverity(core::Severity severity)
{
    switch (severity)
    {
    case core::Severity::Info:
    case core::Severity::Warning:
    case core::Severity::Error:
    case core::Severity::BlockingError:
        return true;
    }
    return false;
}

const ImportCodeInfo* infoFor(std::string_view code)
{
    static thread_local std::optional<ImportCodeInfo> cached;
    cached = importCodeInfo(code);
    return cached.has_value() ? &cached.value() : nullptr;
}

} // namespace

/// Guards the contract D-039 depends on: the UI shows issues by looking up
/// `issue.code()` in `importCodeInfo`, so every emitted code must have exactly
/// one row, a stable severity and human readable Chinese text.
class ImportCodeCatalogTest : public QObject
{
    Q_OBJECT

private slots:
    void tableCoversEveryCode();
    void tableHasNoDuplicates();
    void everyRowCarriesText();
    void severityMatchesTheSpecification();
    void messagesAreNotInternalDetails();
};

void ImportCodeCatalogTest::tableCoversEveryCode()
{
    for (const char* code : importcodes::kAllCodes)
    {
        QVERIFY2(infoFor(code) != nullptr, code);
    }
    QCOMPARE(importcodes::kAllCodes.size(), importcodes::kCodeCount);
}

void ImportCodeCatalogTest::tableHasNoDuplicates()
{
    std::set<std::string> seen;
    for (const char* code : importcodes::kAllCodes)
    {
        const auto inserted = seen.insert(code).second;
        QVERIFY2(inserted, code);
    }
    QCOMPARE(seen.size(), importcodes::kCodeCount);
}

void ImportCodeCatalogTest::everyRowCarriesText()
{
    for (const char* code : importcodes::kAllCodes)
    {
        const ImportCodeInfo* info = infoFor(code);
        QVERIFY2(info != nullptr, code);
        QVERIFY2(!info->message.empty(), code);
        QVERIFY2(!info->suggestedFix.empty(), code);
        QVERIFY2(isKnownSeverity(info->severity), code);
        // The message is shown instead of the caller supplied detail, so it must
        // never look like an internal token.
        QVERIFY2(info->message.find("IMPORT-") == std::string_view::npos, code);
    }
}

void ImportCodeCatalogTest::severityMatchesTheSpecification()
{
    // `docs/CASE_IMPORT_SPEC.md` appendix C is authoritative for severities;
    // these are the rows the review flagged as wrong when the table was short.
    QVERIFY(infoFor(importcodes::kTurbulenceMissing)->severity == core::Severity::Info);
    QVERIFY(infoFor(importcodes::kExportPdrCoexists)->severity == core::Severity::Info);
    QVERIFY(infoFor(importcodes::kLimitNonUniform)->severity == core::Severity::Warning);
    QVERIFY(infoFor(importcodes::kReportTooManyFiles)->severity == core::Severity::Warning);
    QVERIFY(infoFor(importcodes::kPatchBoundaryMismatch)->severity == core::Severity::Warning);
    QVERIFY(infoFor(importcodes::kPatchFaceConflict)->severity == core::Severity::Error);
    QVERIFY(infoFor(importcodes::kCaseConflictingTarget)->severity == core::Severity::BlockingError);
    QVERIFY(infoFor(importcodes::kEncNonAscii)->severity == core::Severity::Warning);
    QVERIFY(infoFor(importcodes::kParseNestingTooDeep)->severity == core::Severity::Error);
    QVERIFY(infoFor(importcodes::kReportTruncated)->severity == core::Severity::Info);
}

void ImportCodeCatalogTest::messagesAreNotInternalDetails()
{
    const std::vector<core::ValidationIssue> issues{
        makeImportIssue(importcodes::kTurbulenceMissing, "0/p", 1, 1, "turbulenceProperties", {}, {}),
        makeImportIssue(importcodes::kExportPdrCoexists, "system/blockMeshDict", 3, 5, "PDRblockMeshDict", {}, {}),
        makeImportIssue(importcodes::kPatchFaceConflict, "system/blockMeshDict", 9, 9, "boundary", {}, "xMin:0"),
    };

    QCOMPARE(issues.size(), std::size_t{3});
    QCOMPARE(issues[0].severity(), core::Severity::Info);
    QCOMPARE(issues[0].message(), std::string{"没有 constant/turbulenceProperties，按 laminar 处理"});
    QCOMPARE(issues[1].message(), std::string{"同时存在 PDRblockMeshDict 与 blockMeshDict，本次只解析后者"});
    QCOMPARE(issues[2].message(), std::string{"同一个面被多个 patch 同时认领"});
    // The detail is appended for diagnostics, never used as the message.
    QVERIFY(issues[2].message() != std::string{"xMin:0"});
    for (const core::ValidationIssue& issue : issues)
    {
        QCOMPARE(issue.origin(), std::string{"import"});
        QVERIFY(!issue.suggestedFix().empty());
    }
}

} // namespace ofgui::io

QTEST_APPLESS_MAIN(ofgui::io::ImportCodeCatalogTest)
#include "ImportCodeCatalogTest.moc"
