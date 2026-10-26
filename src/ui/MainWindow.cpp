#include "ui/MainWindow.h"

#include "render/VtkViewportWidget.h"
#include "ui/ApplicationStyle.h"
#include "ui/ConfigurationPanel.h"

#include <QAction>
#include <QApplication>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>

namespace ofgui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setObjectName(QStringLiteral("mainWindow"));
    setWindowTitle(tr("OpenFOAM 可视化前处理"));
    resize(1440, 860);
    setMinimumSize(1100, 680);
    setStyleSheet(applicationStyleSheet());

    createCentralLayout();
    createMenus();
    createToolBar();

    statusBar()->showMessage(tr("就绪 | 独立 Qt/VTK 界面，不依赖 OpenFOAM 环境"));
}

void MainWindow::createMenus()
{
    auto* fileMenu = menuBar()->addMenu(tr("文件(&F)"));

    auto* quitAction = new QAction(tr("退出(&X)"), this);
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);
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