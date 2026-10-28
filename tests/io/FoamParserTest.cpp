#include "io/FoamDiagnostic.h"
#include "io/FoamDictionary.h"
#include "io/FoamNode.h"
#include "io/FoamParseOptions.h"
#include "io/FoamValue.h"

#include "FoamParserTestSupport.h"

#include <QtTest>

#include <algorithm>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace ofgui::io {

using namespace foamtest;
class FoamParserTest : public QObject
{
    Q_OBJECT

private slots:
    void minimalDictionary();
    void nestedDictionariesAndLists();
    void dimensionSet();
    void variableEntry();
    void directivePreserved();
    void verbatimPreserved();
    void duplicateKeyword();
    void cavityDictionariesParse();
    void realBoundaryConditions();
    void visitReportsPaths();
};

void FoamParserTest::minimalDictionary()
{
    const FoamDictionary dictionary = parseText(
        "FoamFile\n"
        "{\n"
        "    version     2.0;\n"
        "    format      ascii;\n"
        "    class       dictionary;\n"
        "    object      controlDict;\n"
        "}\n"
        "\n"
        "application     icoFoam;\n"
        "endTime         0.5;\n");

    QVERIFY(dictionary.ok());
    QVERIFY(dictionary.diagnostics().empty());
    QVERIFY(dictionary.hasFoamFileHeader());
    QVERIFY(!dictionary.isEmpty());
    QCOMPARE(static_cast<int>(dictionary.root().kind()), static_cast<int>(FoamNodeKind::Dictionary));
    QCOMPARE(dictionary.root().childCount(), std::size_t{3});
    QCOMPARE(QString::fromStdString(valueText(dictionary, "FoamFile/class")), QStringLiteral("dictionary"));
    QCOMPARE(QString::fromStdString(valueText(dictionary, "application")), QStringLiteral("icoFoam"));
    QCOMPARE(QString::fromStdString(valueText(dictionary, "endTime")), QStringLiteral("0.5"));

    // Diagnostics carry the file label injected by the caller.
    const FoamDiagnostic probe = [&]() {
        const FoamDictionary bad = parseText("notAHeader 1;\n");
        return bad.diagnostics().front();
    }();
    QCOMPARE(QString::fromStdString(probe.location.file), QStringLiteral("inline"));
}

void FoamParserTest::nestedDictionariesAndLists()
{
    const FoamDictionary dictionary = parseFixture("cavity/system/blockMeshDict");
    QVERIFY2(dictionary.ok(), dictionary.diagnosticsText().c_str());

    const FoamNode* vertices = dictionary.find("vertices");
    QVERIFY(vertices != nullptr);
    QCOMPARE(static_cast<int>(vertices->kind()), static_cast<int>(FoamNodeKind::List));
    QCOMPARE(vertices->childCount(), std::size_t{8});
    QCOMPARE(static_cast<int>(vertices->childAt(0)->kind()), static_cast<int>(FoamNodeKind::List));
    QCOMPARE(static_cast<int>(vertices->childAt(0)->childCount()), 3);

    const FoamNode* blocks = dictionary.find("blocks");
    QVERIFY(blocks != nullptr);
    QCOMPARE(static_cast<int>(blocks->kind()), static_cast<int>(FoamNodeKind::List));
    // hex (0 1 2 3 4 5 6 7) (20 20 1) simpleGrading (1 1 1)
    QCOMPARE(blocks->childCount(), std::size_t{5});
    QCOMPARE(QString::fromStdString(FoamValue{*blocks->childAt(0)}.text()), QStringLiteral("hex"));
    QCOMPARE(blocks->childAt(1)->childCount(), std::size_t{8});
    QCOMPARE(blocks->childAt(2)->childCount(), std::size_t{3});
    QCOMPARE(QString::fromStdString(FoamValue{*blocks->childAt(3)}.text()), QStringLiteral("simpleGrading"));
    QCOMPARE(blocks->childAt(4)->childCount(), std::size_t{3});

    const FoamNode* movingWall = dictionary.find("boundary/movingWall");
    QVERIFY(movingWall != nullptr);
    QVERIFY(movingWall->isDictionary());
    QCOMPARE(QString::fromStdString(valueText(dictionary, "boundary/movingWall/type")), QStringLiteral("wall"));
    const FoamNode* faces = dictionary.find("boundary/movingWall/faces");
    QVERIFY(faces != nullptr);
    QCOMPARE(faces->childCount(), std::size_t{1});
    QCOMPARE(faces->childAt(0)->childCount(), std::size_t{4});
    QCOMPARE(QString::fromStdString(FoamValue{*faces->childAt(0)}.text()), QStringLiteral("(3 7 6 2)"));
}

void FoamParserTest::dimensionSet()
{
    const FoamDictionary dictionary = parseFixture("cavity/0/p");
    QVERIFY2(dictionary.ok(), dictionary.diagnosticsText().c_str());

    const FoamNode* dimensions = dictionary.find("dimensions");
    QVERIFY(dimensions != nullptr);
    QCOMPARE(static_cast<int>(dimensions->kind()), static_cast<int>(FoamNodeKind::DimensionSet));
    QCOMPARE(dimensions->childCount(), std::size_t{7});
    QCOMPARE(QString::fromStdString(FoamValue{*dimensions}.text()), QStringLiteral("[0 2 -2 0 0 0 0]"));
}

void FoamParserTest::variableEntry()
{
    const FoamDictionary dictionary = parseFixture("cavity/system/fvSolution");
    QVERIFY2(dictionary.ok(), dictionary.diagnosticsText().c_str());

    const FoamNode* entry = dictionary.find("solvers/pFinal");
    QVERIFY(entry != nullptr);
    QVERIFY(entry->isDictionary());
    QCOMPARE(entry->childCount(), std::size_t{2});

    const FoamNode* variable = entry->childAt(0);
    QVERIFY(!variable->hasKeyword());
    QCOMPARE(static_cast<int>(variable->kind()), static_cast<int>(FoamNodeKind::Variable));
    QCOMPARE(QString::fromStdString(variable->rawText()), QStringLiteral("$p"));
    QCOMPARE(QString::fromStdString(valueText(dictionary, "solvers/pFinal/relTol")), QStringLiteral("0"));
}

void FoamParserTest::directivePreserved()
{
    const FoamDictionary dictionary = parseFixture("pipeCyclic/U");
    QVERIFY2(dictionary.ok(), dictionary.diagnosticsText().c_str());

    const FoamNode* boundaryField = dictionary.find("boundaryField");
    QVERIFY(boundaryField != nullptr);
    QVERIFY(boundaryField->isDictionary());
    QVERIFY(boundaryField->childAt(0) != nullptr);

    const FoamNode* directive = boundaryField->childAt(0);
    QCOMPARE(static_cast<int>(directive->kind()), static_cast<int>(FoamNodeKind::Directive));
    QCOMPARE(QString::fromStdString(directive->rawText()), QStringLiteral("#includeEtc"));
    QCOMPARE(directive->childCount(), std::size_t{1});
    QCOMPARE(
        QString::fromStdString(directive->childAt(0)->rawText()),
        QStringLiteral("caseDicts/setConstraintTypes"));
    QCOMPARE(directive->location().line, 23);

    // The entry after the (semicolon-less) directive is still parsed.
    const FoamNode* inlet = dictionary.find("boundaryField/inlet");
    QVERIFY(inlet != nullptr);
    QVERIFY(inlet->isDictionary());

    // An include directive is reported as "kept verbatim", never expanded.
    const FoamDiagnostic* includeWarning = firstWithCode(dictionary.diagnostics(), "IMPORT-DIR-001");
    QVERIFY2(includeWarning != nullptr, dictionary.diagnosticsText().c_str());
    QCOMPARE(static_cast<int>(includeWarning->severity), static_cast<int>(FoamSeverity::Warning));
    QCOMPARE(includeWarning->location.line, 23);
    QVERIFY(includeWarning->message.find("includeEtc") != std::string::npos);

    // Unknown directives are still preserved, flagged as unsupported.
    const FoamDictionary unknown = FoamDictionary::parse(
        "FoamFile\n{\n    class dictionary;\n}\n\n#mysteryDirective 1 2;\nvalue 1;\n",
        "inline");
    QVERIFY(unknown.ok());
    const FoamDiagnostic* unknownWarning = firstWithCode(unknown.diagnostics(), "IMPORT-DIR-003");
    QVERIFY(unknownWarning != nullptr);
    QCOMPARE(unknownWarning->location.line, 6);
    const FoamNode* unknownNode = unknown.root().childAt(1);
    QVERIFY(unknownNode != nullptr);
    QCOMPARE(static_cast<int>(unknownNode->kind()), static_cast<int>(FoamNodeKind::Directive));
    QCOMPARE(unknownNode->childCount(), std::size_t{2});
    QVERIFY(unknown.find("value") != nullptr);
}

void FoamParserTest::verbatimPreserved()
{
    const FoamDictionary dictionary = parseFixture("pipeCyclic/U");
    QVERIFY(dictionary.ok());

    const FoamNode* code = dictionary.find("boundaryField/inlet/code");
    QVERIFY(code != nullptr);
    // A single-token value is unwrapped by the parser, so the code node is the verbatim itself.
    const FoamNode* verbatim = code;
    QCOMPARE(static_cast<int>(verbatim->kind()), static_cast<int>(FoamNodeKind::Verbatim));
    QVERIFY(verbatim->rawText().find("vectorField v(2.0*this->patch().Cf() ^ axis);") != std::string::npos);
    QVERIFY(verbatim->rawText().find("operator==(v);") != std::string::npos);
    QVERIFY(verbatim->rawText().find("#{") == std::string::npos);

    QCOMPARE(
        QString::fromStdString(valueText(dictionary, "boundaryField/inlet/value")),
        QStringLiteral("$internalField"));
}

void FoamParserTest::duplicateKeyword()
{
    const FoamDictionary dictionary = parseFixture("invalid/duplicate_keyword");
    QVERIFY(hasCode(dictionary.diagnostics(), "IMPORT-SYN-003"));
    QVERIFY(dictionary.ok()); // duplicate keywords are a warning, not an error
    const auto matches = dictionary.findAll("", "solver");
    QCOMPARE(matches.size(), std::size_t{2});
    QCOMPARE(QString::fromStdString(FoamValue{*matches[0]}.text()), QStringLiteral("PCG"));
    QCOMPARE(QString::fromStdString(FoamValue{*matches[1]}.text()), QStringLiteral("smoothSolver"));

    const FoamDiagnostic* diagnostic = firstWithCode(dictionary.diagnostics(), "IMPORT-SYN-003");
    QVERIFY(diagnostic != nullptr);
    QCOMPARE(diagnostic->location.line, 9);
}

void FoamParserTest::cavityDictionariesParse()
{
    for (const char* relative : kCavityDictionaries)
    {
        const FoamDictionary dictionary = parseFixture(relative);
        QVERIFY2(
            dictionary.ok(),
            qPrintable(QStringLiteral("%1: %2")
                           .arg(QString::fromUtf8(relative))
                           .arg(QString::fromStdString(dictionary.diagnosticsText()))));
        QVERIFY(dictionary.hasFoamFileHeader());
        QVERIFY(!dictionary.isEmpty());
    }

    const FoamDictionary controlDict = parseFixture("cavity/system/controlDict");
    QCOMPARE(QString::fromStdString(valueText(controlDict, "application")), QStringLiteral("icoFoam"));
    QCOMPARE(QString::fromStdString(valueText(controlDict, "endTime")), QStringLiteral("0.5"));
    QCOMPARE(QString::fromStdString(valueText(controlDict, "deltaT")), QStringLiteral("0.005"));

    const FoamDictionary schemes = parseFixture("cavity/system/fvSchemes");
    QCOMPARE(
        QString::fromStdString(valueText(schemes, "divSchemes/div(phi,U)")),
        QStringLiteral("Gauss linear"));
    QCOMPARE(
        QString::fromStdString(valueText(schemes, "gradSchemes/grad(p)")),
        QStringLiteral("Gauss linear"));

    const FoamDictionary transport = parseFixture("cavity/constant/transportProperties");
    QCOMPARE(QString::fromStdString(valueText(transport, "nu")), QStringLiteral("0.01"));
}

void FoamParserTest::realBoundaryConditions()
{
    const FoamDictionary p = parseFixture("cavity/0/p");
    QCOMPARE(QString::fromStdString(valueText(p, "internalField")), QStringLiteral("uniform 0"));
    QCOMPARE(QString::fromStdString(valueText(p, "boundaryField/movingWall/type")), QStringLiteral("zeroGradient"));
    QCOMPARE(QString::fromStdString(valueText(p, "boundaryField/frontAndBack/type")), QStringLiteral("empty"));

    const FoamDictionary U = parseFixture("cavity/0/U");
    QCOMPARE(QString::fromStdString(valueText(U, "internalField")), QStringLiteral("uniform (0 0 0)"));
    QCOMPARE(QString::fromStdString(valueText(U, "boundaryField/movingWall/value")), QStringLiteral("uniform (1 0 0)"));
    QCOMPARE(QString::fromStdString(valueText(U, "boundaryField/fixedWalls/type")), QStringLiteral("noSlip"));

    const FoamDictionary duplicate = parseFixture("pointDisplacement/pointDisplacement");
    QVERIFY2(duplicate.ok(), duplicate.diagnosticsText().c_str());
    const FoamNode* regex = duplicate.find("boundaryField/.*");
    QVERIFY(regex != nullptr);
    QVERIFY(regex->keywordIsString());
    const FoamNode* freeOrHole = duplicate.find("boundaryField/(free|hole)");
    QVERIFY(freeOrHole != nullptr);
    QCOMPARE(
        QString::fromStdString(valueText(duplicate, "boundaryField/(free|hole)/patchType")),
        QStringLiteral("overset"));
    const FoamNode* table = duplicate.find("boundaryField/(free|hole)/uniformValue");
    QVERIFY(table != nullptr);
    QCOMPARE(table->childCount(), std::size_t{2});
    QCOMPARE(static_cast<int>(table->childAt(1)->kind()), static_cast<int>(FoamNodeKind::List));
    QCOMPARE(table->childAt(1)->childCount(), std::size_t{3});
}


void FoamParserTest::visitReportsPaths()
{
    const FoamDictionary dictionary = parseFixture("cavity/0/p");
    QVERIFY(dictionary.ok());

    std::vector<std::string> paths;
    dictionary.visit([&paths](const FoamNode&, std::string_view path) {
        paths.emplace_back(path);
    });

    QVERIFY(!paths.empty());
    const auto hasPath = [&paths](std::string_view expected) {
        return std::any_of(paths.begin(), paths.end(), [expected](const std::string& path) {
            return path == expected;
        });
    };
    QVERIFY(hasPath("boundaryField"));
    QVERIFY(hasPath("boundaryField.movingWall"));
    QVERIFY(hasPath("boundaryField.movingWall.type"));
    QVERIFY(hasPath("internalField"));
    QVERIFY(hasPath("dimensions"));
}

} // namespace ofgui::io

QTEST_APPLESS_MAIN(ofgui::io::FoamParserTest)

#include "FoamParserTest.moc"
