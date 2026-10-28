#include "core/BoundaryPatch.h"

#include <QtTest>

#include <set>
#include <string>

namespace ofgui::core {
namespace {

class BoundaryPatchTest final : public QObject
{
    Q_OBJECT

private slots:
    void createAssignsFreshValidId();
    void renamingKeepsTheId();
    void validatesPatchNames();
    void managesTheFaceSet();
    void mapsPatchTypeNames();
    void defaultsToVisibleGrey();
};

void BoundaryPatchTest::createAssignsFreshValidId()
{
    const BoundaryPatch inlet = BoundaryPatch::create("inlet");
    const BoundaryPatch outlet = BoundaryPatch::create("outlet");

    QVERIFY(inlet.id().isValid());
    QVERIFY(outlet.id().isValid());
    QVERIFY(inlet.id() != outlet.id());
    QCOMPARE(QString::fromStdString(inlet.name()), QStringLiteral("inlet"));
    QCOMPARE(inlet.type(), PatchType::Patch);
}

void BoundaryPatchTest::renamingKeepsTheId()
{
    BoundaryPatch patch = BoundaryPatch::create("inlet");
    const ObjectId original = patch.id();

    patch.setName("inlet_left");

    QCOMPARE(QString::fromStdString(patch.name()), QStringLiteral("inlet_left"));
    QVERIFY(patch.id() == original);
}

void BoundaryPatchTest::validatesPatchNames()
{
    QVERIFY(BoundaryPatch::isValidName("inlet"));
    QVERIFY(BoundaryPatch::isValidName("wall_1"));
    QVERIFY(BoundaryPatch::isValidName("A1"));

    QVERIFY(!BoundaryPatch::isValidName(""));
    QVERIFY(!BoundaryPatch::isValidName("1wall"));
    QVERIFY(!BoundaryPatch::isValidName("in let"));
    QVERIFY(!BoundaryPatch::isValidName("wall-1"));
    QVERIFY(!BoundaryPatch::isValidName("_wall"));
    QVERIFY(!BoundaryPatch::isValidName("墙面"));
}

void BoundaryPatchTest::managesTheFaceSet()
{
    BoundaryPatch patch = BoundaryPatch::create("walls");

    QVERIFY(patch.faces().empty());
    QCOMPARE(static_cast<int>(patch.faceCount()), 0);

    QVERIFY(patch.addFace(FaceId::XMin));
    QVERIFY(!patch.addFace(FaceId::XMin));
    QVERIFY(patch.hasFace(FaceId::XMin));
    QCOMPARE(static_cast<int>(patch.faceCount()), 1);

    QVERIFY(patch.removeFace(FaceId::XMin));
    QVERIFY(!patch.removeFace(FaceId::XMin));
    QVERIFY(!patch.hasFace(FaceId::XMin));

    patch.setFaces(std::set<FaceId>{FaceId::YMin, FaceId::YMax});
    QCOMPARE(static_cast<int>(patch.faceCount()), 2);
    QVERIFY(patch.hasFace(FaceId::YMin));
    QVERIFY(patch.hasFace(FaceId::YMax));
}

void BoundaryPatchTest::mapsPatchTypeNames()
{
    QCOMPARE(QString::fromStdString(std::string{patchTypeName(PatchType::Patch)}), QStringLiteral("patch"));
    QCOMPARE(QString::fromStdString(std::string{patchTypeName(PatchType::Wall)}), QStringLiteral("wall"));
    QCOMPARE(QString::fromStdString(std::string{patchTypeName(PatchType::Empty)}), QStringLiteral("empty"));
    QCOMPARE(QString::fromStdString(std::string{patchTypeName(PatchType::Symmetry)}), QStringLiteral("symmetry"));
    QCOMPARE(
        QString::fromStdString(std::string{patchTypeName(PatchType::SymmetryPlane)}),
        QStringLiteral("symmetryPlane"));

    for (const PatchType type : {PatchType::Patch, PatchType::Wall, PatchType::Empty,
             PatchType::Symmetry, PatchType::SymmetryPlane})
    {
        const auto parsed = patchTypeFromName(patchTypeName(type));
        QVERIFY(parsed.has_value());
        QCOMPARE(*parsed, type);
    }

    QVERIFY(!patchTypeFromName("notAType").has_value());
}

void BoundaryPatchTest::defaultsToVisibleGrey()
{
    const BoundaryPatch patch = BoundaryPatch::create("walls");
    QVERIFY(patch.isVisible());
    QCOMPARE(patch.color().at(0), 0.8);
    QCOMPARE(patch.color().at(1), 0.8);
    QCOMPARE(patch.color().at(2), 0.8);
}

} // namespace
} // namespace ofgui::core

QTEST_APPLESS_MAIN(ofgui::core::BoundaryPatchTest)

#include "BoundaryPatchTest.moc"
