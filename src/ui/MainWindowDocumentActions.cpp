// Document lifecycle actions of MainWindow: new / open case / open project /
// save / save-as / close and the dirty-document prompt. Split out of
// MainWindow.cpp to keep each file under the 400-line limit (D-021).
#include "ui/MainWindow.h"

#include "render/VtkViewportWidget.h"

#include "ui/CaseDocumentController.h"
#include "ui/ConfigurationPanel.h"
#include "ui/ProjectTreeView.h"

#include "core/ValidationIssue.h"
#include "io/ImportReport.h"

#include <QAbstractButton>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>
#include <QStatusBar>

#include <cstddef>

namespace ofgui {
namespace {

std::size_t blockingIssueCount(const io::ImportReport* report)
{
    if (report == nullptr)
    {
        return 0;
    }
    std::size_t count = 0;
    for (const core::ValidationIssue& issue : report->issues)
    {
        count += issue.isBlocking() ? 1U : 0U;
    }
    return count;
}

} // namespace

void MainWindow::newProject()
{
    if (!maybeSaveChanges())
    {
        return;
    }
    m_document->newProject();
    m_configurationPanel->treeView()->selectProject();
    m_viewport->fitView();
    statusBar()->showMessage(tr("已新建空工程"), 5000);
}

void MainWindow::promptOpenCase()
{
    const QString directory =
        QFileDialog::getExistingDirectory(this, tr("选择 OpenFOAM 算例目录"), defaultDirectory());
    if (!directory.isEmpty())
    {
        openCaseDirectory(directory);
    }
}

void MainWindow::promptOpenProject()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        tr("打开工程"),
        defaultDirectory(),
        tr("OpenFOAM GUI 工程 (*.ofgproj)"));
    if (!path.isEmpty())
    {
        openProjectFile(path);
    }
}

bool MainWindow::openCaseDirectory(const QString& directory)
{
    if (!maybeSaveChanges())
    {
        return false;
    }
    if (!m_document->openCaseDirectory(directory))
    {
        showError(m_document->lastError());
        return false;
    }

    m_lastDirectory = QFileInfo(directory).absolutePath();
    m_configurationPanel->treeView()->selectProject();
    m_viewport->fitView();

    const io::ImportReport* report = m_document->importReport();
    const std::size_t fileCount = report != nullptr ? report->files.size() : 0;
    const std::size_t blocking = blockingIssueCount(report);
    statusBar()->showMessage(
        tr("已加载算例：%1 个文件，%2 个校验问题（阻断 %3）")
            .arg(static_cast<qulonglong>(fileCount))
            .arg(static_cast<qulonglong>(report != nullptr ? report->issues.size() : 0))
            .arg(static_cast<qulonglong>(blocking)),
        10000);
    return true;
}

bool MainWindow::openProjectFile(const QString& path)
{
    if (!maybeSaveChanges())
    {
        return false;
    }
    if (!m_document->openProjectFile(path))
    {
        showError(m_document->lastError());
        return false;
    }

    m_lastDirectory = QFileInfo(path).absolutePath();
    m_configurationPanel->treeView()->selectProject();
    m_viewport->fitView();
    const QString transportHint =
        m_document->caseDirectory().isEmpty()
            ? QString()
            : tr("\ntransportModel/nu 不随 .ofgproj 保存，已按源算例 constant/transportProperties 重读。");
    statusBar()->showMessage(
        tr("已加载工程：%1").arg(QDir::toNativeSeparators(path)) + transportHint, 8000);
    return true;
}

bool MainWindow::saveProject()
{
    if (m_document->projectFilePath().isEmpty())
    {
        return saveProjectAs();
    }
    if (!m_document->saveProject())
    {
        showError(m_document->lastError());
        return false;
    }
    statusBar()->showMessage(
        tr("已保存：%1").arg(QDir::toNativeSeparators(m_document->projectFilePath()))
            + tr("\ntransportModel/nu 不随 .ofgproj 保存，重开工程将按源算例重读。"),
        8000);
    return true;
}

bool MainWindow::saveProjectAs()
{
    QString suggestedName = m_document->displayName().trimmed();
    if (suggestedName.isEmpty())
    {
        suggestedName = tr("未命名工程");
    }
    const QString suggested =
        QDir(defaultDirectory()).filePath(suggestedName + QStringLiteral(".ofgproj"));

    const QString path = QFileDialog::getSaveFileName(
        this,
        tr("工程另存为"),
        suggested,
        tr("OpenFOAM GUI 工程 (*.ofgproj)"));
    if (path.isEmpty())
    {
        return false;
    }
    if (!m_document->saveProjectAs(path))
    {
        showError(m_document->lastError());
        return false;
    }

    m_lastDirectory = QFileInfo(path).absolutePath();
    statusBar()->showMessage(
        tr("已保存：%1").arg(QDir::toNativeSeparators(m_document->projectFilePath()))
            + tr("\ntransportModel/nu 不随 .ofgproj 保存，重开工程将按源算例重读。"),
        8000);
    return true;
}

void MainWindow::closeDocument()
{
    if (!maybeSaveChanges())
    {
        return;
    }
    m_document->newProject();
    m_configurationPanel->treeView()->selectProject();
    m_viewport->fitView();
    statusBar()->showMessage(tr("已关闭工程"), 5000);
}

bool MainWindow::maybeSaveChanges()
{
    if (!m_document->isDirty())
    {
        return true;
    }

    QMessageBox box(this);
    box.setObjectName(QStringLiteral("unsavedChangesBox"));
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle(tr("未保存的修改"));
    box.setText(tr("工程“%1”有未保存的修改。").arg(m_document->displayName()));
    box.setInformativeText(tr("是否先保存再继续？"));
    QPushButton* saveButton = box.addButton(tr("保存"), QMessageBox::AcceptRole);
    QPushButton* discardButton = box.addButton(tr("不保存"), QMessageBox::DestructiveRole);
    box.addButton(tr("取消"), QMessageBox::RejectRole);
    box.setDefaultButton(saveButton);
    box.exec();

    if (box.clickedButton() == static_cast<QAbstractButton*>(saveButton))
    {
        return saveProject();
    }
    if (box.clickedButton() == static_cast<QAbstractButton*>(discardButton))
    {
        return true;
    }
    return false;
}

} // namespace ofgui
