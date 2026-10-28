#include "io/FoamDiagnostic.h"
#include "io/FoamLexer.h"
#include "io/FoamToken.h"

#include <QtTest>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace ofgui::io {
namespace {

struct LexResult
{
    std::vector<FoamToken> tokens;
    std::vector<FoamDiagnostic> diagnostics;
};

LexResult lexAll(std::string_view source, std::string label = "test")
{
    FoamLexer lexer{source, std::move(label)};
    LexResult result;
    while (auto token = lexer.next())
    {
        result.tokens.push_back(std::move(*token));
    }
    result.diagnostics = lexer.diagnostics();
    return result;
}

bool hasCode(const std::vector<FoamDiagnostic>& diagnostics, std::string_view code)
{
    for (const FoamDiagnostic& diagnostic : diagnostics)
    {
        if (diagnostic.code == code)
        {
            return true;
        }
    }
    return false;
}

const FoamDiagnostic* findByCode(const std::vector<FoamDiagnostic>& diagnostics, std::string_view code)
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

class FoamLexerTest : public QObject
{
    Q_OBJECT

private slots:
    void wordsAndPunctuation();
    void lineCommentsAndBlockComments();
    void unterminatedBlockComment();
    void stringsAndEscapes();
    void unterminatedString();
    void numbers();
    void malformedNumber();
    void variables();
    void loneDollar();
    void wordsWithParentheses();
    void wordWithUnbalancedParenthesis();
    void verbatimBlock();
    void verbatimBlockUnterminated();
    void directives();
    void tokenPositions();
    void invalidCharacter();
    void malformedNumberKeepsPosition();
};

void FoamLexerTest::wordsAndPunctuation()
{
    const LexResult result = lexAll("key value;\na { b ( c ) [ d ] : , = + * / }");
    QVERIFY(result.diagnostics.empty());

    const std::vector<FoamTokenKind> expected{
        FoamTokenKind::Word,
        FoamTokenKind::Word,
        FoamTokenKind::Semicolon,
        FoamTokenKind::Word,
        FoamTokenKind::OpenBrace,
        FoamTokenKind::Word,
        FoamTokenKind::OpenParen,
        FoamTokenKind::Word,
        FoamTokenKind::CloseParen,
        FoamTokenKind::OpenBracket,
        FoamTokenKind::Word,
        FoamTokenKind::CloseBracket,
        FoamTokenKind::Colon,
        FoamTokenKind::Comma,
        FoamTokenKind::Assign,
        FoamTokenKind::Plus,
        FoamTokenKind::Multiply,
        FoamTokenKind::Divide,
        FoamTokenKind::CloseBrace,
    };
    QCOMPARE(result.tokens.size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i)
    {
        QCOMPARE(static_cast<int>(result.tokens[i].kind), static_cast<int>(expected[i]));
    }
    QCOMPARE(QString::fromStdString(result.tokens[0].text), QStringLiteral("key"));
    QCOMPARE(QString::fromStdString(result.tokens[1].text), QStringLiteral("value"));
    QCOMPARE(result.tokens[2].punctuation(), ';');
    QCOMPARE(result.tokens[4].punctuation(), '{');
    QCOMPARE(result.tokens[18].punctuation(), '}');
}

void FoamLexerTest::lineCommentsAndBlockComments()
{
    const LexResult result = lexAll(
        "a // trailing comment with ; and {\n"
        "b /* block\n comment\n */ c\n"
        "d\n");
    QVERIFY(result.diagnostics.empty());
    QCOMPARE(result.tokens.size(), std::size_t{4});
    QCOMPARE(QString::fromStdString(result.tokens[0].text), QStringLiteral("a"));
    QCOMPARE(QString::fromStdString(result.tokens[1].text), QStringLiteral("b"));
    QCOMPARE(QString::fromStdString(result.tokens[2].text), QStringLiteral("c"));
    QCOMPARE(QString::fromStdString(result.tokens[3].text), QStringLiteral("d"));
    QCOMPARE(result.tokens[2].line, 4);
}

void FoamLexerTest::unterminatedBlockComment()
{
    const LexResult result = lexAll("a /* never closed\nvalue 1;\n");
    QCOMPARE(result.tokens.size(), std::size_t{1});
    QVERIFY(hasCode(result.diagnostics, "IMPORT-LEX-002"));
    const FoamDiagnostic* diagnostic = findByCode(result.diagnostics, "IMPORT-LEX-002");
    QVERIFY(diagnostic != nullptr);
    QCOMPARE(diagnostic->location.line, 1);
    QCOMPARE(diagnostic->location.column, 3);
}

void FoamLexerTest::stringsAndEscapes()
{
    const LexResult result = lexAll(R"("a\"b" "C:\temp\file" "")");
    QVERIFY(result.diagnostics.empty());
    QCOMPARE(result.tokens.size(), std::size_t{3});
    QCOMPARE(QString::fromStdString(result.tokens[0].text), QStringLiteral("a\"b"));
    QCOMPARE(
        QString::fromStdString(result.tokens[1].text), QStringLiteral("C:\\temp\\file"));
    QVERIFY(result.tokens[2].text.empty());
    for (const FoamToken& token : result.tokens)
    {
        QCOMPARE(static_cast<int>(token.kind), static_cast<int>(FoamTokenKind::String));
    }
}

void FoamLexerTest::unterminatedString()
{
    const LexResult newlineCase = lexAll("value \"abc\nnext 1;\n");
    QVERIFY(hasCode(newlineCase.diagnostics, "IMPORT-LEX-001"));
    const FoamDiagnostic* diagnostic = findByCode(newlineCase.diagnostics, "IMPORT-LEX-001");
    QVERIFY(diagnostic != nullptr);
    QCOMPARE(diagnostic->location.line, 1);
    // Recovery: the string token is emitted, parsing continues on the next line.
    QCOMPARE(newlineCase.tokens.size(), std::size_t{5});
    QCOMPARE(QString::fromStdString(newlineCase.tokens[2].text), QStringLiteral("next"));

    const LexResult eofCase = lexAll("value \"abc");
    QVERIFY(hasCode(eofCase.diagnostics, "IMPORT-LEX-001"));
}

void FoamLexerTest::numbers()
{
    const LexResult result = lexAll("0 1 -3 1e-06 -2.5e+3 .5 20");
    QVERIFY(result.diagnostics.empty());
    QCOMPARE(result.tokens.size(), std::size_t{7});
    const char* expected[] = {"0", "1", "-3", "1e-06", "-2.5e+3", ".5", "20"};
    for (std::size_t i = 0; i < 7; ++i)
    {
        QCOMPARE(static_cast<int>(result.tokens[i].kind), static_cast<int>(FoamTokenKind::Number));
        QCOMPARE(QString::fromStdString(result.tokens[i].text), QString::fromLatin1(expected[i]));
    }

    double value = 0.0;
    QVERIFY(parseFoamReal(result.tokens[3].text, value));
    QCOMPARE(value, 1e-06);
    long long integer = 0;
    QVERIFY(parseFoamInt(result.tokens[6].text, integer));
    QCOMPARE(integer, 20LL);

    // '+' is a punctuation token, not a number sign, matching ISstream::read.
    const LexResult plusCase = lexAll("+2");
    QCOMPARE(plusCase.tokens.size(), std::size_t{2});
    QCOMPARE(static_cast<int>(plusCase.tokens[0].kind), static_cast<int>(FoamTokenKind::Plus));

    // A lone '-' is the minus punctuation token.
    const LexResult minusCase = lexAll("- ;");
    QCOMPARE(minusCase.tokens.size(), std::size_t{2});
    QCOMPARE(static_cast<int>(minusCase.tokens[0].kind), static_cast<int>(FoamTokenKind::Minus));
}

void FoamLexerTest::malformedNumber()
{
    const LexResult result = lexAll("1.2.3;");
    QCOMPARE(result.tokens.size(), std::size_t{2});
    QCOMPARE(static_cast<int>(result.tokens[0].kind), static_cast<int>(FoamTokenKind::Word));
    QCOMPARE(QString::fromStdString(result.tokens[0].text), QStringLiteral("1.2.3"));
    QVERIFY(hasCode(result.diagnostics, "IMPORT-LEX-006"));
}

void FoamLexerTest::variables()
{
    const LexResult result = lexAll("$p $internalField ${name} $.x ${{ 1 + 2 }}");
    QVERIFY(result.diagnostics.empty());
    QCOMPARE(result.tokens.size(), std::size_t{5});
    QCOMPARE(QString::fromStdString(result.tokens[0].text), QStringLiteral("$p"));
    QCOMPARE(QString::fromStdString(result.tokens[1].text), QStringLiteral("$internalField"));
    QCOMPARE(QString::fromStdString(result.tokens[2].text), QStringLiteral("${name}"));
    QCOMPARE(QString::fromStdString(result.tokens[3].text), QStringLiteral("$.x"));
    QCOMPARE(QString::fromStdString(result.tokens[4].text), QStringLiteral("${{ 1 + 2 }}"));

    for (std::size_t i = 0; i < 4; ++i)
    {
        QCOMPARE(
            static_cast<int>(result.tokens[i].kind), static_cast<int>(FoamTokenKind::Variable));
    }
    QCOMPARE(static_cast<int>(result.tokens[4].kind), static_cast<int>(FoamTokenKind::Expression));

    // The lexer stops at the documented delimiters: `$p;` yields a variable then ';'.
    const LexResult terminated = lexAll("value $p;");
    QCOMPARE(terminated.tokens.size(), std::size_t{3});
    QCOMPARE(QString::fromStdString(terminated.tokens[1].text), QStringLiteral("$p"));
    QCOMPARE(static_cast<int>(terminated.tokens[2].kind), static_cast<int>(FoamTokenKind::Semicolon));
}

void FoamLexerTest::loneDollar()
{
    const LexResult result = lexAll("$ ;");
    QCOMPARE(result.tokens.size(), std::size_t{2});
    QCOMPARE(static_cast<int>(result.tokens[0].kind), static_cast<int>(FoamTokenKind::Word));
    QCOMPARE(QString::fromStdString(result.tokens[0].text), QStringLiteral("$"));
    QVERIFY(hasCode(result.diagnostics, "IMPORT-LEX-007"));
}

void FoamLexerTest::wordsWithParentheses()
{
    const LexResult result = lexAll("grad(p) div(phi,U) simpleGrading (1 1 1)");
    QVERIFY(result.diagnostics.empty());
    QCOMPARE(result.tokens.size(), std::size_t{8});
    QCOMPARE(QString::fromStdString(result.tokens[0].text), QStringLiteral("grad(p)"));
    QCOMPARE(QString::fromStdString(result.tokens[1].text), QStringLiteral("div(phi,U)"));
    QCOMPARE(QString::fromStdString(result.tokens[2].text), QStringLiteral("simpleGrading"));
    QCOMPARE(static_cast<int>(result.tokens[3].kind), static_cast<int>(FoamTokenKind::OpenParen));
    QCOMPARE(static_cast<int>(result.tokens[7].kind), static_cast<int>(FoamTokenKind::CloseParen));
}

void FoamLexerTest::wordWithUnbalancedParenthesis()
{
    const LexResult result = lexAll("foo(1\nbar 2;");
    QVERIFY(hasCode(result.diagnostics, "IMPORT-LEX-005"));
    QCOMPARE(result.tokens.size(), std::size_t{4});
    QCOMPARE(QString::fromStdString(result.tokens[0].text), QStringLiteral("foo(1"));
}

void FoamLexerTest::verbatimBlock()
{
    const LexResult result = lexAll(
        "code\n"
        "#{\n"
        "    const vector axis(1, 0, 0);\n"
        "    // comment with ; and # not closing\n"
        "    operator==(v);\n"
        "#};\n");
    QVERIFY(result.diagnostics.empty());
    QCOMPARE(result.tokens.size(), std::size_t{3});
    QCOMPARE(static_cast<int>(result.tokens[1].kind), static_cast<int>(FoamTokenKind::Verbatim));
    const std::string& text = result.tokens[1].text;
    QVERIFY(text.find("const vector axis(1, 0, 0);") != std::string::npos);
    QVERIFY(text.find("// comment with ; and # not closing") != std::string::npos);
    QVERIFY(text.find("#{") == std::string::npos);
    QVERIFY(text.find("#}") == std::string::npos);
    QCOMPARE(result.tokens[1].line, 2);
    QCOMPARE(static_cast<int>(result.tokens[2].kind), static_cast<int>(FoamTokenKind::Semicolon));
}

void FoamLexerTest::verbatimBlockUnterminated()
{
    const LexResult result = lexAll("code\n#{\nint i = 0;\n");
    QVERIFY(hasCode(result.diagnostics, "IMPORT-LEX-003"));
    QCOMPARE(result.tokens.size(), std::size_t{2});
    QCOMPARE(static_cast<int>(result.tokens[1].kind), static_cast<int>(FoamTokenKind::Verbatim));
}

void FoamLexerTest::directives()
{
    const LexResult result = lexAll("#includeEtc \"caseDicts/setConstraintTypes\"\nkey value;\n");
    QVERIFY(result.diagnostics.empty());
    QCOMPARE(result.tokens.size(), std::size_t{5});
    QCOMPARE(QString::fromStdString(result.tokens[0].text), QStringLiteral("#includeEtc"));
    QCOMPARE(static_cast<int>(result.tokens[1].kind), static_cast<int>(FoamTokenKind::String));
    QCOMPARE(QString::fromStdString(result.tokens[1].text), QStringLiteral("caseDicts/setConstraintTypes"));
}

void FoamLexerTest::tokenPositions()
{
    const std::string source = "first 1;\n    second 2;\n";
    const LexResult result = lexAll(source);
    QVERIFY(result.diagnostics.empty());
    QCOMPARE(result.tokens.size(), std::size_t{6});

    const FoamToken& second = result.tokens[3];
    QCOMPARE(QString::fromStdString(second.text), QStringLiteral("second"));
    QCOMPARE(second.line, 2);
    QCOMPARE(second.column, 5);
    QCOMPARE(second.byteOffset, source.find("second"));

    const FoamSourceLocation location = second.location("case/system/controlDict");
    QCOMPARE(QString::fromStdString(location.toString()), QStringLiteral("case/system/controlDict:2:5"));
}

void FoamLexerTest::invalidCharacter()
{
    const std::string source = std::string{"a\x01"} + "b";
    const LexResult result = lexAll(source);
    QVERIFY(hasCode(result.diagnostics, "IMPORT-LEX-004"));
    QCOMPARE(result.tokens.size(), std::size_t{2});
    QCOMPARE(QString::fromStdString(result.tokens[0].text), QStringLiteral("a"));
    QCOMPARE(QString::fromStdString(result.tokens[1].text), QStringLiteral("b"));
}

void FoamLexerTest::malformedNumberKeepsPosition()
{
    const LexResult result = lexAll("key 1e;\n");
    QVERIFY(hasCode(result.diagnostics, "IMPORT-LEX-006"));
    const FoamDiagnostic* diagnostic = findByCode(result.diagnostics, "IMPORT-LEX-006");
    QVERIFY(diagnostic != nullptr);
    QCOMPARE(diagnostic->location.line, 1);
    QCOMPARE(diagnostic->location.column, 5);
    QVERIFY(!diagnostic->snippet.empty());

    // `1e` is a word once the number parse fails, so the entry still parses.
    QCOMPARE(result.tokens.size(), std::size_t{3});
    QCOMPARE(QString::fromStdString(result.tokens[1].text), QStringLiteral("1e"));
}

} // namespace
} // namespace ofgui::io

QTEST_APPLESS_MAIN(ofgui::io::FoamLexerTest)

#include "FoamLexerTest.moc"
