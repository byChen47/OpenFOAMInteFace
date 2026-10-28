// Preview concern of ExportPreviewDialog: plan listing, per-file text preview
// and status/button state. Kept in its own translation unit because the dialog
// file is over the 400-line limit (D-021).
#include "ui/ExportPreviewDialog.h"

#include "ui/ExportController.h"

#include "io/writers/ExportAction.h"

#include <QCheckBox>
#include <QDir>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <cstddef>
#include <string>

namespace ofgui {
namespace {

constexpr int kFileIndexRole = Qt::UserRole + 1;

QString actionLabel(io::ExportAction action)
{
    switch (action)
    {
    case io::ExportAction::Written:
        return QCoreApplication::translate("ExportPreviewDialog", "新写");
    case io::ExportAction::Copied:
        return QCoreApplication::translate("ExportPreviewDialog", "复制");
    case io::ExportAction::Mirrored:
        return QCoreApplication::translate("ExportPreviewDialog", "镜像");
    case io::ExportAction::Skipped:
        return QCoreApplication::translate("ExportPreviewDialog", "跳过");
    }
    return QCoreApplication::translate("ExportPreviewDialog", "新写");
}

QString byteSizeText(std::uintmax_t bytes)
{
    if (bytes >= 1024U * 1024U)
    {
        return QCoreApplication::translate("ExportPreviewDialog", "%1 MB")
            .arg(static_cast<double>(bytes) / (1024.0 * 1024.0), 0, 'f', 2);
    }
    if (bytes >= 1024U)
    {
        return QCoreApplication::translate("ExportPreviewDialog", "%1 KB")
            .arg(static_cast<double>(bytes) / 1024.0, 0, 'f', 1);
    }
    return QCoreApplication::translate("ExportPreviewDialog", "%1 B").arg(bytes);
}

} // namespace

void ExportPreviewDialog::refreshPreview()
{
    m_fileTree->clear();
    m_textPreview->clear();
    m_plan = io::CaseExportPlan{};
    m_previewOk = false;
    m_previewError.clear();
    m_targetHasContent = false;

    if (m_controller == nullptr)
    {
        m_previewError = tr("导出控制器不可用。");
    }
    else if (targetDirectory().isEmpty())
    {
        m_previewError = tr("请选择导出目标目录。");
    }
    else
    {
        const ExportPreview preview = m_controller->preview(targetDirectory());
        m_plan = preview.plan;
        m_targetHasContent = preview.targetHasContent;
        m_previewOk = preview.ok;
        m_previewError = preview.errorText;
    }

    for (std::size_t index = 0; index < m_plan.files.size(); ++index)
    {
        const io::ExportFile& file = m_plan.files[index];
        auto* item = new QTreeWidgetItem(m_fileTree);
        item->setText(0, QString::fromStdString(file.relativePath));
        item->setText(1, actionLabel(file.action));
        item->setText(2, byteSizeText(file.byteSize));
        item->setData(0, kFileIndexRole, static_cast<int>(index));
        if (file.isCopy())
        {
            item->setToolTip(
                0, tr("来源：%1").arg(QDir::toNativeSeparators(
                       QString::fromStdString(file.sourcePath.string()))));
        }
    }
    for (const io::ExportSkippedFile& skipped : m_plan.skipped)
    {
        auto* item = new QTreeWidgetItem(m_fileTree);
        item->setText(0, QString::fromStdString(skipped.relativePath));
        item->setText(1, actionLabel(io::ExportAction::Skipped));
        item->setText(2, QStringLiteral("—"));
        item->setToolTip(0, QString::fromStdString(skipped.reason));
    }

    if (m_fileTree->topLevelItemCount() > 0)
    {
        m_fileTree->setCurrentItem(m_fileTree->topLevelItem(0));
    }
    updateExportEnabled();
    updateTextPreview();
}

void ExportPreviewDialog::updateExportEnabled()
{
    const bool hasTarget = !targetDirectory().isEmpty();
    bool enabled = hasTarget && m_previewOk;
    QString status;

    if (m_controller == nullptr)
    {
        status = tr("导出控制器不可用。");
    }
    else if (!hasTarget)
    {
        status = tr("请选择导出目标目录，预览会列出将要写入的文件。");
    }
    else if (!m_previewOk)
    {
        status = tr("导出预检失败：\n%1").arg(m_previewError);
    }
    else
    {
        status = tr("计划写入 %1 个文件（新写 %2、复制 %3、镜像 %4），跳过 %5。")
                     .arg(static_cast<qulonglong>(m_plan.size()))
                     .arg(static_cast<qulonglong>(m_plan.count(io::ExportAction::Written)))
                     .arg(static_cast<qulonglong>(m_plan.count(io::ExportAction::Copied)))
                     .arg(static_cast<qulonglong>(m_plan.count(io::ExportAction::Mirrored)))
                     .arg(static_cast<qulonglong>(m_plan.skipped.size()));
        if (m_targetHasContent)
        {
            status += QLatin1Char('\n')
                      + tr("目标目录已有文件：勾选“覆盖…”后才能导出，旧目录会先备份。");
            enabled = m_overwriteCheck->isChecked();
        }
    }

    m_statusLabel->setText(status);
    m_exportButton->setEnabled(enabled);
}

void ExportPreviewDialog::updateTextPreview()
{
    m_textPreview->clear();
    QTreeWidgetItem* item = m_fileTree->currentItem();
    if (item == nullptr)
    {
        return;
    }
    const QVariant data = item->data(0, kFileIndexRole);
    if (!data.isValid())
    {
        m_textPreview->setPlainText(tr("该文件被跳过，原因：\n%1")
                                        .arg(item->toolTip(0)));
        return;
    }
    const int index = data.toInt();
    if (index < 0 || static_cast<std::size_t>(index) >= m_plan.files.size())
    {
        return;
    }
    const io::ExportFile& file = m_plan.files[static_cast<std::size_t>(index)];
    if (file.isCopy())
    {
        m_textPreview->setPlainText(
            tr("该文件不从模型生成，导出时%s：\n%1")
                .arg(QDir::toNativeSeparators(
                    QString::fromStdString(file.sourcePath.string())))
                .arg(tr("原样复制")));
        return;
    }
    m_textPreview->setPlainText(QString::fromStdString(file.text));
}

void ExportPreviewDialog::showPlanItem(QTreeWidgetItem* item)
{
    Q_UNUSED(item)
    updateTextPreview();
}

} // namespace ofgui
