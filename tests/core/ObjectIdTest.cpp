#include "core/ObjectId.h"

#include <QtTest>

#include <set>
#include <string>
#include <vector>

namespace ofgui::core {
namespace {

class ObjectIdTest final : public QObject
{
    Q_OBJECT

private slots:
    void defaultConstructedIdIsInvalid();
    void createReturnsDistinctValidIds();
    void textFormRoundTrips();
    void textFormIsZeroPadded();
    void acceptsHexPrefix();
    void rejectsMalformedText();
    void comparesById();
};

void ObjectIdTest::defaultConstructedIdIsInvalid()
{
    const ObjectId id;
    QVERIFY(!id.isValid());
    QCOMPARE(static_cast<qulonglong>(id.value()), 0ULL);
}

void ObjectIdTest::createReturnsDistinctValidIds()
{
    constexpr int kSampleCount = 1000;
    std::set<std::uint64_t> values;
    for (int index = 0; index < kSampleCount; ++index)
    {
        const ObjectId id = ObjectId::create();
        QVERIFY(id.isValid());
        values.insert(id.value());
    }

    QCOMPARE(static_cast<int>(values.size()), kSampleCount);
}

void ObjectIdTest::textFormRoundTrips()
{
    const ObjectId original{0x00C0FFEEull};
    const auto parsed = ObjectId::fromString(original.toString());
    QVERIFY(parsed.has_value());
    QVERIFY(*parsed == original);
}

void ObjectIdTest::textFormIsZeroPadded()
{
    QCOMPARE(QString::fromStdString(ObjectId{0x2A}.toString()), QStringLiteral("000000000000002a"));
}

void ObjectIdTest::acceptsHexPrefix()
{
    const auto parsed = ObjectId::fromString("0x00000000000000ff");
    QVERIFY(parsed.has_value());
    QCOMPARE(static_cast<qulonglong>(parsed->value()), 0xffULL);
}

void ObjectIdTest::rejectsMalformedText()
{
    QVERIFY(!ObjectId::fromString("").has_value());
    QVERIFY(!ObjectId::fromString("xyz").has_value());
    QVERIFY(!ObjectId::fromString("0").has_value());
    QVERIFY(!ObjectId::fromString("0000000000000000").has_value());
    QVERIFY(!ObjectId::fromString("00000000000000001").has_value());
}

void ObjectIdTest::comparesById()
{
    const ObjectId low{1};
    const ObjectId high{2};

    QVERIFY(low < high);
    QVERIFY(low == ObjectId{1});
    QVERIFY(low != high);
}

} // namespace
} // namespace ofgui::core

QTEST_APPLESS_MAIN(ofgui::core::ObjectIdTest)

#include "ObjectIdTest.moc"
