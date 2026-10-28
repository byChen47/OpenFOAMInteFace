#include "io/FoamDiagnostic.h"
#include "io/FoamDictionary.h"
#include "io/FoamNode.h"
#include "io/FoamParseOptions.h"
#include "io/FoamValue.h"

#include <QtTest>

#include <array>
#include <filesystem>
#include <string>
#include <vector>

namespace ofgui::io {
namespace {

QString fixturePath(const char* relative)
{
    return QStringLiteral(OFGUI_FOAM_FIXTURE_DIR) + QLatin1Char('/') + QString::fromUtf8(relative);
}

FoamDictionary parseFixture(const char* relative, FoamParseOptions options = FoamParseOptions{})
{
    return FoamDictionary::parseFile(
        std::filesystem::path{fixturePath(relative).toStdWString()}, options);
}

std::string writeTempFile(QTemporaryDir& dir, const char* name, const QByteArray& content)
{
    const QString path = dir.filePath(QString::fromUtf8(name));
    QFile file{path};
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        return {};
    }
    file.write(content);
    file.close();
    return path.toStdString();
}

std::filesystem::path nativePath(const std::string& utf8Path)
{
    return std::filesystem::path{QString::fromStdString(utf8Path).toStdWString()};
}

const FoamDiagnostic* firstWithCode(
    const std::vector<FoamDiagnostic>& diagnostics, std::string_view code)
{
    for (const FoamDiagnostic& diagnostic : diagnostics)
    {
        if (diagnostic.code == code)
        {
            return &diagnostic;
        }
    }
    return nullptr;
}

class FoamDictionaryTest : public QObject
{
    Q_OBJECT

private slots:
    void missingFile();
    void emptyFile();
    void utf16File();
    void utf8BomFile();
    void binaryFile();
    void fileSizeLimit();
    void scalarConversions();
    void vectorConversion();
    void dimensionsConversion();
    void realListConversion();
    void wordConversions();
    void pathQueries();
    void sourceTextIsByteExact();
    void headerAccessors();
    void diagnosticsTextFormat();
};

void FoamDictionaryTest::missingFile()
{
    const FoamDictionary dictionary =
        FoamDictionary::parseFile(std::filesystem::path{"this-file-does-not-exist.pumm"});
    QVERIFY(!dictionary.ok());
    QVERIFY(dictionary.isEmpty());
    const FoamDiagnostic* diagnostic = firstWithCode(dictionary.diagnostics(), "IMPORT-IO-001");
    QVERIFY(diagnostic != nullptr);
    QCOMPARE(static_cast<int>(diagnostic->severity), static_cast<int>(FoamSeverity::Error));
    QVERIFY(!dictionary.diagnosticsText().empty());
}

void FoamDictionaryTest::emptyFile()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const std::string path = writeTempFile(dir, "empty.dict", QByteArray{});
    QVERIFY(!path.empty());

    const FoamDictionary dictionary = FoamDictionary::parseFile(nativePath(path));
    QVERIFY(dictionary.ok());
    QVERIFY(dictionary.isEmpty());
    QVERIFY(firstWithCode(dictionary.diagnostics(), "IMPORT-IO-002") != nullptr);
}

void FoamDictionaryTest::utf16File()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QByteArray content;
    content.append('\xFF');
    content.append('\xFE');
    content.append("F\0o\0a\0m\0", 8);
    const std::string path = writeTempFile(dir, "utf16.dict", content);
    QVERIFY(!path.empty());

    const FoamDictionary dictionary = FoamDictionary::parseFile(nativePath(path));
    QVERIFY(!dictionary.ok());
    const FoamDiagnostic* diagnostic = firstWithCode(dictionary.diagnostics(), "IMPORT-ENC-001");
    QVERIFY(diagnostic != nullptr);
    QVERIFY(dictionary.isEmpty());
}

void FoamDictionaryTest::utf8BomFile()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QByteArray content;
    content.append('\xEF');
    content.append('\xBB');
    content.append('\xBF');
    content.append("FoamFile\n{\n    version     2.0;\n}\n\nvalue 7;\n");
    const std::string path = writeTempFile(dir, "bom.dict", content);
    QVERIFY(!path.empty());

    const FoamDictionary dictionary = FoamDictionary::parseFile(nativePath(path));
    QVERIFY2(dictionary.ok(), dictionary.diagnosticsText().c_str());
    QVERIFY(firstWithCode(dictionary.diagnostics(), "IMPORT-ENC-003") != nullptr);
    QCOMPARE(QString::fromStdString(FoamValue{*dictionary.find("value")}.text()), QStringLiteral("7"));
}

void FoamDictionaryTest::binaryFile()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QByteArray content{"FoamFile\n{\n"};
    content.append('\0');
    content.append("}\n");
    const std::string path = writeTempFile(dir, "binary.dict", content);
    QVERIFY(!path.empty());

    const FoamDictionary dictionary = FoamDictionary::parseFile(nativePath(path));
    QVERIFY(!dictionary.ok());
    QVERIFY(firstWithCode(dictionary.diagnostics(), "IMPORT-ENC-001") != nullptr);
}

void FoamDictionaryTest::fileSizeLimit()
{
    FoamParseOptions options;
    options.maxFileBytes = 16;
    const FoamDictionary dictionary = parseFixture("cavity/system/controlDict", options);
    QVERIFY(!dictionary.ok());
    const FoamDiagnostic* diagnostic = firstWithCode(dictionary.diagnostics(), "IMPORT-IO-001");
    QVERIFY(diagnostic != nullptr);
    QVERIFY(dictionary.isEmpty());
}

void FoamDictionaryTest::scalarConversions()
{
    const FoamDictionary p = parseFixture("cavity/0/p");
    QCOMPARE(QString::fromStdString(FoamValue{*p.find("internalField")}.text()), QStringLiteral("uniform 0"));

    double value = -1.0;
    QVERIFY(FoamValue{*p.find("internalField")}.asReal(value));
    QCOMPARE(value, 0.0);
    QVERIFY(FoamValue{*p.find("internalField")}.isUniform());
    QVERIFY(!FoamValue{*p.find("internalField")}.isNonuniform());

    const FoamDictionary controlDict = parseFixture("cavity/system/controlDict");
    long long integer = 0;
    QVERIFY(FoamValue{*controlDict.find("writeInterval")}.asInt(integer));
    QCOMPARE(integer, 20LL);

    value = -1.0;
    QVERIFY(FoamValue{*controlDict.find("deltaT")}.asReal(value));
    QCOMPARE(value, 0.005);

    bool flag = false;
    QVERIFY(FoamValue{*controlDict.find("runTimeModifiable")}.asBool(flag));
    QVERIFY(flag);

    std::string word;
    QVERIFY(FoamValue{*controlDict.find("application")}.asWord(word));
    QCOMPARE(QString::fromStdString(word), QStringLiteral("icoFoam"));

    std::string type;
    QVERIFY(FoamValue{*p.find("boundaryField/frontAndBack/type")}.asWord(type));
    QCOMPARE(QString::fromStdString(type), QStringLiteral("empty"));

    // A dictionary has no scalar value.
    QVERIFY(!FoamValue{*p.find("boundaryField")}.asReal(value));
}

void FoamDictionaryTest::vectorConversion()
{
    const FoamDictionary U = parseFixture("cavity/0/U");
    const FoamNode* movingWall = U.find("boundaryField/movingWall/value");
    QVERIFY(movingWall != nullptr);

    std::array<double, 3> vector{};
    QVERIFY(FoamValue{*movingWall}.asVector(vector));
    QCOMPARE(vector[0], 1.0);
    QCOMPARE(vector[1], 0.0);
    QCOMPARE(vector[2], 0.0);

    std::array<double, 3> internal{};
    QVERIFY(FoamValue{*U.find("internalField")}.asVector(internal));
    QCOMPARE(internal[0], 0.0);

    // `dimensions [...]` is not a vector list.
    std::array<double, 3> notAVector{};
    QVERIFY(!FoamValue{*U.find("dimensions")}.asVector(notAVector));
}

void FoamDictionaryTest::dimensionsConversion()
{
    const FoamDictionary p = parseFixture("cavity/0/p");
    std::array<int, 7> pDimensions{};
    QVERIFY(FoamValue{*p.find("dimensions")}.asDimensions(pDimensions));
    const std::array<int, 7> expectedP{0, 2, -2, 0, 0, 0, 0};
    for (std::size_t i = 0; i < expectedP.size(); ++i)
    {
        QCOMPARE(pDimensions[i], expectedP[i]);
    }

    const FoamDictionary T = parseFixture("field/nonuniformT");
    QVERIFY2(T.ok(), T.diagnosticsText().c_str());
    std::array<int, 7> tDimensions{};
    QVERIFY(FoamValue{*T.find("dimensions")}.asDimensions(tDimensions));
    const std::array<int, 7> expectedT{0, 0, 0, 1, 0, 0, 0};
    for (std::size_t i = 0; i < expectedT.size(); ++i)
    {
        QCOMPARE(tDimensions[i], expectedT[i]);
    }
}

void FoamDictionaryTest::realListConversion()
{
    const FoamDictionary T = parseFixture("field/nonuniformT");
    QVERIFY(T.ok());

    const FoamNode* internalField = T.find("internalField");
    QVERIFY(internalField != nullptr);
    QCOMPARE(static_cast<int>(internalField->kind()), static_cast<int>(FoamNodeKind::Sequence));
    QVERIFY(FoamValue{*internalField}.isNonuniform());
    QCOMPARE(QString::fromStdString(FoamValue{*internalField}.dataNode()->rawText()), QStringLiteral("4"));

    std::vector<double> values;
    QVERIFY(FoamValue{*internalField}.asRealList(values));
    QCOMPARE(values.size(), std::size_t{4});
    QCOMPARE(values[0], 0.0);
    QCOMPARE(values[1], 0.1);
    QCOMPARE(values[2], 0.2);
    QCOMPARE(values[3], 0.3);

    std::vector<double> single;
    QVERIFY(FoamValue{*T.find("boundaryField/walls/value")}.asRealList(single));
    QCOMPARE(single.size(), std::size_t{1});
    QCOMPARE(single[0], 300.0);

    double scalar = 0.0;
    QVERIFY(FoamValue{*T.find("boundaryField/walls/value")}.asReal(scalar));
    QCOMPARE(scalar, 300.0);
}

void FoamDictionaryTest::wordConversions()
{
    const FoamDictionary p = parseFixture("cavity/0/p");
    const std::vector<std::string> words = FoamValue{*p.find("boundaryField/movingWall/type")}.words();
    QCOMPARE(words.size(), std::size_t{1});
    QCOMPARE(QString::fromStdString(words.front()), QStringLiteral("zeroGradient"));

    // `uniform` is a keyword of the field syntax; the payload is not a word.
    const std::vector<std::string> uniformWords = FoamValue{*p.find("internalField")}.words();
    QCOMPARE(uniformWords.size(), std::size_t{1});
    QCOMPARE(QString::fromStdString(uniformWords.front()), QStringLiteral("uniform"));
    QCOMPARE(QString::fromStdString(FoamValue{*p.find("internalField")}.text()), QStringLiteral("uniform 0"));
}

void FoamDictionaryTest::pathQueries()
{
    const FoamDictionary p = parseFixture("cavity/0/p");
    QVERIFY(p.find("boundaryField.movingWall.type") != nullptr);
    QVERIFY(p.find("boundaryField/movingWall/type") != nullptr);
    QVERIFY(p.find("boundaryField..movingWall") != nullptr);
    QVERIFY(p.find("boundaryField/missing") == nullptr);
    QVERIFY(p.find("internalField/nested") == nullptr);
    QCOMPARE(p.find(""), &p.root());

    // findAll() matches direct children of the given dictionary only.
    const auto all = p.findAll("boundaryField/movingWall", "type");
    QCOMPARE(all.size(), std::size_t{1});
    QCOMPARE(QString::fromStdString(FoamValue{*all.front()}.text()), QStringLiteral("zeroGradient"));

    const auto patches = p.findAll("boundaryField", "frontAndBack");
    QCOMPARE(patches.size(), std::size_t{1});
    QCOMPARE(QString::fromStdString(FoamValue{*p.findAll("boundaryField/frontAndBack", "type").front()}.text()),
             QStringLiteral("empty"));
    QVERIFY(p.findAll("boundaryField", "type").empty());

    QVERIFY(p.findAll("boundaryField/movingWall", "type").size() == 1);
    QVERIFY(p.findAll("missing", "type").empty());
    QVERIFY(p.findAll("boundaryField/movingWall/type", "type").empty());
}

void FoamDictionaryTest::sourceTextIsByteExact()
{
    const QString path = fixturePath("cavity/system/blockMeshDict");
    QFile file{path};
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QByteArray expected = file.readAll();
    file.close();

    const FoamDictionary dictionary = parseFixture("cavity/system/blockMeshDict");
    QVERIFY(dictionary.ok());
    QCOMPARE(QByteArray::fromStdString(dictionary.sourceText()), expected);
    QCOMPARE(dictionary.sourceText().size(), static_cast<std::size_t>(expected.size()));

    const std::string canonical = dictionary.toText();
    QVERIFY(canonical.size() != dictionary.sourceText().size());
}

void FoamDictionaryTest::headerAccessors()
{
    const FoamDictionary p = parseFixture("cavity/0/p");
    QVERIFY(p.header() != nullptr);
    QVERIFY(p.hasFoamFileHeader());
    QCOMPARE(QString::fromStdString(p.header()->keyword()), QStringLiteral("FoamFile"));
    QCOMPARE(
        QString::fromStdString(FoamValue{*p.find("FoamFile/object")}.text()),
        QStringLiteral("p"));

    const FoamDictionary noHeader = FoamDictionary::parse("value 1;\n", "inline");
    QVERIFY(noHeader.header() == nullptr);
    QVERIFY(!noHeader.hasFoamFileHeader());
    QCOMPARE(QString::fromStdString(noHeader.fileLabel()), QStringLiteral("inline"));
}

void FoamDictionaryTest::diagnosticsTextFormat()
{
    const FoamDictionary dictionary = parseFixture("invalid/unterminated_string");
    QVERIFY(!dictionary.ok());
    const std::string text = dictionary.diagnosticsText();
    QVERIFY(text.find("IMPORT-LEX-001") != std::string::npos);
    QVERIFY(text.find("error") != std::string::npos);
    QVERIFY(text.find("unterminated_string") != std::string::npos);
    QVERIFY(!text.empty());
    QVERIFY(text.back() == '\n');
    QVERIFY(text.find(":7:") != std::string::npos);

    const FoamDiagnostic* diagnostic = firstWithCode(dictionary.diagnostics(), "IMPORT-LEX-001");
    QVERIFY(diagnostic != nullptr);
    QCOMPARE(static_cast<int>(diagnostic->severity), static_cast<int>(FoamSeverity::Error));
    QVERIFY(!diagnostic->snippet.empty());
}

} // namespace
} // namespace ofgui::io

QTEST_APPLESS_MAIN(ofgui::io::FoamDictionaryTest)

#include "FoamDictionaryTest.moc"
