#include "FoamParserTestSupport.h"

#include "io/FoamDictionary.h"
#include "io/FoamNode.h"

#include <QtTest>

#include <cstddef>
#include <string>
#include <string_view>

namespace ofgui::io {

using namespace foamtest;

/// Error recovery, header rules and the canonical text emitter.
class FoamParserRecoveryTest : public QObject
{
    Q_OBJECT

private slots:
    void missingSemicolonRecovery();
    void missingClosingBrace();
    void strayClosingBrace();
    void missingFoamFileHeader();
    void optionalFoamFileHeader();
    void diagnosticTruncation();
    void crlfIsTolerated();
    void canonicalTextShape();
    void canonicalTextRoundTrip();
    void canonicalTextKeepsSpecialValues();
    void nestingDepthIsBounded();
};

void FoamParserRecoveryTest::missingSemicolonRecovery()
{
    const FoamDictionary dictionary = parseFixture("invalid/missing_semicolon");
    QVERIFY(!dictionary.ok());
    const FoamDiagnostic* diagnostic = firstWithCode(dictionary.diagnostics(), "IMPORT-PARSE-001");
    QVERIFY2(diagnostic != nullptr, dictionary.diagnosticsText().c_str());
    QCOMPARE(diagnostic->location.line, 11); // the value whose ';' is missing
    QVERIFY(!diagnostic->snippet.empty());

    // Recovery keeps both the truncated entry and the following sibling.
    const FoamNode* ddtSchemes = dictionary.find("ddtSchemes");
    QVERIFY(ddtSchemes != nullptr);
    QCOMPARE(QString::fromStdString(valueText(dictionary, "ddtSchemes/default")), QStringLiteral("Euler"));
    const FoamNode* gradSchemes = dictionary.find("ddtSchemes/gradSchemes");
    QVERIFY2(gradSchemes != nullptr, dictionary.diagnosticsText().c_str());
    QCOMPARE(
        QString::fromStdString(valueText(dictionary, "ddtSchemes/gradSchemes/default")),
        QStringLiteral("Gauss linear"));
}

void FoamParserRecoveryTest::missingClosingBrace()
{
    const FoamDictionary dictionary = parseFixture("invalid/missing_brace");
    QVERIFY(!dictionary.ok());
    QVERIFY(hasCode(dictionary.diagnostics(), "IMPORT-PARSE-002"));

    // Every unclosed brace is reported: the FoamFile block and boundaryField.
    std::vector<int> reportedLines;
    for (const FoamDiagnostic& diagnostic : dictionary.diagnostics())
    {
        if (diagnostic.code == "IMPORT-PARSE-002")
        {
            reportedLines.push_back(diagnostic.location.line);
        }
    }
    std::sort(reportedLines.begin(), reportedLines.end());
    QCOMPARE(reportedLines, (std::vector<int>{2, 8}));

    // The entries read before EOF are preserved.
    QCOMPARE(
        QString::fromStdString(valueText(dictionary, "FoamFile/boundaryField/movingWall/type")),
        QStringLiteral("zeroGradient"));
}

void FoamParserRecoveryTest::strayClosingBrace()
{
    const FoamDictionary dictionary = parseFixture("invalid/stray_brace");
    QVERIFY(!dictionary.ok());
    const FoamDiagnostic* diagnostic = firstWithCode(dictionary.diagnostics(), "IMPORT-PARSE-006");
    QVERIFY(diagnostic != nullptr);
    QCOMPARE(diagnostic->location.line, 9);

    // Parsing continues after the stray brace.
    QCOMPARE(QString::fromStdString(valueText(dictionary, "other")), QStringLiteral("2"));
}

void FoamParserRecoveryTest::missingFoamFileHeader()
{
    const FoamDictionary dictionary = parseText("application icoFoam;\n");
    QVERIFY(!dictionary.ok());
    QVERIFY(!dictionary.hasFoamFileHeader());
    const FoamDiagnostic* diagnostic = firstWithCode(dictionary.diagnostics(), "IMPORT-PARSE-003");
    QVERIFY(diagnostic != nullptr);
    QCOMPARE(diagnostic->location.line, 1);
    QCOMPARE(QString::fromStdString(valueText(dictionary, "application")), QStringLiteral("icoFoam"));
}

void FoamParserRecoveryTest::optionalFoamFileHeader()
{
    FoamParseOptions options;
    options.requireFoamFileHeader = false;
    const FoamDictionary dictionary = parseText("application icoFoam;\n", options);
    QVERIFY2(dictionary.ok(), dictionary.diagnosticsText().c_str());
    QVERIFY(dictionary.diagnostics().empty());
    QVERIFY(!dictionary.hasFoamFileHeader());
}

void FoamParserRecoveryTest::diagnosticTruncation()
{
    std::string source;
    for (int i = 0; i < 250; ++i)
    {
        source += ";\n";
    }

    FoamParseOptions options;
    options.requireFoamFileHeader = false;
    options.maxDiagnostics = 200;
    const FoamDictionary dictionary = parseText(source, options);

    QCOMPARE(dictionary.diagnostics().size(), std::size_t{201});
    const FoamDiagnostic& last = dictionary.diagnostics().back();
    QCOMPARE(QString::fromStdString(last.code), QStringLiteral("IMPORT-REPORT-001"));
    QCOMPARE(static_cast<int>(last.severity), static_cast<int>(FoamSeverity::Info));
    QVERIFY(foamCount(dictionary.diagnostics(), FoamSeverity::Error) == 200);
}

void FoamParserRecoveryTest::crlfIsTolerated()
{
    const std::string source =
        "FoamFile\r\n{\r\n    version     2.0;\r\n    class       dictionary;\r\n}\r\n\r\n"
        "endTime\t0.5;\r\n";
    const FoamDictionary dictionary = parseText(source);
    QVERIFY2(dictionary.ok(), dictionary.diagnosticsText().c_str());
    QCOMPARE(QString::fromStdString(valueText(dictionary, "endTime")), QStringLiteral("0.5"));
}

void FoamParserRecoveryTest::canonicalTextShape()
{
    const FoamDictionary dictionary = parseFixture("cavity/system/controlDict");
    QVERIFY(dictionary.ok());
    const std::string text = dictionary.toText();

    QVERIFY(text.find("FoamFile\n{\n") != std::string::npos);
    QVERIFY(text.find("    version    2.0;\n") != std::string::npos);
    // Root-level entries carry no indentation, exactly like OpenFOAM writes them.
    QVERIFY(text.find("application    icoFoam;\n") != std::string::npos);
    QVERIFY(text.find("endTime    0.5;\n") != std::string::npos);
    QVERIFY(text.find('\r') == std::string::npos);
    QVERIFY(!text.empty());
    QVERIFY(text.back() == '\n');
}

void FoamParserRecoveryTest::canonicalTextRoundTrip()
{
    for (const char* relative : kCavityDictionaries)
    {
        const FoamDictionary first = parseFixture(relative);
        QVERIFY(first.ok());

        const std::string text = first.toText();
        const FoamDictionary second = parseText(text);
        QVERIFY2(
            second.ok(),
            qPrintable(QStringLiteral("%1: %2")
                           .arg(QString::fromUtf8(relative))
                           .arg(QString::fromStdString(second.diagnosticsText()))));
        QVERIFY2(
            structurallyEqual(first.root(), second.root()),
            qPrintable(QStringLiteral("round-trip mismatch: %1").arg(QString::fromUtf8(relative))));
        QCOMPARE(QString::fromStdString(second.toText()), QString::fromStdString(text));
    }
}

void FoamParserRecoveryTest::canonicalTextKeepsSpecialValues()
{
    const FoamDictionary dictionary = parseFixture("pipeCyclic/U");
    QVERIFY(dictionary.ok());
    const std::string text = dictionary.toText();

    QVERIFY(text.find("#includeEtc \"caseDicts/setConstraintTypes\"\n") != std::string::npos);
    QVERIFY(text.find("code") != std::string::npos);
    QVERIFY(text.find("#{") != std::string::npos);
    QVERIFY(text.find("#};") != std::string::npos);
    QVERIFY(text.find("value    $internalField;\n") != std::string::npos);

    const FoamDictionary second = parseText(text);
    QVERIFY2(second.ok(), second.diagnosticsText().c_str());
    QVERIFY(structurallyEqual(dictionary.root(), second.root()));

    const FoamDictionary displacement = parseFixture("pointDisplacement/pointDisplacement");
    const std::string displacementText = displacement.toText();
    QVERIFY(displacementText.find("\".*\"\n") != std::string::npos);
    QVERIFY(displacementText.find("\"(free|hole)\"\n") != std::string::npos);
    const FoamDictionary reparsed = parseText(displacementText);
    QVERIFY2(reparsed.ok(), reparsed.diagnosticsText().c_str());
    QVERIFY(reparsed.find("boundaryField/(free|hole)") != nullptr);
}

void FoamParserRecoveryTest::nestingDepthIsBounded()
{
    // 40 nested dictionaries must not recurse without bound: the parser stops at
    // the configured depth, reports IMPORT-PARSE-009 and skips the rest.
    constexpr int kLevels = 40;
    std::string source = "FoamFile { version 2.0; format ascii; class dictionary; object deep; }\n";
    for (int level = 0; level < kLevels; ++level)
    {
        source += "level" + std::to_string(level) + "\n{\n";
    }
    source += "leaf value;\n";
    for (int level = 0; level < kLevels; ++level)
    {
        source += "}\n";
    }

    FoamParseOptions options;
    options.maxNestingDepth = 8;
    const FoamDictionary dictionary = parseText(source, options);

    QVERIFY(!dictionary.ok());
    const FoamDiagnostic* diagnostic = firstWithCode(dictionary.diagnostics(), "IMPORT-PARSE-009");
    QVERIFY2(diagnostic != nullptr, dictionary.diagnosticsText().c_str());
    QCOMPARE(diagnostic->location.line, 3 + 2 * 8); // the 9th opening brace
    QVERIFY(diagnostic->message.find("8 levels") != std::string::npos);

    // A default parse of the same text succeeds: real dictionaries stay shallow.
    const FoamDictionary relaxed = parseText(source);
    QVERIFY2(relaxed.ok(), relaxed.diagnosticsText().c_str());
    QVERIFY(relaxed.find("level0/level1/level2") != nullptr);

    // The depth guard is per parse, so a second parse of shallow text is clean.
    const FoamDictionary shallow = parseText("FoamFile { version 2.0; format ascii; class dictionary; object x; }\na { b { c 1; } }");
    QVERIFY(shallow.ok());
}

} // namespace ofgui::io

QTEST_APPLESS_MAIN(ofgui::io::FoamParserRecoveryTest)

#include "FoamParserRecoveryTest.moc"
