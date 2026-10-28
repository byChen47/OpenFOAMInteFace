#include "ui/CaseDocumentController.h"
#include "ui/DefaultCaseTemplate.h"
#include "ui/ExportController.h"
#include "ui/ExportDictionaryCheck.h"
#include "ui/ExportPreviewDialog.h"
#include "ui/MainWindow.h"
#include "ui/ProjectNodeRef.h"
#include "ui/ValidationPanel.h"

#include "core/DomainBox.h"
#include "core/FieldDefinition.h"
#include "core/MeshSettings.h"
#include "core/SolverSettings.h"
#include "core/ValidationIssue.h"
#include "core/ValidationResult.h"
#include "io/writers/ExportAction.h"
#include "io/writers/ExportManifestSchema.h"

#include <QAction>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QPushButton>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <filesystem>
#include <string>
#include <vector>

namespace ofgui {
namespace {

QString readTextFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

QString tempEntry(const QTemporaryDir& directory, const QString& name)
{
    return QDir(directory.path()).filePath(name);
}

bool hasTempResidue(const QString& parentPath, const QString& baseName)
{
    const QDir parent(parentPath);
    const QFileInfoList entries =
        parent.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);
    for (const QFileInfo& entry : entries)
    {
        if (entry.fileName().startsWith(baseName + QStringLiteral(".tmp-")))
        {
            return true;
        }
    }
    return false;
}

/// Row index of the first validation issue with `code`, or -1.
int rowWithCode(const ValidationPanel& panel, const std::string& code)
{
    QTreeWidget* tree = panel.tree();
    for (int row = 0; row < tree->topLevelItemCount(); ++row)
    {
        QTreeWidgetItem* item = tree->topLevelItem(row);
        if (item != nullptr && item->text(1) == QString::fromStdString(code))
        {
            return row;
        }
    }
    return -1;
}

} // namespace

class ExportWorkflowTest final : public QObject
{
    Q_OBJECT

private slots:
    void defaultCavityTemplateMatchesTutorial();
    void defaultProjectIsExportable();
    void exportedTransportUsesEditedValue();
    void exportFailureLeavesNoResidue();
    void existingTargetRequiresConfirmation();
    void overwriteCreatesBackup();
    void dictionaryCheckRejectsBrokenText();
    void validationPanelNavigatesToIssueNode();
    void exportActionIsDisabledByBlockingIssue();
    void previewDialogReflectsBlockingState();
};

void ExportWorkflowTest::defaultCavityTemplateMatchesTutorial()
{
    const DefaultCaseTemplate bundle = makeDefaultCavityTemplate("cavity");
    const core::CaseProject& project = bundle.project;

    QCOMPARE(QString::fromStdString(project.name()), QStringLiteral("cavity"));
    QCOMPARE(project.patches().size(), std::size_t{3});
    QCOMPARE(project.fields().size(), std::size_t{2});
    QVERIFY(project.domain().mode() == core::DomainMode::TwoD);
    const core::MeshSettings::CellCounts cells = project.mesh().cells();
    QVERIFY(cells == (core::MeshSettings::CellCounts{20, 20, 1}));
    QCOMPARE(QString::fromStdString(project.solver().application()), QStringLiteral("icoFoam"));
    QCOMPARE(project.solver().endTime(), 0.5);
    QCOMPARE(bundle.transport.kinematicViscosity, 0.01);

    const core::FieldDefinition* velocity = project.findFieldByName("U");
    QVERIFY(velocity != nullptr);
    const core::BoundaryPatch* movingWall = project.findPatchByName("movingWall");
    QVERIFY(movingWall != nullptr);
    const core::BoundaryCondition* wallCondition =
        velocity->findBoundaryCondition(movingWall->id());
    QVERIFY(wallCondition != nullptr);
    QCOMPARE(QString::fromStdString(wallCondition->type()), QStringLiteral("fixedValue"));
    QVERIFY(wallCondition->parameter("value").has_value());
}

void ExportWorkflowTest::defaultProjectIsExportable()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    CaseDocumentController document;
    ExportController exporter(&document);

    QVERIFY(!document.validation().hasBlockingErrors());
    QVERIFY(!document.validation().hasErrors());

    const QString target = tempEntry(temp, QStringLiteral("cavity-export"));
    const ExportPreview preview = exporter.preview(target);
    QVERIFY2(preview.ok, qPrintable(preview.errorText));
    QVERIFY(preview.plan.find("system/blockMeshDict") != nullptr);
    QVERIFY(preview.plan.find("system/controlDict") != nullptr);
    QVERIFY(preview.plan.find("system/fvSchemes") != nullptr);
    QVERIFY(preview.plan.find("system/fvSolution") != nullptr);
    QVERIFY(preview.plan.find("0/p") != nullptr);
    QVERIFY(preview.plan.find("0/U") != nullptr);
    QVERIFY(preview.plan.find("constant/transportProperties") != nullptr);
    for (const io::ExportFile& file : preview.plan.files)
    {
        if (file.isCopy())
        {
            continue;
        }
        QVERIFY2(!file.text.empty(), file.relativePath.c_str());
    }

    const ExportOutcome outcome = exporter.exportTo(target, false);
    QVERIFY2(outcome.ok, qPrintable(outcome.message));
    QVERIFY(outcome.result.ok);
    QVERIFY(QFileInfo::exists(target + QStringLiteral("/0/p")));
    QVERIFY(!readTextFile(target + QStringLiteral("/system/controlDict")).isEmpty());
    QVERIFY(outcome.manifestVerified);
    QVERIFY(outcome.manifestFiles > 0);
    QCOMPARE(outcome.manifestFiles, static_cast<int>(outcome.result.fileCount));
    QVERIFY(outcome.result.writtenCount > 0);
}

void ExportWorkflowTest::exportedTransportUsesEditedValue()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    CaseDocumentController document;
    document.transportSettings().transportModel = "Newtonian";
    document.transportSettings().kinematicViscosity = 0.02;
    document.transportSettings().kinematicViscosityText.clear();
    document.notifyTransportEdited();

    ExportController exporter(&document);
    const QString target = tempEntry(temp, QStringLiteral("transport-export"));
    const ExportOutcome outcome = exporter.exportTo(target, false);
    QVERIFY2(outcome.ok, qPrintable(outcome.message));

    const QString text = readTextFile(target + QStringLiteral("/constant/transportProperties"));
    QVERIFY(text.contains(QStringLiteral("transportModel")));
    QVERIFY(text.contains(QStringLiteral("Newtonian")));
    QVERIFY(text.contains(QStringLiteral("nu")));
    QVERIFY(text.contains(QStringLiteral("0.02")));
}

void ExportWorkflowTest::exportFailureLeavesNoResidue()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    CaseDocumentController document;
    (void)document.project().mesh().setCellCount(0, 0);
    document.notifyProjectEdited();
    QVERIFY(document.validation().hasBlockingErrors());

    ExportController exporter(&document);
    const QString target = tempEntry(temp, QStringLiteral("broken-export"));
    const ExportOutcome outcome = exporter.exportTo(target, false);
    QVERIFY(!outcome.ok);
    QVERIFY(!outcome.message.isEmpty());
    QVERIFY(!QDir(target).exists());
    QVERIFY(!hasTempResidue(temp.path(), QStringLiteral("broken-export")));
}

void ExportWorkflowTest::existingTargetRequiresConfirmation()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    CaseDocumentController document;
    ExportController exporter(&document);

    const QString target = tempEntry(temp, QStringLiteral("existing-export"));
    QVERIFY(QDir().mkpath(target));
    const QString sentinel = QDir(target).filePath(QStringLiteral("keep.txt"));
    {
        QFile file(sentinel);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("do not touch");
    }

    const ExportOutcome blocked = exporter.exportTo(target, false);
    QVERIFY(!blocked.ok);
    QVERIFY(QFileInfo::exists(sentinel));
    QCOMPARE(readTextFile(sentinel), QStringLiteral("do not touch"));
}

void ExportWorkflowTest::overwriteCreatesBackup()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    CaseDocumentController document;
    ExportController exporter(&document);
    const QString target = tempEntry(temp, QStringLiteral("overwrite-export"));

    QVERIFY(exporter.exportTo(target, false).ok);

    document.project().solver().setEndTime(1.5);
    document.notifyProjectEdited();

    const ExportOutcome outcome = exporter.exportTo(target, true);
    QVERIFY2(outcome.ok, qPrintable(outcome.message));
    QVERIFY(!outcome.backupDirectory.isEmpty());
    QVERIFY(QFileInfo::exists(outcome.backupDirectory));

    const QString controlDict = readTextFile(target + QStringLiteral("/system/controlDict"));
    QVERIFY(controlDict.contains(QStringLiteral("endTime")));
    QVERIFY(controlDict.contains(QStringLiteral("1.5")));
}

void ExportWorkflowTest::dictionaryCheckRejectsBrokenText()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    CaseDocumentController document;

    io::CaseExportPlan plan;
    io::ExportFile broken;
    broken.relativePath = "system/controlDict";
    broken.text = "this is not a valid dictionary {{{";
    broken.action = io::ExportAction::Written;
    broken.byteSize = broken.text.size();
    plan.files.push_back(broken);

    const std::vector<core::ValidationIssue> issues =
        checkExportPlan(plan, document.project());
    bool sawInvalid = false;
    for (const core::ValidationIssue& issue : issues)
    {
        if (issue.code() == "EXPORT-DICT-001")
        {
            sawInvalid = true;
            QCOMPARE(issue.severity(), core::Severity::BlockingError);
        }
    }
    QVERIFY(sawInvalid);
}

void ExportWorkflowTest::validationPanelNavigatesToIssueNode()
{
    CaseDocumentController document;
    ValidationPanel panel;
    panel.setDocument(&document);

    (void)document.project().mesh().setCellCount(0, 0);
    document.notifyProjectEdited();
    QVERIFY(panel.issueCount() > 0);

    const int row = rowWithCode(panel, "CASE-MESH-001");
    QVERIFY(row >= 0);
    const ProjectNodeRef ref = panel.nodeRefForRow(row);
    QVERIFY(ref.isValid());
    QCOMPARE(ref.kind, NodeKind::Mesh);

    QSignalSpy spy(&panel, &ValidationPanel::navigateRequested);
    QVERIFY(panel.activateRow(row));
    QCOMPARE(spy.count(), 1);
    const auto emitted = spy.first().first().value<ProjectNodeRef>();
    QCOMPARE(emitted.kind, NodeKind::Mesh);
}

void ExportWorkflowTest::exportActionIsDisabledByBlockingIssue()
{
    MainWindow window;
    auto* action = window.findChild<QAction*>(QStringLiteral("actionExportCase"));
    QVERIFY(action != nullptr);
    QVERIFY(action->isEnabled());

    (void)window.document()->project().mesh().setCellCount(0, 0);
    window.document()->notifyProjectEdited();
    QVERIFY(window.document()->validation().hasBlockingErrors());
    QVERIFY(!action->isEnabled());

    (void)window.document()->project().mesh().setCellCount(0, 20);
    (void)window.document()->project().mesh().setCellCount(1, 20);
    window.document()->notifyProjectEdited();
    QVERIFY(!window.document()->validation().hasBlockingErrors());
    QVERIFY(action->isEnabled());
}

void ExportWorkflowTest::previewDialogReflectsBlockingState()
{
    MainWindow window;
    QTemporaryDir target;
    QVERIFY(target.isValid());

    {
        auto* dialog = new ExportPreviewDialog(window.exportController(), &window);
        dialog->setTargetDirectory(target.path());
        QVERIFY(dialog->fileTree()->topLevelItemCount() > 0);
        QVERIFY(dialog->exportButton()->isEnabled());
        // Destruction while the dialog still owns live child widgets used to
        // assert in QLineEdit::focusOutEvent -> refreshPreview(); keep this
        // scope so the destructor path stays covered.
        delete dialog;
    }

    (void)window.document()->project().mesh().setCellCount(0, 0);
    (void)window.document()->project().mesh().setCellCount(1, 0);
    window.document()->notifyProjectEdited();

    ExportPreviewDialog blocked(window.exportController(), &window);
    blocked.setTargetDirectory(target.path());
    QVERIFY(!blocked.exportButton()->isEnabled());
    QVERIFY(blocked.statusLabel()->text().contains(QStringLiteral("导出预检失败")));
    QVERIFY(blocked.statusLabel()->text().contains(QStringLiteral("CASE-MESH-001")));
}

} // namespace ofgui

QTEST_MAIN(ofgui::ExportWorkflowTest)
#include "ExportWorkflowTest.moc"
