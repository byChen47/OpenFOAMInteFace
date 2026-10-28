#include "core/BoundaryPatch.h"
#include "core/FaceAssignmentValidator.h"

#include <QtTest>

#include <set>
#include <vector>

namespace ofgui::core {
namespace {

class FaceAssignmentValidatorTest final : public QObject
{
    Q_OBJECT

private slots:
    void coverageFollowsFaceEnumOrder();
    void detectsUnassignedFaces();
    void detectsMultiplyAssignedFaces();
    void requiresExactlyOneOwnerPerFace();
};

void FaceAssignmentValidatorTest::coverageFollowsFaceEnumOrder()
{
    BoundaryPatch walls("walls", PatchType::Wall);
    walls.setFaces({FaceId::XMin, FaceId::YMin});
    BoundaryPatch outlet = BoundaryPatch::create("outlet");
    outlet.setFaces({FaceId::XMax});
    const std::vector<BoundaryPatch> patches{walls, outlet};

    const auto coverage = FaceAssignmentValidator::coverage(patches);
    for (std::size_t index = 0; index < kAllFaceIds.size(); ++index)
    {
        QCOMPARE(coverage.at(index).face, kAllFaceIds.at(index));
    }

    QVERIFY(coverage.at(faceIndex(FaceId::XMin)).isAssigned());
    QVERIFY(coverage.at(faceIndex(FaceId::XMax)).isAssigned());
    QVERIFY(!coverage.at(faceIndex(FaceId::ZMax)).isAssigned());
}

void FaceAssignmentValidatorTest::detectsUnassignedFaces()
{
    BoundaryPatch patch = BoundaryPatch::create("walls");
    patch.setFaces({FaceId::XMin});

    const std::vector<FaceId> unassigned =
        FaceAssignmentValidator::unassignedFaces(std::vector<BoundaryPatch>{patch});
    QCOMPARE(static_cast<int>(unassigned.size()), 5);
    QVERIFY(unassigned.at(0) == FaceId::XMax);
    QVERIFY(unassigned.at(4) == FaceId::ZMax);
    QVERIFY(!FaceAssignmentValidator::isFullyAssigned(std::vector<BoundaryPatch>{patch}));
}

void FaceAssignmentValidatorTest::detectsMultiplyAssignedFaces()
{
    BoundaryPatch first = BoundaryPatch::create("first");
    first.setFaces({FaceId::XMin, FaceId::YMin});
    BoundaryPatch second = BoundaryPatch::create("second");
    second.setFaces({FaceId::YMin, FaceId::ZMin});

    const std::vector<BoundaryPatch> patches{first, second};
    const std::vector<FaceId> ambiguous = FaceAssignmentValidator::multiplyAssignedFaces(patches);

    QCOMPARE(static_cast<int>(ambiguous.size()), 1);
    QVERIFY(ambiguous.front() == FaceId::YMin);
    QCOMPARE(static_cast<int>(FaceAssignmentValidator::coverage(patches).at(faceIndex(FaceId::YMin)).patchIds.size()), 2);
    QVERIFY(!FaceAssignmentValidator::isFullyAssigned(patches));
}

void FaceAssignmentValidatorTest::requiresExactlyOneOwnerPerFace()
{
    BoundaryPatch patch = BoundaryPatch::create("all");
    std::set<FaceId> faces;
    for (const FaceId face : kAllFaceIds)
    {
        faces.insert(face);
    }
    patch.setFaces(faces);

    const std::vector<BoundaryPatch> patches{patch};
    QVERIFY(FaceAssignmentValidator::isFullyAssigned(patches));
    QVERIFY(FaceAssignmentValidator::unassignedFaces(patches).empty());
    QVERIFY(FaceAssignmentValidator::multiplyAssignedFaces(patches).empty());
}

} // namespace
} // namespace ofgui::core

QTEST_APPLESS_MAIN(ofgui::core::FaceAssignmentValidatorTest)

#include "FaceAssignmentValidatorTest.moc"
