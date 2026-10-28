// Export workflow actions of MainWindow: export dialog handoff, progress
// reporting, availability guarding and the default target directory. Split out
// of MainWindow.cpp to keep each file under the 400-line limit (D-021).
#include "ui/MainWindow.h"

#include "ui/CaseDocumentController.h"
#include "ui/ExportController.h"
#include "ui/ExportPreviewDialog.h"
#include "ui/ValidationPanel.h"

#include <QAction>
#include <QDir>
#include <QDockWidget>
#include <QFileInfo>
#include <QMessageBox>
#include <QProgressBar>
#include <QStatusBar>

namespace ofgui {

void MainWindow::promptExport()
{
    if (m_document->validation().hasErrors() || m_document->validation().hasBlockingErrors())
    {
        m_validationDock->setVisible(true);
        m_validationPanel->refresh();
        QMessageBox::warning(
            this, tr("无法导出"),
            tr("工程存在错误或阻断问题（%1）。\n请先在校验问题面板中双击问题定位并修复。")
                .arg(QString::fromStdString(m_document->validation().summary())));
        return;
    }

    ExportPreviewDialog dialog(m_exportController, this);
    dialog.setTargetDirectory(defaultExportDirectory());
    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    if (!exportCase(dialog.targetDirectory(), dialog.allowExistingTarget()))
    {
        m_validationDock->setVisible(true);
        m_validationPanel->refresh();
        QMessageBox::warning(this, tr("导出失败"), m_lastExportMessage);
        return;
    }

    QMessageBox::information(
        this, tr("导出完成"),
        tr("%1\n\n目标目录：%2")
            .arg(m_lastExportMessage,
                 QDir::toNativeSeparators(dialog.targetDirectory())));
}

bool MainWindow::exportCase(const QString& targetDirectory, bool allowExistingTarget)
{
    if (m_progressBar != nullptr)
    {
        m_progressBar->setRange(0, 0);
        m_progressBar->setVisible(true);
    }

    const ExportOutcome outcome =
        m_exportController->exportTo(targetDirectory, allowExistingTarget);

    if (m_progressBar != nullptr)
    {
        m_progressBar->setVisible(false);
        m_progressBar->setRange(0, 1);
        m_progressBar->setValue(1);
    }

    m_lastExportMessage = outcome.message;
    if (!outcome.ok)
    {
        m_lastExportMessage = outcome.message.isEmpty() ? tr("导出失败。") : outcome.message;
        statusBar()->showMessage(tr("导出失败：%1").arg(m_lastExportMessage.simplified()), 15000);
        return false;
    }

    if (outcome.manifestVerified)
    {
        m_lastExportMessage += tr("\nmanifest 记录 %1 个文件。")
                                   .arg(outcome.manifestFiles);
    }
    m_lastDirectory = QFileInfo(targetDirectory).absolutePath();
    statusBar()->showMessage(m_lastExportMessage.simplified(), 10000);
    return true;
}

void MainWindow::updateExportAvailability()
{
    if (m_exportAction == nullptr)
    {
        return;
    }
    const bool blocked = m_document->validation().hasErrors()
                         || m_document->validation().hasBlockingErrors();
    m_exportAction->setEnabled(!blocked);
    m_exportAction->setToolTip(
        blocked ? tr("工程存在错误或阻断问题，请先在校验问题面板修复 (Ctrl+E)")
                : tr("预览导出文件并写入目标目录 (Ctrl+E)"));
}

QString MainWindow::defaultExportDirectory() const
{
    QString name = m_document->displayName().trimmed();
    if (name.isEmpty())
    {
        name = tr("case");
    }
    return QDir(defaultDirectory()).filePath(name + QStringLiteral("-export"));
}

} // namespace ofgui
