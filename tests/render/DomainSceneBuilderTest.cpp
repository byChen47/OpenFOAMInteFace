#include "render/DomainSceneBuilder.h"

#include <QSet>
#include <QString>
#include <QtTest>

#include <vtkProperty.h>

#include <set>

namespace ofgui {
namespace {

class DomainSceneBuilderTest final : public QObject
{
    Q_OBJECT

private slots:
    void createsOneActorForEachFace();
    void assignsUniqueFaceColors();
    void createsOutline();
    void buildsDefaultPositiveDomain();
};

void DomainSceneBuilderTest::createsOneActorForEachFace()
{
    const auto scene = DomainSceneBuilder{}.build(makeDefaultPreviewDomain());

    QCOMPARE(scene.faces.size(), kAllFaceIds.size());

    std::set<int> faceIds;
    std::set<vtkActor*> actorAddresses;

    for (const auto& face : scene.faces)
    {
        faceIds.insert(static_cast<int>(face.faceId));
        QVERIFY(face.actor != nullptr);
        QVERIFY(face.actor->GetMapper() != nullptr);
        actorAddresses.insert(face.actor.GetPointer());
    }

    QCOMPARE(faceIds.size(), kAllFaceIds.size());
    QCOMPARE(actorAddresses.size(), kAllFaceIds.size());
}

void DomainSceneBuilderTest::assignsUniqueFaceColors()
{
    const auto scene = DomainSceneBuilder{}.build(makeDefaultPreviewDomain());
    QSet<QString> colors;

    for (const auto& face : scene.faces)
    {
        double color[3]{};
        face.actor->GetProperty()->GetColor(color);
        colors.insert(QStringLiteral("%1/%2/%3")
                          .arg(color[0], 0, 'f', 6)
                          .arg(color[1], 0, 'f', 6)
                          .arg(color[2], 0, 'f', 6));
    }

    QCOMPARE(colors.size(), kAllFaceIds.size());
}

void DomainSceneBuilderTest::createsOutline()
{
    const auto scene = DomainSceneBuilder{}.build(makeDefaultPreviewDomain());
    QVERIFY(scene.outline != nullptr);
    QVERIFY(scene.outline->GetMapper() != nullptr);
}

void DomainSceneBuilderTest::buildsDefaultPositiveDomain()
{
    const auto domain = makeDefaultPreviewDomain();
    QVERIFY(domain.maxX > domain.minX);
    QVERIFY(domain.maxY > domain.minY);
    QVERIFY(domain.maxZ > domain.minZ);
}

} // namespace
} // namespace ofgui

QTEST_APPLESS_MAIN(ofgui::DomainSceneBuilderTest)

#include "DomainSceneBuilderTest.moc"