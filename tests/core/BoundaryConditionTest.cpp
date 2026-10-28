#include "core/BoundaryCondition.h"

#include <QtTest>

#include <string>
#include <string_view>

namespace ofgui::core {
namespace {

class BoundaryConditionTest final : public QObject
{
    Q_OBJECT

private slots:
    void defaultIsEmpty();
    void storesTypeAndParameters();
    void removesParameters();
    void recognisesKnownTypes();
    void looksUpParametersWithStringView();
};

void BoundaryConditionTest::defaultIsEmpty()
{
    const BoundaryCondition condition;
    QVERIFY(condition.isEmpty());
    QVERIFY(!condition.isKnownType());
    QVERIFY(condition.parameters().empty());
}

void BoundaryConditionTest::storesTypeAndParameters()
{
    BoundaryCondition condition("fixedValue");
    QVERIFY(!condition.isEmpty());
    QCOMPARE(QString::fromStdString(condition.type()), QStringLiteral("fixedValue"));

    condition.setParameter("value", "uniform (0 0 0)");
    QVERIFY(condition.hasParameter("value"));
    QCOMPARE(
        QString::fromStdString(condition.parameter("value").value_or(std::string{})),
        QStringLiteral("uniform (0 0 0)"));
    QCOMPARE(static_cast<int>(condition.parameters().size()), 1);

    condition.setParameter("value", "uniform (1 0 0)");
    QCOMPARE(static_cast<int>(condition.parameters().size()), 1);
    QCOMPARE(
        QString::fromStdString(condition.parameter("value").value_or(std::string{})),
        QStringLiteral("uniform (1 0 0)"));

    condition.setType("noSlip");
    QCOMPARE(QString::fromStdString(condition.type()), QStringLiteral("noSlip"));
}

void BoundaryConditionTest::removesParameters()
{
    BoundaryCondition condition("inletOutlet");
    condition.setParameter("inletValue", "uniform (0 0 0)");

    QVERIFY(condition.removeParameter("inletValue"));
    QVERIFY(!condition.removeParameter("inletValue"));
    QVERIFY(!condition.hasParameter("inletValue"));
    QVERIFY(!condition.parameter("inletValue").has_value());
}

void BoundaryConditionTest::recognisesKnownTypes()
{
    QVERIFY(BoundaryCondition::isKnownTypeName("zeroGradient"));
    QVERIFY(BoundaryCondition::isKnownTypeName("fixedValue"));
    QVERIFY(BoundaryCondition::isKnownTypeName("empty"));
    QVERIFY(BoundaryCondition::isKnownTypeName("noSlip"));
    QVERIFY(BoundaryCondition::isKnownTypeName("inletOutlet"));
    QVERIFY(!BoundaryCondition::isKnownTypeName("someUserDefinedType"));
    QVERIFY(!BoundaryCondition::isKnownTypeName(""));

    const BoundaryCondition known("fixedValue");
    QVERIFY(known.isKnownType());

    const BoundaryCondition unknown("someUserDefinedType");
    QVERIFY(!unknown.isKnownType());
}

void BoundaryConditionTest::looksUpParametersWithStringView()
{
    BoundaryCondition condition("fixedValue");
    condition.setParameter("value", "uniform 1");

    const std::string_view name = "value";
    QVERIFY(condition.hasParameter(name));
    QVERIFY(condition.parameter(name).has_value());
}

} // namespace
} // namespace ofgui::core

QTEST_APPLESS_MAIN(ofgui::core::BoundaryConditionTest)

#include "BoundaryConditionTest.moc"
