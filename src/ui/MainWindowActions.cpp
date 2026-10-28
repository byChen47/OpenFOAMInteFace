// Construction of the window chrome (actions, menus, toolbar, central splitter,
// validation dock, status bar). Split out of MainWindow.cpp to keep each file
// under the 400-line limit (D-021).
#include "ui/MainWindow.h"

#include "render/VtkViewportWidget.h"

#include "ui/CaseDocumentController.h"
#include "ui/ConfigurationPanel.h"
#include "ui/ProjectTreeView.h"
#include "ui/ValidationPanel.h"

#include <QAction>
#include <QDockWidget>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QProgressBar>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>

namespace ofgui {

void MainWindow::createActions()
{
    m_newAction = new QAction(tr("新建工程(&N)"), this);
    m_newAction->setObjectName(QStringLiteral("actionNewProject"));
    m_newAction->setShortcut(QKeySequence::New);
    connect(m_newAction, &QAction::triggered, this, &MainWindow::newProject);

    m_openCaseAction = new QAction(tr("打开算例(&O)..."), this);
    m_openCaseAction->setObjectName(QStringLiteral("actionOpenCase"));
    m_openCaseAction->setShortcut(QKeySequence::Open);
    m_openCaseAction->setToolTip(tr("选择一个包含 0/、constant/、system/ 的 OpenFOAM 算例目录"));
    connect(m_openCaseAction, &QAction::triggered, this, &MainWindow::promptOpenCase);

    m_openProjectAction = new QAction(tr("打开工程(&P)..."), this);
    m_openProjectAction->setObjectName(QStringLiteral("actionOpenProject"));
    m_openProjectAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+O")));
    connect(m_openProjectAction, &QAction::triggered, this, &MainWindow::promptOpenProject);

    m_saveAction = new QAction(tr("保存(&S)"), this);
    m_saveAction->setObjectName(QStringLiteral("actionSaveProject"));
    m_saveAction->setShortcut(QKeySequence::Save);
    connect(m_saveAction, &QAction::triggered, this, &MainWindow::saveProject);

    m_saveAsAction = new QAction(tr("另存为(&A)..."), this);
    m_saveAsAction->setObjectName(QStringLiteral("actionSaveProjectAs"));
    m_saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(m_saveAsAction, &QAction::triggered, this, &MainWindow::saveProjectAs);

    m_closeAction = new QAction(tr("关闭工程(&W)"), this);
    m_closeAction->setObjectName(QStringLiteral("actionCloseProject"));
    m_closeAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+W")));
    connect(m_closeAction, &QAction::triggered, this, &MainWindow::closeDocument);

    m_exportAction = new QAction(tr("导出算例(&E)..."), this);
    m_exportAction->setObjectName(QStringLiteral("actionExportCase"));
    m_exportAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+E")));
    connect(m_exportAction, &QAction::triggered, this, &MainWindow::promptExport);
}

void MainWindow::createMenus()
{
    auto* fileMenu = menuBar()->addMenu(tr("文件(&F)"));
    fileMenu->addAction(m_newAction);
    fileMenu->addAction(m_openCaseAction);
    fileMenu->addAction(m_openProjectAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_saveAction);
    fileMenu->addAction(m_saveAsAction);
    fileMenu->addAction(m_exportAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_closeAction);
    fileMenu->addSeparator();

    auto* quitAction = new QAction(tr("退出(&X)"), this);
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(quitAction);

    auto* viewMenu = menuBar()->addMenu(tr("视图(&V)"));

    auto* fitAction = new QAction(tr("适配视图(&F)"), this);
    fitAction->setShortcut(QKeySequence(Qt::Key_F));
    connect(fitAction, &QAction::triggered, m_viewport, &VtkViewportWidget::fitView);
    viewMenu->addAction(fitAction);

    auto* resetAction = new QAction(tr("重置视角(&R)"), this);
    resetAction->setShortcut(QKeySequence(Qt::Key_R));
    connect(resetAction, &QAction::triggered, m_viewport, &VtkViewportWidget::resetView);
    viewMenu->addAction(resetAction);

    viewMenu->addSeparator();
    viewMenu->addAction(m_validationDock->toggleViewAction());

    auto* helpMenu = menuBar()->addMenu(tr("帮助(&H)"));
    auto* aboutAction = new QAction(tr("关于(&A)"), this);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAboutDialog);
    helpMenu->addAction(aboutAction);
}

void MainWindow::createToolBar()
{
    auto* toolbar = addToolBar(tr("主工具栏"));
    toolbar->setObjectName(QStringLiteral("mainToolBar"));
    toolbar->setMovable(false);

    toolbar->addAction(m_newAction);
    toolbar->addAction(m_openCaseAction);
    toolbar->addAction(m_saveAction);
    toolbar->addSeparator();
    toolbar->addAction(m_exportAction);
    toolbar->addSeparator();

    auto* fitAction = new QAction(tr("适配视图"), this);
    fitAction->setToolTip(tr("将计算域完整适配到三维视口 (F)"));
    connect(fitAction, &QAction::triggered, m_viewport, &VtkViewportWidget::fitView);
    toolbar->addAction(fitAction);

    auto* resetAction = new QAction(tr("重置视角"), this);
    resetAction->setToolTip(tr("恢复默认等轴测视角 (R)"));
    connect(resetAction, &QAction::triggered, m_viewport, &VtkViewportWidget::resetView);
    toolbar->addAction(resetAction);
}

void MainWindow::createCentralLayout()
{
    m_configurationPanel = new ConfigurationPanel(this);
    m_configurationPanel->setDocument(m_document);
    m_viewport = new VtkViewportWidget(this);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setObjectName(QStringLiteral("mainSplitter"));
    splitter->setChildrenCollapsible(false);
    splitter->addWidget(m_configurationPanel);
    splitter->addWidget(m_viewport);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({360, 1080});

    setCentralWidget(splitter);
}

void MainWindow::createValidationDock()
{
    m_validationDock = new QDockWidget(tr("校验问题"), this);
    m_validationDock->setObjectName(QStringLiteral("validationDock"));
    m_validationDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    m_validationDock->setFeatures(QDockWidget::DockWidgetMovable
                                  | QDockWidget::DockWidgetClosable);

    m_validationPanel = new ValidationPanel(m_validationDock);
    m_validationPanel->setDocument(m_document);
    m_validationDock->setWidget(m_validationPanel);
    addDockWidget(Qt::BottomDockWidgetArea, m_validationDock);
    resizeDocks({m_validationDock}, {160}, Qt::Vertical);

    connect(m_validationPanel, &ValidationPanel::navigateRequested, this,
            [this](ProjectNodeRef ref)
            {
                m_configurationPanel->treeView()->selectNode(ref);
                if (m_validationDock != nullptr)
                {
                    m_validationDock->setVisible(true);
                }
            });
}

void MainWindow::createStatusBar()
{
    m_validationLabel = new QLabel(this);
    m_validationLabel->setObjectName(QStringLiteral("statusValidation"));
    statusBar()->addPermanentWidget(m_validationLabel);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setObjectName(QStringLiteral("exportProgress"));
    m_progressBar->setMaximumWidth(180);
    m_progressBar->setTextVisible(false);
    m_progressBar->setVisible(false);
    statusBar()->addPermanentWidget(m_progressBar);

    updateValidationStatus();
}

} // namespace ofgui
