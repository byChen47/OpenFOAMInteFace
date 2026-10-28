#include "ui/MainWindow.h"

#include "render/VtkViewportWidget.h"

#include "ui/ApplicationStyle.h"
#include "ui/CaseDocumentController.h"
#include "ui/ConfigurationPanel.h"
#include "ui/ExportController.h"
#include "ui/InspectorPanel.h"
#include "ui/ProjectTreeView.h"
#include "ui/ValidationPanel.h"

#include <QCloseEvent>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QMessageBox>
#include <QStatusBar>

namespace ofgui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_document(new CaseDocumentController(this))
    , m_exportController(new ExportController(m_document, this))
{
    setObjectName(QStringLiteral("mainWindow"));
    resize(1440, 860);
    setMinimumSize(1100, 680);
    setStyleSheet(applicationStyleSheet());

    createCentralLayout();
    createValidationDock();
    createActions();
    createMenus();
    createToolBar();
    createStatusBar();
    connectDocument();

    updateWindowTitle();
    statusBar()->showMessage(tr("就绪 | 独立 Qt/VTK 界面，不依赖 OpenFOAM 环境"));
}

MainWindow::~MainWindow() = default;

CaseDocumentController* MainWindow::document() const noexcept
{
    return m_document;
}

ConfigurationPanel* MainWindow::configurationPanel() const noexcept
{
    return m_configurationPanel;
}

VtkViewportWidget* MainWindow::viewport() const noexcept
{
    return m_viewport;
}

ExportController* MainWindow::exportController() const noexcept
{
    return m_exportController;
}

ValidationPanel* MainWindow::validationPanel() const noexcept
{
    return m_validationPanel;
}

void MainWindow::connectDocument()
{
    connect(m_document, &CaseDocumentController::documentStateChanged,
            this, &MainWindow::updateWindowTitle);
    connect(m_document, &CaseDocumentController::projectChanged,
            this, &MainWindow::updateWindowTitle);
    connect(m_document, &CaseDocumentController::projectChanged,
            this, &MainWindow::syncDomainToViewport);
    connect(m_document, &CaseDocumentController::validationChanged,
            this, &MainWindow::updateValidationStatus);
    connect(m_document, &CaseDocumentController::validationChanged,
            this, &MainWindow::updateExportAvailability);
    connect(m_document, &CaseDocumentController::projectChanged,
            this, &MainWindow::updateExportAvailability);
    connect(m_exportController, &ExportController::progressMessage, this,
            [this](const QString& message)
            { statusBar()->showMessage(message, 5000); });
    connect(m_document, &CaseDocumentController::errorOccurred, this,
            [this](const QString& message)
            {
                statusBar()->showMessage(message.simplified(), 8000);
            });

    syncDomainToViewport();
    updateExportAvailability();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (!maybeSaveChanges())
    {
        event->ignore();
        return;
    }
    event->accept();
}

void MainWindow::updateWindowTitle()
{
    const QString name = m_document->displayName();
    setWindowTitle(tr("%1[*] — OpenFOAM 可视化前处理").arg(name));
    setWindowModified(m_document->isDirty());
}

void MainWindow::updateValidationStatus()
{
    if (m_validationLabel == nullptr)
    {
        return;
    }
    m_validationLabel->setText(
        tr("校验：%1").arg(QString::fromStdString(m_document->validation().summary())));
}

void MainWindow::syncDomainToViewport()
{
    m_viewport->setDocumentDomain(m_document->project().domain());
}

QString MainWindow::defaultDirectory() const
{
    const QString caseDirectory = m_document->caseDirectory();
    if (!caseDirectory.isEmpty() && QFileInfo::exists(caseDirectory))
    {
        return caseDirectory;
    }
    if (!m_lastDirectory.isEmpty() && QFileInfo::exists(m_lastDirectory))
    {
        return m_lastDirectory;
    }
    const QString projectDirectory = QFileInfo(m_document->projectFilePath()).absolutePath();
    if (!m_document->projectFilePath().isEmpty() && QFileInfo::exists(projectDirectory))
    {
        return projectDirectory;
    }
    return QDir::homePath();
}

void MainWindow::showError(const QString& message)
{
    if (message.trimmed().isEmpty())
    {
        return;
    }
    QMessageBox::warning(this, tr("操作失败"), message);
}

void MainWindow::showAboutDialog()
{
    QMessageBox::about(
        this,
        tr("关于 OpenFOAM 可视化前处理"),
        tr("<h3>OpenFOAM 可视化前处理</h3>"
           "<p>独立 Qt 6 + VTK 桌面界面。</p>"
           "<p>版本：0.1.0（M0 技术基线）</p>"
           "<p>本程序不编译、不链接 OpenFOAM。</p>"));
}

} // namespace ofgui
