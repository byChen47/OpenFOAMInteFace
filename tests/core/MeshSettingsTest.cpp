#include "core/MeshSettings.h"

#include <QtTest>

#include <limits>
#include <stdexcept>

namespace ofgui::core {
namespace {

class MeshSettingsTest final : public QObject
{
    Q_OBJECT

private slots:
    void defaultsAreValid();
    void rejectsZeroOrNegativeCells();
    void setCellCountChecksAxis();
    void cellCountRejectsUnknownAxis();
    void rejectsInvalidGrading();
    void rejectsInvalidScale();
    void detectsSingleCellLayer();
    void detectsExcessiveCellBudget();
};

void MeshSettingsTest::defaultsAreValid()
{
    const MeshSettings mesh;
    QVERIFY(mesh.isValid());
    QCOMPARE(mesh.cellCount(0), 10);
    QCOMPARE(mesh.cellCount(1), 10);
    QCOMPARE(mesh.cellCount(2), 10);
    QCOMPARE(static_cast<qulonglong>(mesh.totalCells()), 1000ULL);
    QCOMPARE(mesh.scale(), 1.0);
}

void MeshSettingsTest::rejectsZeroOrNegativeCells()
{
    MeshSettings mesh;
    mesh.setCells({0, 10, 10});
    QVERIFY(!mesh.isValid());
    QVERIFY(!mesh.hasPositiveCells());
    QCOMPARE(static_cast<qlonglong>(mesh.totalCells()), -1LL);

    mesh.setCells({10, -2, 10});
    QVERIFY(!mesh.isValid());
    QVERIFY(!mesh.hasPositiveCells());
}

void MeshSettingsTest::setCellCountChecksAxis()
{
    MeshSettings mesh;
    QVERIFY(mesh.setCellCount(0, 20));
    QCOMPARE(mesh.cellCount(0), 20);
    QVERIFY(!mesh.setCellCount(3, 20));
    QCOMPARE(mesh.cellCount(0), 20);
}

void MeshSettingsTest::cellCountRejectsUnknownAxis()
{
    MeshSettings mesh;
    QVERIFY_THROWS_EXCEPTION(std::out_of_range, static_cast<void>(mesh.cellCount(3)));
}

void MeshSettingsTest::rejectsInvalidGrading()
{
    MeshSettings mesh;
    mesh.setGrading({1.0, 0.0, 1.0});
    QVERIFY(!mesh.isValid());
    QVERIFY(!mesh.hasPositiveGrading());

    mesh.setGrading({1.0, std::numeric_limits<double>::infinity(), 1.0});
    QVERIFY(!mesh.isValid());

    mesh.setGrading({1.0, 2.0, 0.5});
    QVERIFY(mesh.isValid());
}

void MeshSettingsTest::rejectsInvalidScale()
{
    MeshSettings mesh;
    mesh.setScale(0.0);
    QVERIFY(!mesh.isValid());

    mesh.setScale(-1.0);
    QVERIFY(!mesh.isValid());

    mesh.setScale(0.1);
    QVERIFY(mesh.isValid());
}

void MeshSettingsTest::detectsSingleCellLayer()
{
    const MeshSettings mesh(MeshSettings::CellCounts{20, 20, 1});
    QVERIFY(mesh.isSingleCellLayer(2));
    QVERIFY(!mesh.isSingleCellLayer(0));
}

void MeshSettingsTest::detectsExcessiveCellBudget()
{
    const MeshSettings small(MeshSettings::CellCounts{20, 20, 20});
    QVERIFY(!small.exceedsCellBudget());

    const MeshSettings large(MeshSettings::CellCounts{200, 200, 200});
    QCOMPARE(static_cast<qlonglong>(large.totalCells()), 8000000LL);
    QVERIFY(large.exceedsCellBudget());
}

} // namespace
} // namespace ofgui::core

QTEST_APPLESS_MAIN(ofgui::core::MeshSettingsTest)

#include "MeshSettingsTest.moc"
