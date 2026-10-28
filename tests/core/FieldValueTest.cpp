#include "core/FieldValue.h"

#include <QtTest>

#include <stdexcept>

namespace ofgui::core {
namespace {

class FieldValueTest final : public QObject
{
    Q_OBJECT

private slots:
    void defaultIsUnset();
    void storesUniformScalar();
    void storesUniformVector();
    void keepsNonUniformTextVerbatim();
    void rejectsRepresentationMismatch();
};

void FieldValueTest::defaultIsUnset()
{
    const FieldValue value;
    QVERIFY(!value.isValid());
    QVERIFY(!value.isUniformScalar());
    QVERIFY(!value.isUniformVector());
    QVERIFY(!value.isNonUniformRaw());
}

void FieldValueTest::storesUniformScalar()
{
    const FieldValue value = FieldValue::uniformScalar(0.5);
    QVERIFY(value.isValid());
    QVERIFY(value.isUniformScalar());
    QCOMPARE(value.kind(), FieldValueKind::UniformScalar);
    QCOMPARE(value.scalar(), 0.5);
}

void FieldValueTest::storesUniformVector()
{
    const FieldValue value = FieldValue::uniformVector(Vec3d{1.0, 0.0, -1.0});
    QVERIFY(value.isValid());
    QVERIFY(value.isUniformVector());
    QCOMPARE(value.kind(), FieldValueKind::UniformVector);
    QVERIFY(value.vector().isApprox(Vec3d{1.0, 0.0, -1.0}));
}

void FieldValueTest::keepsNonUniformTextVerbatim()
{
    const std::string raw = "nonuniform List<scalar> 3(0.1 0.2 0.3)";
    const FieldValue value = FieldValue::nonUniformRaw(raw);
    QVERIFY(value.isValid());
    QVERIFY(value.isNonUniformRaw());
    QCOMPARE(QString::fromStdString(value.rawText()), QString::fromStdString(raw));
}

void FieldValueTest::rejectsRepresentationMismatch()
{
    const FieldValue scalar = FieldValue::uniformScalar(1.0);
    QVERIFY_THROWS_EXCEPTION(std::logic_error, static_cast<void>(scalar.vector()));
    QVERIFY_THROWS_EXCEPTION(std::logic_error, static_cast<void>(scalar.rawText()));

    const FieldValue raw = FieldValue::nonUniformRaw("nonuniform List<scalar> 1(0)");
    QVERIFY_THROWS_EXCEPTION(std::logic_error, static_cast<void>(raw.scalar()));
}

} // namespace
} // namespace ofgui::core

QTEST_APPLESS_MAIN(ofgui::core::FieldValueTest)

#include "FieldValueTest.moc"
