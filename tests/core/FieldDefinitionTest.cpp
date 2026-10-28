#include "core/FieldDefinition.h"

#include <QtTest>

#include <string>
#include <vector>

namespace ofgui::core {
namespace {

class FieldDefinitionTest final : public QObject
{
    Q_OBJECT

private slots:
    void storesMetadata();
    void addsReplacesAndRemovesBoundaryConditions();
    void reportsPatchesWithoutCondition();
    void mapsFieldClassNames();
    void internalFieldStartsUnset();
};

void FieldDefinitionTest::storesMetadata()
{
    const FieldDefinition field("p", FieldClass::VolScalarField, FieldDefinition::Dimensions{0, 2, -2, 0, 0, 0, 0});
    QCOMPARE(QString::fromStdString(field.name()), QStringLiteral("p"));
    QCOMPARE(field.fieldClass(), FieldClass::VolScalarField);
    QCOMPARE(field.dimensions().at(1), 2);
    QCOMPARE(field.dimensions().at(2), -2);
    QVERIFY(!field.id().isValid());
}

void FieldDefinitionTest::addsReplacesAndRemovesBoundaryConditions()
{
    FieldDefinition field("U", FieldClass::VolVectorField);
    const ObjectId inlet{0x11};
    const ObjectId outlet{0x22};

    field.setBoundaryCondition(inlet, BoundaryCondition("fixedValue"));
    QVERIFY(field.hasBoundaryCondition(inlet));
    QVERIFY(!field.hasBoundaryCondition(outlet));
    QVERIFY(field.findBoundaryCondition(inlet) != nullptr);
    QVERIFY(field.findBoundaryCondition(outlet) == nullptr);

    field.setBoundaryCondition(inlet, BoundaryCondition("noSlip"));
    QCOMPARE(static_cast<int>(field.boundaryConditions().size()), 1);
    QCOMPARE(
        QString::fromStdString(field.findBoundaryCondition(inlet)->type()),
        QStringLiteral("noSlip"));

    field.setBoundaryCondition(outlet, BoundaryCondition("zeroGradient"));
    QCOMPARE(static_cast<int>(field.boundaryConditions().size()), 2);

    QVERIFY(field.removeBoundaryCondition(inlet));
    QVERIFY(!field.removeBoundaryCondition(inlet));
    QCOMPARE(static_cast<int>(field.boundaryConditions().size()), 1);
}

void FieldDefinitionTest::reportsPatchesWithoutCondition()
{
    FieldDefinition field("p", FieldClass::VolScalarField);
    const ObjectId first{0x01};
    const ObjectId second{0x02};
    const ObjectId third{0x03};
    const std::vector<ObjectId> allPatches{first, second, third};

    field.setBoundaryCondition(second, BoundaryCondition("zeroGradient"));

    const std::vector<ObjectId> missing = field.patchesWithoutCondition(allPatches);
    QCOMPARE(static_cast<int>(missing.size()), 2);
    QVERIFY(missing.at(0) == first);
    QVERIFY(missing.at(1) == third);

    QVERIFY(field.patchesWithoutCondition({second}).empty());
}

void FieldDefinitionTest::mapsFieldClassNames()
{
    QCOMPARE(
        QString::fromStdString(std::string{fieldClassName(FieldClass::VolScalarField)}),
        QStringLiteral("volScalarField"));
    QCOMPARE(
        QString::fromStdString(std::string{fieldClassName(FieldClass::VolVectorField)}),
        QStringLiteral("volVectorField"));

    const auto scalar = fieldClassFromName("volScalarField");
    QVERIFY(scalar.has_value());
    QCOMPARE(*scalar, FieldClass::VolScalarField);

    QVERIFY(!fieldClassFromName("surfaceScalarField").has_value());
}

void FieldDefinitionTest::internalFieldStartsUnset()
{
    FieldDefinition field("p", FieldClass::VolScalarField);
    QVERIFY(!field.internalField().isValid());

    field.setInternalField(FieldValue::uniformScalar(0.0));
    QVERIFY(field.internalField().isValid());
}

} // namespace
} // namespace ofgui::core

QTEST_APPLESS_MAIN(ofgui::core::FieldDefinitionTest)

#include "FieldDefinitionTest.moc"
