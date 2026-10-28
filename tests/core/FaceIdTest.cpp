#include "core/FaceId.h"

#include <QtTest>

#include <set>

namespace ofgui::core {
namespace {

class FaceIdTest final : public QObject
{
    Q_OBJECT

private slots:
    void enumValuesAreStable();
    void allFaceIdsFollowEnumOrder();
    void faceIndexMatchesEnumValue();
    void axisAndSideAreConsistent();
    void faceNamesAreStable();
    void facesAreDistinct();
};

void FaceIdTest::enumValuesAreStable()
{
    // The numeric values are persisted in .ofgproj files and used by the VTK
    // pick mapping, so they must never change.
    QCOMPARE(static_cast<int>(FaceId::XMin), 0);
    QCOMPARE(static_cast<int>(FaceId::XMax), 1);
    QCOMPARE(static_cast<int>(FaceId::YMin), 2);
    QCOMPARE(static_cast<int>(FaceId::YMax), 3);
    QCOMPARE(static_cast<int>(FaceId::ZMin), 4);
    QCOMPARE(static_cast<int>(FaceId::ZMax), 5);
}

void FaceIdTest::allFaceIdsFollowEnumOrder()
{
    QCOMPARE(static_cast<int>(kAllFaceIds.size()), 6);
    for (std::size_t index = 0; index < kAllFaceIds.size(); ++index)
    {
        QCOMPARE(static_cast<int>(kAllFaceIds.at(index)), static_cast<int>(index));
    }
}

void FaceIdTest::faceIndexMatchesEnumValue()
{
    for (const FaceId face : kAllFaceIds)
    {
        QCOMPARE(static_cast<int>(faceIndex(face)), static_cast<int>(face));
    }
}

void FaceIdTest::axisAndSideAreConsistent()
{
    QCOMPARE(faceAxis(FaceId::XMin), 0);
    QCOMPARE(faceAxis(FaceId::XMax), 0);
    QCOMPARE(faceAxis(FaceId::YMin), 1);
    QCOMPARE(faceAxis(FaceId::YMax), 1);
    QCOMPARE(faceAxis(FaceId::ZMin), 2);
    QCOMPARE(faceAxis(FaceId::ZMax), 2);

    QVERIFY(!faceIsMax(FaceId::XMin));
    QVERIFY(faceIsMax(FaceId::XMax));
    QVERIFY(!faceIsMax(FaceId::YMin));
    QVERIFY(faceIsMax(FaceId::YMax));
    QVERIFY(!faceIsMax(FaceId::ZMin));
    QVERIFY(faceIsMax(FaceId::ZMax));
}

void FaceIdTest::faceNamesAreStable()
{
    QCOMPARE(QString::fromUtf8(faceName(FaceId::XMin)), QStringLiteral("xMin"));
    QCOMPARE(QString::fromUtf8(faceName(FaceId::XMax)), QStringLiteral("xMax"));
    QCOMPARE(QString::fromUtf8(faceName(FaceId::YMin)), QStringLiteral("yMin"));
    QCOMPARE(QString::fromUtf8(faceName(FaceId::YMax)), QStringLiteral("yMax"));
    QCOMPARE(QString::fromUtf8(faceName(FaceId::ZMin)), QStringLiteral("zMin"));
    QCOMPARE(QString::fromUtf8(faceName(FaceId::ZMax)), QStringLiteral("zMax"));
}

void FaceIdTest::facesAreDistinct()
{
    std::set<int> values;
    for (const FaceId face : kAllFaceIds)
    {
        values.insert(static_cast<int>(face));
    }

    QCOMPARE(static_cast<int>(values.size()), 6);
}

} // namespace
} // namespace ofgui::core

QTEST_APPLESS_MAIN(ofgui::core::FaceIdTest)

#include "FaceIdTest.moc"
