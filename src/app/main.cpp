#include "app/ApplicationBootstrap.h"
#include "app/SmokeTestController.h"
#include "core/CaseProject.h"
#include "ui/CaseDocumentController.h"
#include "ui/ConfigurationPanel.h"
#include "ui/ExportPreviewDialog.h"
#include "ui/InspectorPanel.h"
#include "ui/MainWindow.h"
#include "ui/ProjectNodeRef.h"
#include "ui/ProjectTreeView.h"
#include "ui/SolverEditor.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QStringList>

int main(int argc, char* argv[])
{
    ofgui::configureOpenGLSurfaceFormat();

    QApplication application(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("OpenFOAMGUI"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));
    QCoreApplication::setOrganizationName(QStringLiteral("OpenFOAMGUI"));

    ofgui::MainWindow window;
    window.show();

    // Startup hook for scripted smoke tests and screenshots: open a case before
    // the optional --screenshot capture fires.
    const QStringList arguments = QCoreApplication::arguments();
    const int openCaseIndex = arguments.indexOf(QStringLiteral("--open-case"));
    if (openCaseIndex >= 0 && openCaseIndex + 1 < arguments.size())
    {
        window.openCaseDirectory(arguments.at(openCaseIndex + 1));
    }

    // Screenshot hook: show the editable domain page (t9) without user input.
    if (arguments.contains(QStringLiteral("--select-domain")))
    {
        window.configurationPanel()->treeView()->selectNode(
            ofgui::ProjectNodeRef::section(ofgui::NodeKind::Domain));
    }

    // Screenshot hook: show a field page (t10) by field name.
    const int selectFieldIndex = arguments.indexOf(QStringLiteral("--select-field"));
    if (selectFieldIndex >= 0 && selectFieldIndex + 1 < arguments.size() && window.document())
    {
        const auto* field = window.document()->project().findFieldByName(
            arguments.at(selectFieldIndex + 1).toStdString());
        if (field)
        {
            window.configurationPanel()->treeView()->selectNode(
                ofgui::ProjectNodeRef::forField(field->id()));
        }
    }

    // Screenshot hook: show the solver settings page (t11) without user input.
    if (arguments.contains(QStringLiteral("--select-solver")))
    {
        window.configurationPanel()->treeView()->selectNode(
            ofgui::ProjectNodeRef::section(ofgui::NodeKind::Solver));
        if (arguments.contains(QStringLiteral("--solver-transport")))
        {
            window.configurationPanel()->inspector()->solverEditor()->tabs()->setCurrentIndex(1);
        }
    }

    // Screenshot hook: show the export preview dialog (t12) non-modally so the
    // regular --screenshot timer can still capture it. `--export-blocked` first
    // injects a blocking mesh value to capture the "export is blocked" state.
    QWidget* screenshotTarget = &window;
    if (arguments.contains(QStringLiteral("--export-preview")))
    {
        if (arguments.contains(QStringLiteral("--export-blocked")))
        {
            (void)window.document()->project().mesh().setCellCount(0, 0);
            window.document()->notifyProjectEdited();
        }

        auto* dialog = new ofgui::ExportPreviewDialog(window.exportController(), &window);
        const int targetIndex = arguments.indexOf(QStringLiteral("--export-target"));
        const QString target =
            (targetIndex >= 0 && targetIndex + 1 < arguments.size())
                ? arguments.at(targetIndex + 1)
                : QDir(QDir::tempPath()).filePath(QStringLiteral("ofgui-export-preview"));
        dialog->setTargetDirectory(target);
        dialog->resize(1000, 660);
        dialog->show();
        dialog->raise();
        dialog->activateWindow();
        screenshotTarget = dialog;
    }

    ofgui::scheduleSmokeTest(application, *screenshotTarget, arguments);

    return application.exec();
}
