#include "core/CaseProject.h"
#include "render/VtkViewportWidget.h"
#include "ui/CaseDocumentController.h"
#include "ui/ConfigurationPanel.h"
#include "ui/DomainEditor.h"
#include "ui/InspectorPanel.h"
#include "ui/MainWindow.h"
#include "ui/ProjectNodeRef.h"
#include "ui/ProjectTreeView.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileInfo>
#include <QSpinBox>
#include <QtTest>

#include <array>

namespace ofgui {
namespace {

QString cavityTutorialDirectory()
{
    return QStringLiteral(OFGUI_FOAM_REFERENCE_DIR)
           + QStringLiteral("/tutorials/incompressible/icoFoam/cavity/cavity");
}

QDoubleSpinBox* coordinateSpin(DomainEditor& editor, const QString& name)
{
    return editor.findChild<QDoubleSpinBox*>(name);
}

QSpinBox* cellsSpin(DomainEditor& editor, const QString& name)
{
    return editor.findChild<QSpinBox*>(name);
}

/// Selects the domain node, which must switch the inspector to the editor page.
DomainEditor* selectDomainEditor(MainWindow& window)
{
    window.configurationPanel()->treeView()->selectNode(ProjectNodeRef::section(NodeKind::Domain));
    InspectorPanel* inspector = window.configurationPanel()->inspector();
    if (!inspector->isEditingDomain())
    {
        return nullptr;
    }
    return inspector->domainEditor();
}

class DomainEditorTest final : public QObject
{
    Q_OBJECT

private slots:
    void selectingDomainShowsEditor();
    void editingMaximumUpdatesModelAndScene();
    void editingMinimumUpdatesModelAndScene();
    void invalidMaximumIsRejected();
    void cellsChangeKeepsGeometry();
    void twoDModeForcesSingleLayer();
    void importedCaseFillsDomainEditor();
};

void DomainEditorTest::selectingDomainShowsEditor()
{
    MainWindow window;
    DomainEditor* editor = selectDomainEditor(window);
    QVERIFY(editor != nullptr);
    QCOMPARE(editor->errorText(), QString());
    // New projects start as the 2D cavity template (t12), not a unit cube.
    QCOMPARE(window.document()->project().domain().max().x, 0.1);
}

void DomainEditorTest::editingMaximumUpdatesModelAndScene()
{
    MainWindow window;
    VtkViewportWidget* viewport = window.viewport();
    const int revisionBefore = viewport->sceneRevision();

    DomainEditor* editor = selectDomainEditor(window);
    QVERIFY(editor != nullptr);
    QDoubleSpinBox* maxX = coordinateSpin(*editor, QStringLiteral("domainMaxX"));
    QVERIFY(maxX != nullptr);

    maxX->setValue(2.5);

    QCOMPARE(window.document()->project().domain().max().x, 2.5);
    QCOMPARE(viewport->sceneRevision(), revisionBefore + 1);
    QCOMPARE(viewport->sceneBounds()[1], 2.5);
    QVERIFY(window.document()->isDirty());
}

void DomainEditorTest::editingMinimumUpdatesModelAndScene()
{
    MainWindow window;
    VtkViewportWidget* viewport = window.viewport();
    const int revisionBefore = viewport->sceneRevision();

    DomainEditor* editor = selectDomainEditor(window);
    QVERIFY(editor != nullptr);
    QDoubleSpinBox* minY = coordinateSpin(*editor, QStringLiteral("domainMinY"));
    QVERIFY(minY != nullptr);

    minY->setValue(-0.5);

    QCOMPARE(window.document()->project().domain().min().y, -0.5);
    QCOMPARE(viewport->sceneRevision(), revisionBefore + 1);
    QCOMPARE(viewport->sceneBounds()[2], -0.5);
}

void DomainEditorTest::invalidMaximumIsRejected()
{
    MainWindow window;
    VtkViewportWidget* viewport = window.viewport();
    const int revisionBefore = viewport->sceneRevision();
    const std::array<double, 6> boundsBefore = viewport->sceneBounds();

    DomainEditor* editor = selectDomainEditor(window);
    QVERIFY(editor != nullptr);
    QDoubleSpinBox* maxX = coordinateSpin(*editor, QStringLiteral("domainMaxX"));
    QVERIFY(maxX != nullptr);

    // max <= min is rejected before the model and the scene are touched.
    maxX->setValue(-1.0);

    QCOMPARE(window.document()->project().domain().max().x, 0.1);
    QCOMPARE(viewport->sceneRevision(), revisionBefore);
    QVERIFY(viewport->sceneBounds() == boundsBefore);
    QVERIFY(!editor->errorText().isEmpty());
    QVERIFY(editor->errorText().contains(QStringLiteral("x 轴")));

    // Invalid cell counts are rejected the same way.
    QSpinBox* cellsX = cellsSpin(*editor, QStringLiteral("domainCellsX"));
    QVERIFY(cellsX != nullptr);
    cellsX->setValue(0);
    QCOMPARE(window.document()->project().mesh().cellCount(0), 20);
    QVERIFY(editor->errorText().contains(QStringLiteral("网格数")));
    QCOMPARE(viewport->sceneRevision(), revisionBefore);
}

void DomainEditorTest::cellsChangeKeepsGeometry()
{
    MainWindow window;
    VtkViewportWidget* viewport = window.viewport();
    const int revisionBefore = viewport->sceneRevision();
    const std::array<double, 6> boundsBefore = viewport->sceneBounds();

    DomainEditor* editor = selectDomainEditor(window);
    QVERIFY(editor != nullptr);
    QSpinBox* cellsX = cellsSpin(*editor, QStringLiteral("domainCellsX"));
    QVERIFY(cellsX != nullptr);

    cellsX->setValue(30);

    QCOMPARE(window.document()->project().mesh().cellCount(0), 30);
    // Cell counts are not geometry: no rebuild, identical bounds.
    QCOMPARE(viewport->sceneRevision(), revisionBefore);
    QVERIFY(viewport->sceneBounds() == boundsBefore);
}

void DomainEditorTest::twoDModeForcesSingleLayer()
{
    MainWindow window;
    DomainEditor* editor = selectDomainEditor(window);
    QVERIFY(editor != nullptr);

    auto* mode = editor->findChild<QComboBox*>(QStringLiteral("domainModeCombo"));
    QSpinBox* cellsZ = cellsSpin(*editor, QStringLiteral("domainCellsZ"));
    QVERIFY(mode != nullptr);
    QVERIFY(cellsZ != nullptr);
    // The cavity template is 2D: the z layer is fixed at 1 and disabled.
    QCOMPARE(cellsZ->value(), 1);
    QVERIFY(!cellsZ->isEnabled());

    mode->setCurrentIndex(mode->findData(static_cast<int>(core::DomainMode::ThreeD)));

    QCOMPARE(static_cast<int>(window.document()->project().domain().mode()),
             static_cast<int>(core::DomainMode::ThreeD));
    QVERIFY(cellsZ->isEnabled());
    QVERIFY(editor->errorText().isEmpty());
    QVERIFY(editor->hintText().contains(QStringLiteral("三维")));

    // Back to 2D: the layer is forced to 1 again.
    mode->setCurrentIndex(mode->findData(static_cast<int>(core::DomainMode::TwoD)));
    QCOMPARE(static_cast<int>(window.document()->project().domain().mode()),
             static_cast<int>(core::DomainMode::TwoD));
    QCOMPARE(window.document()->project().mesh().cellCount(2), 1);
    QVERIFY(!cellsZ->isEnabled());
    QCOMPARE(cellsZ->value(), 1);
    QVERIFY(editor->errorText().isEmpty());
    QVERIFY(editor->hintText().contains(QStringLiteral("二维")));
}

void DomainEditorTest::importedCaseFillsDomainEditor()
{
    MainWindow window;
    const QString cavity = cavityTutorialDirectory();
    QVERIFY2(QFileInfo::exists(cavity), qPrintable(cavity));
    QVERIFY(window.openCaseDirectory(cavity));

    DomainEditor* editor = selectDomainEditor(window);
    QVERIFY(editor != nullptr);

    QDoubleSpinBox* minX = coordinateSpin(*editor, QStringLiteral("domainMinX"));
    QDoubleSpinBox* maxX = coordinateSpin(*editor, QStringLiteral("domainMaxX"));
    QDoubleSpinBox* maxZ = coordinateSpin(*editor, QStringLiteral("domainMaxZ"));
    QSpinBox* cellsX = cellsSpin(*editor, QStringLiteral("domainCellsX"));
    QSpinBox* cellsZ = cellsSpin(*editor, QStringLiteral("domainCellsZ"));
    QVERIFY(minX != nullptr && maxX != nullptr && maxZ != nullptr);
    QVERIFY(cellsX != nullptr && cellsZ != nullptr);

    QCOMPARE(minX->value(), 0.0);
    QCOMPARE(maxX->value(), 0.1);
    QVERIFY(qAbs(maxZ->value() - 0.01) < 1e-9);
    QCOMPARE(cellsX->value(), 20);
    QCOMPARE(cellsZ->value(), 1);
    QVERIFY(editor->errorText().isEmpty());

    const std::array<double, 6> bounds = window.viewport()->sceneBounds();
    QCOMPARE(bounds[1], 0.1);
    QVERIFY(qAbs(bounds[5] - 0.01) < 1e-9);
}

} // namespace
} // namespace ofgui

QTEST_MAIN(ofgui::DomainEditorTest)

#include "DomainEditorTest.moc"
