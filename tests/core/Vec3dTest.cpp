#include "core/Vec3d.h"

#include <QtTest>

#include <cmath>
#include <limits>
#include <stdexcept>

namespace ofgui::core {
namespace {

class Vec3dTest final : public QObject
{
    Q_OBJECT

private slots:
    void defaultConstructsToOrigin();
    void arithmeticIsComponentWise();
    void compoundAssignmentMatchesBinaryOperators();
    void dotProductAndLength();
    void normalizesNonZeroVector();
    void normalizingZeroVectorStaysZero();
    void accessesComponentsByAxis();
    void rejectsUnknownAxis();
    void detectsNonFiniteComponents();
    void approximateEqualityUsesAbsoluteAndRelativeTolerance();
    void computesComponentWiseExtremes();
};

void Vec3dTest::defaultConstructsToOrigin()
{
    const Vec3d value;
    QCOMPARE(value.x, 0.0);
    QCOMPARE(value.y, 0.0);
    QCOMPARE(value.z, 0.0);
    QVERIFY(value.isApprox(Vec3d::zero()));

    const Vec3d filled = Vec3d::filled(2.0);
    QVERIFY(filled.isApprox(Vec3d{2.0, 2.0, 2.0}));
}

void Vec3dTest::arithmeticIsComponentWise()
{
    const Vec3d lhs{1.0, 2.0, 3.0};
    const Vec3d rhs{-1.0, 0.5, 2.0};

    const Vec3d sum = lhs + rhs;
    QCOMPARE(sum.x, 0.0);
    QCOMPARE(sum.y, 2.5);
    QCOMPARE(sum.z, 5.0);

    const Vec3d difference = lhs - rhs;
    QCOMPARE(difference.x, 2.0);
    QCOMPARE(difference.y, 1.5);
    QCOMPARE(difference.z, 1.0);

    const Vec3d negated = -lhs;
    QCOMPARE(negated.x, -1.0);
    QCOMPARE(negated.y, -2.0);
    QCOMPARE(negated.z, -3.0);

    const Vec3d scaled = lhs * 2.0;
    QCOMPARE(scaled.x, 2.0);
    QCOMPARE(scaled.y, 4.0);
    QCOMPARE(scaled.z, 6.0);

    const Vec3d divided = lhs / 2.0;
    QCOMPARE(divided.x, 0.5);
    QCOMPARE(divided.y, 1.0);
    QCOMPARE(divided.z, 1.5);
}

void Vec3dTest::compoundAssignmentMatchesBinaryOperators()
{
    Vec3d value{1.0, 2.0, 3.0};

    value += Vec3d{1.0, 1.0, 1.0};
    value -= Vec3d{0.0, 1.0, 2.0};
    value *= 2.0;
    value /= 4.0;

    QVERIFY(value.isApprox(Vec3d{1.0, 1.0, 1.0}));
}

void Vec3dTest::dotProductAndLength()
{
    const Vec3d value{3.0, 4.0, 0.0};
    const Vec3d xAxis{1.0, 0.0, 0.0};
    const Vec3d zAxis{0.0, 0.0, 1.0};

    QCOMPARE(value.dot(xAxis), 3.0);
    QCOMPARE(value.squaredLength(), 25.0);
    QCOMPARE(value.length(), 5.0);
    QCOMPARE(zAxis.dot(zAxis), 1.0);
}

void Vec3dTest::normalizesNonZeroVector()
{
    const Vec3d unit = Vec3d{3.0, 4.0, 0.0}.normalized();

    QVERIFY(unit.isApprox(Vec3d{0.6, 0.8, 0.0}));
    QCOMPARE(unit.length(), 1.0);
}

void Vec3dTest::normalizingZeroVectorStaysZero()
{
    const Vec3d unit = Vec3d::zero().normalized();

    QVERIFY(unit.isApprox(Vec3d::zero()));
    QVERIFY(unit.isFinite());
}

void Vec3dTest::accessesComponentsByAxis()
{
    const Vec3d value{1.5, -2.5, 3.5};

    QCOMPARE(value.component(0), 1.5);
    QCOMPARE(value.component(1), -2.5);
    QCOMPARE(value.component(2), 3.5);

    const std::array<Real, 3> components = value.asArray();
    QCOMPARE(components.at(0), 1.5);
    QCOMPARE(components.at(2), 3.5);
}

void Vec3dTest::rejectsUnknownAxis()
{
    const Vec3d value{1.0, 2.0, 3.0};

    QVERIFY_THROWS_EXCEPTION(std::out_of_range, static_cast<void>(value.component(3)));
}

void Vec3dTest::detectsNonFiniteComponents()
{
    const Real infinity = std::numeric_limits<Real>::infinity();
    const Real notANumber = std::numeric_limits<Real>::quiet_NaN();

    const Vec3d finite{1.0, 2.0, 3.0};
    const Vec3d infinite{infinity, 0.0, 0.0};
    const Vec3d notANumberVector{0.0, notANumber, 0.0};
    const Vec3d negativeInfinite{0.0, 0.0, -infinity};

    QVERIFY(finite.isFinite());
    QVERIFY(!infinite.isFinite());
    QVERIFY(!notANumberVector.isFinite());
    QVERIFY(!negativeInfinite.isFinite());
}

void Vec3dTest::approximateEqualityUsesAbsoluteAndRelativeTolerance()
{
    const Vec3d zero = Vec3d::zero();
    const Vec3d tiny{1.0e-9, 0.0, 0.0};
    const Vec3d oneMillion{1.0e6, 0.0, 0.0};
    const Vec3d oneMillionPlusTiny{1.0e6 + 1.0e-3, 0.0, 0.0};
    const Vec3d onePointOneMillion{1.1e6, 0.0, 0.0};

    QVERIFY(zero.isApprox(zero));
    QVERIFY(!tiny.isApprox(zero));
    QVERIFY(tiny.isApprox(zero, 1.0e-8));
    QVERIFY(!oneMillion.isApprox(oneMillionPlusTiny));
    QVERIFY(oneMillion.isApprox(oneMillionPlusTiny, 1.0e-8));
    QVERIFY(!oneMillion.isApprox(onePointOneMillion));
}

void Vec3dTest::computesComponentWiseExtremes()
{
    const Vec3d lhs{1.0, 5.0, -3.0};
    const Vec3d rhs{2.0, -1.0, -4.0};

    const Vec3d minimum = componentwiseMin(lhs, rhs);
    QCOMPARE(minimum.x, 1.0);
    QCOMPARE(minimum.y, -1.0);
    QCOMPARE(minimum.z, -4.0);

    const Vec3d maximum = componentwiseMax(lhs, rhs);
    QCOMPARE(maximum.x, 2.0);
    QCOMPARE(maximum.y, 5.0);
    QCOMPARE(maximum.z, -3.0);
}

} // namespace
} // namespace ofgui::core

QTEST_APPLESS_MAIN(ofgui::core::Vec3dTest)

#include "Vec3dTest.moc"
