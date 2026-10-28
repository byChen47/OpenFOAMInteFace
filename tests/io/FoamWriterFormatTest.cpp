#include "io/writers/FoamLayout.h"
#include "io/writers/FoamNumberFormat.h"

#include <core/Vec3d.h>

#include <QtTest>

#include <array>
#include <cmath>
#include <string>

namespace ofgui::io {
namespace {

QStringList linesOf(const std::string& text)
{
    return QString::fromUtf8(text.c_str(), static_cast<int>(text.size())).split(QLatin1Char('\n'));
}

} // namespace

class FoamWriterFormatTest : public QObject
{
    Q_OBJECT

private slots:
    void bannerHasSevenAlignedLines();
    void separatorAndTrailerAreUnpadded();
    void fileBlockUsesFixedColumn();
    void entriesAlignToIndentPlusSixteen();
    void longKeywordsFallBackToSingleSpace();
    void realsUseShortestRoundTrip();
    void pointsDropScaleNoise();
    void dimensionsKeepSevenExponents();
    void indentationIsFourSpacesPerLevel();
};

void FoamWriterFormatTest::bannerHasSevenAlignedLines()
{
    const QStringList lines = linesOf(foamBanner());
    QCOMPARE(lines.size(), 8); // seven lines plus the empty tail
    for (int index = 0; index < 7; ++index)
    {
        QCOMPARE(lines.at(index).size(), 79);
    }
    QVERIFY(lines.at(0).startsWith(QStringLiteral("/*----")));
    QVERIFY(lines.at(6).startsWith(QStringLiteral("\\*----")));
}

void FoamWriterFormatTest::separatorAndTrailerAreUnpadded()
{
    const QStringList separator = linesOf(foamSeparator());
    QCOMPARE(separator.size(), 2);
    QCOMPARE(separator.at(0).size(), 79);
    QVERIFY(separator.at(0).startsWith(QStringLiteral("// * *")));

    const QStringList trailer = linesOf(foamTrailer());
    QCOMPARE(trailer.size(), 2);
    QCOMPARE(trailer.at(0).size(), 79);
    QVERIFY(trailer.at(0).endsWith(QStringLiteral("* //")));
}

void FoamWriterFormatTest::fileBlockUsesFixedColumn()
{
    const std::string block = foamFileBlock("volScalarField", "p");
    const QStringList lines = linesOf(block);
    QCOMPARE(lines.at(0), QStringLiteral("FoamFile"));
    QCOMPARE(lines.at(1), QStringLiteral("{"));
    QCOMPARE(lines.at(2), QStringLiteral("    version     2.0;"));
    QCOMPARE(lines.at(3), QStringLiteral("    format      ascii;"));
    QCOMPARE(lines.at(4), QStringLiteral("    class       volScalarField;"));
    QCOMPARE(lines.at(5), QStringLiteral("    object      p;"));
    QCOMPARE(lines.at(6), QStringLiteral("}"));
}

void FoamWriterFormatTest::entriesAlignToIndentPlusSixteen()
{
    QCOMPARE(foamEntryLine(0, "dimensions", "[0 2 -2 0 0 0 0]"), std::string("dimensions      [0 2 -2 0 0 0 0];"));
    QCOMPARE(foamEntryLine(2, "type", "zeroGradient"), std::string("        type            zeroGradient;"));
    QCOMPARE(foamEntryLine(1, "default", "Euler"), std::string("    default         Euler;"));
}

void FoamWriterFormatTest::longKeywordsFallBackToSingleSpace()
{
    QCOMPARE(
        foamEntryLine(1, "nNonOrthogonalCorrectors", "0"),
        std::string("    nNonOrthogonalCorrectors 0;"));
}

void FoamWriterFormatTest::realsUseShortestRoundTrip()
{
    QCOMPARE(formatFoamReal(0.5), std::string("0.5"));
    QCOMPARE(formatFoamReal(0.005), std::string("0.005"));
    QCOMPARE(formatFoamReal(0.0), std::string("0"));
    QCOMPARE(formatFoamReal(20.0), std::string("20"));
    QCOMPARE(formatFoamReal(1e-06), std::string("1e-06"));
    QCOMPARE(formatFoamReal(2.5e-05), std::string("2.5e-05"));
}

void FoamWriterFormatTest::pointsDropScaleNoise()
{
    // `1 * 0.1 * 0.1 / 0.1` round-trips through binary floating point.
    const double noisy = 1.0 * 0.1 * 0.1 / 0.1;
    QCOMPARE(formatFoamPoint(core::Vec3d{0.0, 1.0, noisy}), std::string("(0 1 0.1)"));
    QVERIFY(formatFoamReal(noisy) != std::string("0.1"));
}

void FoamWriterFormatTest::dimensionsKeepSevenExponents()
{
    QCOMPARE(formatFoamDimensions(std::array<int, 7>{0, 2, -2, 0, 0, 0, 0}), std::string("[0 2 -2 0 0 0 0]"));
    QCOMPARE(formatFoamDimensions(std::array<int, 7>{1, -3, 0, 0, 0, 0, 0}), std::string("[1 -3 0 0 0 0 0]"));
}

void FoamWriterFormatTest::indentationIsFourSpacesPerLevel()
{
    QCOMPARE(foamIndent(0), std::string{});
    QCOMPARE(foamIndent(1), std::string("    "));
    QCOMPARE(foamIndent(3), std::string("            "));
}

} // namespace ofgui::io

QTEST_APPLESS_MAIN(ofgui::io::FoamWriterFormatTest)
#include "FoamWriterFormatTest.moc"
