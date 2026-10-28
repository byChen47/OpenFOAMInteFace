#include "core/DomainBox.h"
#include "core/Vec3d.h"
#include "render/DomainBoundsAdapter.h"
#include "render/DomainSceneBuilder.h"
#include "render/PreviewDomain.h"

#include <QSet>
#include <QString>
#include <QtTest>

#include <vtkActor.h>
#include <vtkProperty.h>

#include <set>
#include <limits>

namespace ofgui {
namespace {

class DomainBoundsAdapterTest final : public QObject
{
    Q_OBJECT

private slots:
    void convertsDocumentBox();
    void rejectsInvalidBox();
    void comparesBoundsWithinTolerance();
    void buildsSceneFromDocumentBox();
    void buildsEmptySceneForInvalidBox();
};

void DomainBoundsAdapterTest::convertsDocumentBox()
{
    const core::DomainBox box(core::Vec3d{0.0, 0.0, 0.0}, core::Vec3d{2.0, 3.0, 4.0});
    const std::optional<PreviewDomain> preview = DomainBoundsAdapter::previewDomain(box);

    QVERIFY(preview.has_value());
    const std::array<double, 6> bounds = preview->bounds();
    QCOMPARE(bounds[0], 0.0);
    QCOMPARE(bounds[1], 2.0);
    QCOMPARE(bounds[2], 0.0);
    QCOMPARE(bounds[3], 3.0);
    QCOMPARE(bounds[4], 0.0);
    QCOMPARE(bounds[5], 4.0);
}

void DomainBoundsAdapterTest::rejectsInvalidBox()
{
    // Default box is min == max == 0: not drawable.
    QVERIFY(!DomainBoundsAdapter::previewDomain(core::DomainBox{}).has_value());

    // Inverted axis.
    const core::DomainBox inverted(core::Vec3d{1.0, 0.0, 0.0}, core::Vec3d{0.0, 1.0, 1.0});
    QVERIFY(!DomainBoundsAdapter::previewDomain(inverted).has_value());

    // Not finite.
    const core::DomainBox infinite(core::Vec3d{0.0, 0.0, 0.0},
                                   core::Vec3d{std::numeric_limits<core::Real>::infinity(), 1.0, 1.0});
    QVERIFY(!DomainBoundsAdapter::previewDomain(infinite).has_value());
}

void DomainBoundsAdapterTest::comparesBoundsWithinTolerance()
{
    const PreviewDomain reference = makeDefaultPreviewDomain();
    QVERIFY(DomainBoundsAdapter::sameBounds(reference, reference));

    PreviewDomain almost = reference;
    almost.maxX += 1.0e-13;
    QVERIFY(DomainBoundsAdapter::sameBounds(reference, almost));

    PreviewDomain moved = reference;
    moved.maxX += 1.0e-9;
    QVERIFY(!DomainBoundsAdapter::sameBounds(reference, moved));
}

void DomainBoundsAdapterTest::buildsSceneFromDocumentBox()
{
    const core::DomainBox box(core::Vec3d{0.0, 0.0, 0.0}, core::Vec3d{2.0, 1.5, 1.0});
    const DomainSceneActors scene = DomainSceneBuilder{}.build(box);

    QCOMPARE(scene.faces.size(), kAllFaceIds.size());
    QVERIFY(scene.outline != nullptr);

    std::set<int> ids;
    std::set<vtkActor*> actors;
    QSet<QString> colors;
    for (const auto& face : scene.faces)
    {
        QVERIFY(face.actor != nullptr);
        ids.insert(static_cast<int>(face.faceId));
        actors.insert(face.actor.GetPointer());

        double color[3]{};
        face.actor->GetProperty()->GetColor(color);
        colors.insert(QStringLiteral("%1/%2/%3")
                          .arg(color[0], 0, 'f', 6)
                          .arg(color[1], 0, 'f', 6)
                          .arg(color[2], 0, 'f', 6));
    }
    QCOMPARE(ids.size(), kAllFaceIds.size());
    QCOMPARE(actors.size(), kAllFaceIds.size());
    QCOMPARE(colors.size(), kAllFaceIds.size());
}

void DomainBoundsAdapterTest::buildsEmptySceneForInvalidBox()
{
    const DomainSceneActors scene = DomainSceneBuilder{}.build(core::DomainBox{});
    QVERIFY(scene.faces.empty());
    QVERIFY(scene.outline == nullptr);
}

} // namespace
} // namespace ofgui

QTEST_APPLESS_MAIN(ofgui::DomainBoundsAdapterTest)

#include "DomainBoundsAdapterTest.moc"
