#include "ui/CaseDocumentController.h"
#include "ui/ConfigurationPanel.h"
#include "ui/FieldBoundaryTable.h"
#include "ui/FieldEditor.h"
#include "ui/InspectorPanel.h"
#include "ui/MainWindow.h"
#include "ui/ProjectTreeModel.h"
#include "ui/ProjectTreeView.h"

#include "core/BoundaryPatch.h"
#include "core/CaseProject.h"
#include "core/FieldDefinition.h"

#include "io/CaseImporter.h"

#include <QComboBox>
#include <QDir>
#include <QFileInfo>
#include <QLineEdit>
#include <QModelIndex>
#include <QStringList>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QtTest>

#include <filesystem>
#include <functional>

namespace ofgui {
namespace {

QString referenceDirectory()
{
    return QString::fromLatin1(OFGUI_FOAM_REFERENCE_DIR);
}

/// Tutorial case used by the acceptance criteria of t8.
QString cavityTutorialPath()
{
    return QDir(referenceDirectory())
        .filePath(QStringLiteral("tutorials/incompressible/icoFoam/cavity/cavity"));
}

bool tutorialImportsCleanly(const QString& path)
{
    if (!QFileInfo::exists(path))
    {
        return false;
    }
    const std::filesystem::path root(path.toStdU16String());
    return io::CaseImporter().import(root).ok;
}

QString labelOf(const QAbstractItemModel& model, const QModelIndex& index)
{
    return index.isValid() ? model.data(index, Qt::DisplayRole).toString() : QString();
}

QModelIndex findKind(ProjectTreeModel* model, NodeKind kind)
{
    std::function<QModelIndex(const QModelIndex&)> search =
        [&](const QModelIndex& parent) -> QModelIndex
    {
        for (int row = 0; row < model->rowCount(parent); ++row)
        {
            const QModelIndex child = model->index(row, 0, parent);
            if (model->nodeRef(child).kind == kind)
            {
                return child;
            }
            const QModelIndex found = search(child);
            if (found.isValid())
            {
                return found;
            }
        }
        return {};
    };
    return search(QModelIndex());
}

} // namespace

class CaseDocumentTreeTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void newProjectShowsSectionSkeleton();
    void openingTutorialCaseBuildsTree();
    void selectingPatchBoundaryConditionShowsType();
    void selectingFaceResolvesOwningPatch();
    void everyTreeNodeCarriesStableIdentifiers();
    void savingAndReloadingKeepsTreeStructure();

private:
    QString m_tutorial;
};

void CaseDocumentTreeTest::initTestCase()
{
    m_tutorial = cavityTutorialPath();
}

void CaseDocumentTreeTest::newProjectShowsSectionSkeleton()
{
    MainWindow window;
    auto* tree = window.configurationPanel()->treeView();
    ProjectTreeModel* model = tree->projectModel();

    QCOMPARE(window.document()->project().name(), std::string("未命名工程"));
    QVERIFY(!window.document()->isDirty());
    QVERIFY(window.windowTitle().contains(QStringLiteral("未命名工程")));

    QCOMPARE(model->rowCount(QModelIndex()), 1);
    const QModelIndex projectNode = model->index(0, 0);
    QVERIFY(model->nodeRef(projectNode).kind == NodeKind::Project);
    QVERIFY(labelOf(*model, projectNode).contains(QStringLiteral("工程")));

    QCOMPARE(model->rowCount(projectNode), 6);
    const QStringList expected = {
        QStringLiteral("计算域"),
        QStringLiteral("网格"),
        QStringLiteral("边界（六面）"),
        QStringLiteral("场（0/）"),
        QStringLiteral("求解器设置"),
        QStringLiteral("导出预览"),
    };
    for (int row = 0; row < expected.size(); ++row)
    {
        QCOMPARE(labelOf(*model, model->index(row, 0, projectNode)), expected.at(row));
    }

    const QModelIndex boundaries = findKind(model, NodeKind::Boundaries);
    QVERIFY(boundaries.isValid());
    QCOMPARE(model->rowCount(boundaries), 6);
    // The default cavity template assigns every one of the six faces.
    QVERIFY(labelOf(*model, model->index(0, 0, boundaries))
                .contains(QStringLiteral("fixedWalls")));
}

void CaseDocumentTreeTest::openingTutorialCaseBuildsTree()
{
    if (!tutorialImportsCleanly(m_tutorial))
    {
        QSKIP("OpenFOAM-v2206 cavity tutorial is not available or not importable");
    }

    MainWindow window;
    auto* tree = window.configurationPanel()->treeView();
    ProjectTreeModel* model = tree->projectModel();

    QVERIFY(window.openCaseDirectory(m_tutorial));
    QVERIFY(window.document()->isDirty());

    const core::CaseProject& project = window.document()->project();
    QCOMPARE(static_cast<int>(project.patches().size()), 3);
    QCOMPARE(static_cast<int>(project.fields().size()), 2);

    const QModelIndex fields = findKind(model, NodeKind::Fields);
    QVERIFY(fields.isValid());
    QCOMPARE(model->rowCount(fields), 2);

    QStringList fieldLabels;
    for (int row = 0; row < model->rowCount(fields); ++row)
    {
        fieldLabels.append(labelOf(*model, model->index(row, 0, fields)));
        QCOMPARE(model->rowCount(model->index(row, 0, fields)), 3);
    }
    QVERIFY(fieldLabels.contains(QStringLiteral("p [volScalarField]")));
    QVERIFY(fieldLabels.contains(QStringLiteral("U [volVectorField]")));

    const QModelIndex boundaries = findKind(model, NodeKind::Boundaries);
    QVERIFY(boundaries.isValid());
    QCOMPARE(model->rowCount(boundaries), 6);

    const core::FieldDefinition* p = project.findFieldByName("p");
    const core::FieldDefinition* u = project.findFieldByName("U");
    QVERIFY(p != nullptr);
    QVERIFY(u != nullptr);
    QVERIFY(project.findPatchByName("movingWall") != nullptr);
    QVERIFY(project.findPatchByName("fixedWalls") != nullptr);

    const core::BoundaryPatch* movingWall = project.findPatchByName("movingWall");
    const QModelIndex pMovingWall =
        model->indexFor(ProjectNodeRef::forFieldBoundary(p->id(), movingWall->id()));
    QVERIFY(pMovingWall.isValid());
    QCOMPARE(labelOf(*model, pMovingWall), QStringLiteral("movingWall — zeroGradient"));

    const core::BoundaryPatch* frontAndBack = project.findPatchByName("frontAndBack");
    const QModelIndex pFront = model->indexFor(
        ProjectNodeRef::forFieldBoundary(p->id(), frontAndBack->id()));
    QCOMPARE(labelOf(*model, pFront), QStringLiteral("frontAndBack — empty"));

    const QModelIndex uMovingWall =
        model->indexFor(ProjectNodeRef::forFieldBoundary(u->id(), movingWall->id()));
    QCOMPARE(labelOf(*model, uMovingWall), QStringLiteral("movingWall — fixedValue"));
}

void CaseDocumentTreeTest::selectingPatchBoundaryConditionShowsType()
{
    if (!tutorialImportsCleanly(m_tutorial))
    {
        QSKIP("OpenFOAM-v2206 cavity tutorial is not available or not importable");
    }

    MainWindow window;
    QVERIFY(window.openCaseDirectory(m_tutorial));

    const core::CaseProject& project = window.document()->project();
    const core::FieldDefinition* u = project.findFieldByName("U");
    const core::BoundaryPatch* movingWall = project.findPatchByName("movingWall");
    QVERIFY(u != nullptr);
    QVERIFY(movingWall != nullptr);

    auto* tree = window.configurationPanel()->treeView();
    auto* inspector = window.configurationPanel()->inspector();
    const ProjectNodeRef ref = ProjectNodeRef::forFieldBoundary(u->id(), movingWall->id());

    tree->selectNode(ref);
    QCoreApplication::processEvents();

    QVERIFY(inspector->currentNode() == ref);
    FieldBoundaryTable* table = inspector->fieldEditor()->boundaryTable();
    bool foundFixedValue = false;
    for (int row = 0; row < table->table()->rowCount(); ++row)
    {
        QComboBox* combo = table->typeCombo(row);
        if (combo != nullptr && combo->currentData().toString() == QStringLiteral("fixedValue"))
        {
            foundFixedValue = true;
            QVERIFY(table->valueEdit(row)->text().contains(QStringLiteral("uniform")));
        }
    }
    QVERIFY(foundFixedValue);
}

void CaseDocumentTreeTest::selectingFaceResolvesOwningPatch()
{
    if (!tutorialImportsCleanly(m_tutorial))
    {
        QSKIP("OpenFOAM-v2206 cavity tutorial is not available or not importable");
    }

    MainWindow window;
    QVERIFY(window.openCaseDirectory(m_tutorial));

    auto* tree = window.configurationPanel()->treeView();
    auto* inspector = window.configurationPanel()->inspector();

    // cavity: yMax is the moving wall, xMin is part of fixedWalls.
    tree->selectNode(ProjectNodeRef::forFace(core::FaceId::YMax));
    QCoreApplication::processEvents();
    QVERIFY(inspector->bodyText().contains(QStringLiteral("movingWall")));

    tree->selectNode(ProjectNodeRef::forFace(core::FaceId::XMin));
    QCoreApplication::processEvents();
    QVERIFY(inspector->bodyText().contains(QStringLiteral("fixedWalls")));
}

void CaseDocumentTreeTest::everyTreeNodeCarriesStableIdentifiers()
{
    if (!tutorialImportsCleanly(m_tutorial))
    {
        QSKIP("OpenFOAM-v2206 cavity tutorial is not available or not importable");
    }

    MainWindow window;
    QVERIFY(window.openCaseDirectory(m_tutorial));

    auto* tree = window.configurationPanel()->treeView();
    ProjectTreeModel* model = tree->projectModel();
    const core::CaseProject& project = window.document()->project();

    int faces = 0;
    int fields = 0;
    int boundaries = 0;

    std::function<void(const QModelIndex&)> visit = [&](const QModelIndex& parent)
    {
        for (int row = 0; row < model->rowCount(parent); ++row)
        {
            const QModelIndex index = model->index(row, 0, parent);
            const ProjectNodeRef ref = model->nodeRef(index);
            const QVariant stored = model->data(index, ProjectTreeModel::NodeRefRole);
            QVERIFY(stored.canConvert<ProjectNodeRef>());
            QVERIFY(stored.value<ProjectNodeRef>() == ref);
            QVERIFY(model->indexFor(ref).isValid());
            QVERIFY(model->nodeRef(model->indexFor(ref)) == ref);

            switch (ref.kind)
            {
                case NodeKind::BoundaryFace:
                    ++faces;
                    break;
                case NodeKind::Field:
                    ++fields;
                    QVERIFY(project.findField(ref.fieldId) != nullptr);
                    break;
                case NodeKind::FieldBoundary:
                    ++boundaries;
                    QVERIFY(project.findField(ref.fieldId) != nullptr);
                    QVERIFY(project.findPatch(ref.patchId) != nullptr);
                    break;
                default:
                    break;
            }
            visit(index);
        }
    };
    visit(QModelIndex());

    QCOMPARE(faces, 6);
    QCOMPARE(fields, 2);
    QCOMPARE(boundaries, 6);
}

void CaseDocumentTreeTest::savingAndReloadingKeepsTreeStructure()
{
    if (!tutorialImportsCleanly(m_tutorial))
    {
        QSKIP("OpenFOAM-v2206 cavity tutorial is not available or not importable");
    }

    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    MainWindow window;
    QVERIFY(window.openCaseDirectory(m_tutorial));
    const int fieldCount = static_cast<int>(window.document()->project().fields().size());

    const QString requested = directory.filePath(QStringLiteral("cavity-case"));
    QVERIFY(window.document()->saveProjectAs(requested));
    const QString savedPath = requested + QStringLiteral(".ofgproj");
    QVERIFY(QFileInfo::exists(savedPath));
    QVERIFY(!window.document()->isDirty());
    QVERIFY(!window.isWindowModified());

    window.document()->newProject();
    ProjectTreeModel* model = window.configurationPanel()->treeView()->projectModel();
    // A new project is the bundled cavity template (t12), so it has the same two
    // fields as the tutorial rather than an empty fields section.
    QCOMPARE(static_cast<int>(window.document()->project().fields().size()), 2);
    QCOMPARE(model->rowCount(findKind(model, NodeKind::Fields)), 2);

    QVERIFY(window.openProjectFile(savedPath));
    QCOMPARE(static_cast<int>(window.document()->project().fields().size()), fieldCount);
    QCOMPARE(model->rowCount(findKind(model, NodeKind::Fields)), 2);
    QCOMPARE(model->rowCount(findKind(model, NodeKind::Boundaries)), 6);
    QVERIFY(window.windowTitle().contains(QStringLiteral("cavity")));
}

} // namespace ofgui

QTEST_MAIN(ofgui::CaseDocumentTreeTest)

#include "CaseDocumentTreeTest.moc"
