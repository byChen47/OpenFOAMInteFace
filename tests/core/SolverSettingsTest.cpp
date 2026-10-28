#include "core/SolverSettings.h"

#include <QtTest>

#include <limits>

namespace ofgui::core {
namespace {

class SolverSettingsTest final : public QObject
{
    Q_OBJECT

private slots:
    void defaultsAreValid();
    void storesConfiguredValues();
    void requiresIncreasingTimeRange();
    void requiresPositiveDeltaT();
    void requiresPositiveWriteInterval();
};

void SolverSettingsTest::defaultsAreValid()
{
    const SolverSettings solver;
    QVERIFY(solver.application().empty());
    QCOMPARE(solver.startTime(), 0.0);
    QCOMPARE(solver.endTime(), 1.0);
    QCOMPARE(solver.deltaT(), 0.001);
    QCOMPARE(solver.writeInterval(), 100);
    QVERIFY(solver.hasValidTimeRange());
    QVERIFY(solver.isValid());
}

void SolverSettingsTest::storesConfiguredValues()
{
    SolverSettings solver;
    solver.setApplication("icoFoam");
    solver.setStartTime(0.5);
    solver.setEndTime(2.0);
    solver.setDeltaT(0.002);
    solver.setWriteInterval(50);

    QCOMPARE(QString::fromStdString(solver.application()), QStringLiteral("icoFoam"));
    QCOMPARE(solver.startTime(), 0.5);
    QCOMPARE(solver.endTime(), 2.0);
    QCOMPARE(solver.deltaT(), 0.002);
    QCOMPARE(solver.writeInterval(), 50);
    QVERIFY(solver.isValid());
}

void SolverSettingsTest::requiresIncreasingTimeRange()
{
    SolverSettings solver;
    solver.setEndTime(0.0);
    QVERIFY(!solver.hasValidTimeRange());
    QVERIFY(!solver.isValid());

    solver.setStartTime(1.0);
    solver.setEndTime(0.5);
    QVERIFY(!solver.hasValidTimeRange());

    solver.setStartTime(-1.0);
    solver.setEndTime(0.0);
    QVERIFY(solver.hasValidTimeRange());
}

void SolverSettingsTest::requiresPositiveDeltaT()
{
    SolverSettings solver;
    solver.setDeltaT(0.0);
    QVERIFY(!solver.isValid());

    solver.setDeltaT(-0.001);
    QVERIFY(!solver.isValid());

    solver.setDeltaT(std::numeric_limits<double>::quiet_NaN());
    QVERIFY(!solver.isValid());
}

void SolverSettingsTest::requiresPositiveWriteInterval()
{
    SolverSettings solver;
    solver.setWriteInterval(0);
    QVERIFY(!solver.isValid());

    solver.setWriteInterval(1);
    QVERIFY(solver.isValid());
}

} // namespace
} // namespace ofgui::core

QTEST_APPLESS_MAIN(ofgui::core::SolverSettingsTest)

#include "SolverSettingsTest.moc"
