#include "ui/ExportPreviewDialog.h"

#include "ui/ExportController.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace ofgui {

ExportPreviewDialog::ExportPreviewDialog(ExportController* controller, QWidget* parent)
    : QDialog(parent)
    , m_controller(controller)
{
    setObjectName(QStringLiteral("exportPreviewDialog"));
    setWindowTitle(tr("导出算例"));
    resize(980, 640);

    auto* targetLabel = new QLabel(tr("目标目录："), this);
    m_targetEdit = new QLineEdit(this);
    m_targetEdit->setObjectName(QStringLiteral("exportTargetEdit"));
    m_targetEdit->setPlaceholderText(tr("选择导出到的空目录或不存在的新目录"));
    auto* browseButton = new QPushButton(tr("浏览…"), this);
    browseButton->setObjectName(QStringLiteral("browseExportTarget"));

    auto* targetRow = new QHBoxLayout;
    targetRow->setSpacing(6);
    targetRow->addWidget(targetLabel);
    targetRow->addWidget(m_targetEdit, 1);
    targetRow->addWidget(browseButton);

    m_overwriteCheck = new QCheckBox(tr("覆盖目标目录中的已有文件（旧目录会先备份为 .bak-…）"), this);
    m_overwriteCheck->setObjectName(QStringLiteral("overwriteExportCheck"));

    auto* optionsRow = new QHBoxLayout;
    optionsRow->addWidget(m_overwriteCheck);
    optionsRow->addStretch(1);

    m_fileTree = new QTreeWidget(this);
    m_fileTree->setObjectName(QStringLiteral("exportFileTree"));
    m_fileTree->setColumnCount(3);
    m_fileTree->setHeaderLabels({tr("文件"), tr("动作"), tr("大小")});
    m_fileTree->setUniformRowHeights(true);
    m_fileTree->setAlternatingRowColors(true);
    m_fileTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_fileTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_fileTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

    m_textPreview = new QPlainTextEdit(this);
    m_textPreview->setObjectName(QStringLiteral("exportTextPreview"));
    m_textPreview->setReadOnly(true);
    m_textPreview->setLineWrapMode(QPlainTextEdit::NoWrap);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setObjectName(QStringLiteral("exportPreviewSplitter"));
    splitter->addWidget(m_fileTree);
    splitter->addWidget(m_textPreview);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);
    splitter->setSizes({380, 580});

    m_statusLabel = new QLabel(this);
    m_statusLabel->setObjectName(QStringLiteral("exportStatusLabel"));
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_exportButton = buttons->button(QDialogButtonBox::Ok);
    m_exportButton->setObjectName(QStringLiteral("exportButton"));
    m_exportButton->setText(tr("导出"));
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("取消"));

    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(6);
    layout->addLayout(targetRow);
    layout->addLayout(optionsRow);
    layout->addWidget(splitter, 1);
    layout->addWidget(m_statusLabel);
    layout->addWidget(buttons);

    connect(browseButton, &QPushButton::clicked, this, &ExportPreviewDialog::chooseTarget);
    connect(m_targetEdit, &QLineEdit::editingFinished, this, &ExportPreviewDialog::refreshPreview);
    connect(m_targetEdit, &QLineEdit::returnPressed, this, &ExportPreviewDialog::refreshPreview);
    connect(m_targetEdit, &QLineEdit::textChanged, this, &ExportPreviewDialog::updateExportEnabled);
    connect(m_overwriteCheck, &QCheckBox::toggled, this, &ExportPreviewDialog::updateExportEnabled);
    connect(m_fileTree, &QTreeWidget::currentItemChanged, this,
            [this](QTreeWidgetItem* item, QTreeWidgetItem*) { showPlanItem(item); });
    connect(buttons, &QDialogButtonBox::accepted, this, &ExportPreviewDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    updateExportEnabled();
}

ExportPreviewDialog::~ExportPreviewDialog()
{
    // Tearing down a visible dialog hides it, which makes the target edit lose
    // focus and emit editingFinished while this subclass is already destroyed.
    // Child widgets also emit currentItemChanged while being deleted. Drop every
    // child-to-this connection first so the base destructor stays signal-free.
    const auto children = findChildren<QObject*>();
    for (QObject* child : children) {
        disconnect(child, nullptr, this, nullptr);
    }
    if (m_fileTree != nullptr) {
        m_fileTree->clear();
    }
}

void ExportPreviewDialog::setTargetDirectory(const QString& directory)
{
    m_targetEdit->setText(QDir::toNativeSeparators(directory));
    refreshPreview();
}

QString ExportPreviewDialog::targetDirectory() const
{
    return m_targetEdit->text().trimmed();
}

bool ExportPreviewDialog::allowExistingTarget() const
{
    return m_targetHasContent && m_overwriteCheck->isChecked();
}

bool ExportPreviewDialog::targetHasContent() const
{
    return m_targetHasContent;
}

QLineEdit* ExportPreviewDialog::targetEdit() const noexcept
{
    return m_targetEdit;
}

QCheckBox* ExportPreviewDialog::overwriteCheck() const noexcept
{
    return m_overwriteCheck;
}

QTreeWidget* ExportPreviewDialog::fileTree() const noexcept
{
    return m_fileTree;
}

QPlainTextEdit* ExportPreviewDialog::textPreview() const noexcept
{
    return m_textPreview;
}

QLabel* ExportPreviewDialog::statusLabel() const noexcept
{
    return m_statusLabel;
}

QPushButton* ExportPreviewDialog::exportButton() const noexcept
{
    return m_exportButton;
}

void ExportPreviewDialog::chooseTarget()
{
    const QString directory = QFileDialog::getExistingDirectory(
        this,
        tr("选择导出目标目录"),
        targetDirectory().isEmpty() ? QDir::homePath() : targetDirectory());
    if (directory.isEmpty())
    {
        return;
    }
    setTargetDirectory(directory);
}

void ExportPreviewDialog::accept()
{
    if (targetDirectory().isEmpty())
    {
        QMessageBox::warning(this, tr("导出算例"), tr("请选择导出目标目录。"));
        return;
    }

    // The preview may be stale if the user typed a path and pressed the button
    // directly; refresh once so the confirmation reflects the real target.
    refreshPreview();

    if (!m_previewOk)
    {
        QMessageBox::warning(this, tr("无法导出"), m_previewError);
        return;
    }
    if (m_targetHasContent && !m_overwriteCheck->isChecked())
    {
        QMessageBox::warning(
            this, tr("无法导出"),
            tr("目标目录已有文件。请勾选“覆盖目标目录中的已有文件”后重试。"));
        return;
    }

    if (allowExistingTarget())
    {
        QMessageBox confirm(this);
        confirm.setObjectName(QStringLiteral("overwriteExportBox"));
        confirm.setIcon(QMessageBox::Warning);
        confirm.setWindowTitle(tr("确认覆盖"));
        confirm.setText(tr("目标目录“%1”中已有文件，继续导出会先备份旧目录为 .bak-…，确定覆盖吗？")
                            .arg(QDir::toNativeSeparators(targetDirectory())));
        auto* overwriteButton =
            confirm.addButton(tr("覆盖"), QMessageBox::DestructiveRole);
        confirm.addButton(tr("取消"), QMessageBox::RejectRole);
        confirm.setDefaultButton(qobject_cast<QPushButton*>(overwriteButton));
        confirm.exec();
        if (confirm.clickedButton() != overwriteButton)
        {
            return;
        }
    }

    QDialog::accept();
}

} // namespace ofgui
