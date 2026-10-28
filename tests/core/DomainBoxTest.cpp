#include "core/DomainBox.h"

#include <QtTest>

#include <cmath>
#include <limits>

namespace ofgui::core {
namespace {

class DomainBoxTest final : public QObject
{
    Q_OBJECT

private slots:
    void defaultsToThreeDimensionalMetres();
    void storesMinAndMax();
    void computesSizeAndVolume();
    void detectsInvertedAxis();
    void detectsNonFiniteValues();
    void reportsLargestAndSmallestExtent();
    void suggestsTwoDForThinSlab();
    void suggestModeHonoursRelativeThreshold();
};

void DomainBoxTest::defaultsToThreeDimensionalMetres()
{
    const DomainBox box;
    QCOMPARE(box.mode(), DomainMode::ThreeD);
    QCOMPARE(QString::fromStdString(box.unit()), QStringLiteral("m"));
}

void DomainBoxTest::storesMinAndMax()
{
    DomainBox box(Vec3d{-1.0, -2.0, -3.0}, Vec3d{1.0, 2.0, 3.0});
    QVERIFY(box.min().isApprox(Vec3d{-1.0, -2.0, -3.0}));
    QVERIFY(box.max().isApprox(Vec3d{1.0, 2.0, 3.0}));

    box.setMin(Vec3d{0.0, 0.0, 0.0});
    box.setMax(Vec3d{2.0, 2.0, 2.0});
    QVERIFY(box.min().isApprox(Vec3d{0.0, 0.0, 0.0}));
    QVERIFY(box.max().isApprox(Vec3d{2.0, 2.0, 2.0}));
}

void DomainBoxTest::computesSizeAndVolume()
{
    const DomainBox box(Vec3d{0.0, 0.0, 0.0}, Vec3d{2.0, 3.0, 4.0});
    QVERIFY(box.size().isApprox(Vec3d{2.0, 3.0, 4.0}));
    QCOMPARE(box.volume(), 24.0);
    QVERIFY(box.hasPositiveExtent());
}

void DomainBoxTest::detectsInvertedAxis()
{
    const DomainBox box(Vec3d{1.0, 0.0, 0.0}, Vec3d{0.0, 1.0, 1.0});
    QVERIFY(!box.hasPositiveExtent());
    QCOMPARE(box.volume(), 0.0);
    QVERIFY(box.size().x < 0.0);
}

void DomainBoxTest::detectsNonFiniteValues()
{
    DomainBox box(Vec3d{0.0, 0.0, 0.0}, Vec3d{1.0, 1.0, 1.0});
    QVERIFY(box.isFinite());

    box.setMax(Vec3d{std::numeric_limits<double>::quiet_NaN(), 1.0, 1.0});
    QVERIFY(!box.isFinite());
}

void DomainBoxTest::reportsLargestAndSmallestExtent()
{
    const DomainBox box(Vec3d{0.0, 0.0, 0.0}, Vec3d{2.0, 5.0, 1.0});
    QCOMPARE(box.largestExtent(), 5.0);
    QCOMPARE(box.smallestExtent(), 1.0);
}

void DomainBoxTest::suggestsTwoDForThinSlab()
{
    const DomainBox slab(Vec3d{0.0, 0.0, 0.0}, Vec3d{1.0, 1.0, 0.005});
    QCOMPARE(slab.suggestMode(), DomainMode::TwoD);

    const DomainBox cube(Vec3d{0.0, 0.0, 0.0}, Vec3d{1.0, 1.0, 1.0});
    QCOMPARE(cube.suggestMode(), DomainMode::ThreeD);
}

void DomainBoxTest::suggestModeHonoursRelativeThreshold()
{
    const DomainBox box(Vec3d{0.0, 0.0, 0.0}, Vec3d{1.0, 1.0, 0.02});
    QCOMPARE(box.suggestMode(), DomainMode::ThreeD);
    QCOMPARE(box.suggestMode(0.05), DomainMode::TwoD);
}

} // namespace
} // namespace ofgui::core

QTEST_APPLESS_MAIN(ofgui::core::DomainBoxTest)

#include "DomainBoxTest.moc"
