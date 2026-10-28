#include "ui/ExportPreviewPanel.h"

#include "core/CaseProject.h"
#include "core/CaseValidator.h"
#include "core/FieldDefinition.h"
#include "core/ValidationResult.h"
#include "io/writers/FieldFileWriter.h"
#include "ui/CaseDocumentController.h"

#include <QComboBox>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QVBoxLayout>

namespace ofgui {

ExportPreviewPanel::ExportPreviewPanel(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("exportPreviewPanel"));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    auto* fileRow = new QHBoxLayout();
    fileRow->setContentsMargins(0, 0, 0, 0);
    auto* fileLabel = new QLabel(tr("文件"), this);
    fileRow->addWidget(fileLabel);
    m_combo = new QComboBox(this);
    m_combo->setObjectName(QStringLiteral("exportFileCombo"));
    m_combo->setMinimumWidth(120);
    fileRow->addWidget(m_combo, 1);
    layout->addLayout(fileRow);

    m_view = new QPlainTextEdit(this);
    m_view->setObjectName(QStringLiteral("exportPreviewText"));
    m_view->setReadOnly(true);
    QFont fixed = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    fixed.setPointSize(9);
    m_view->setFont(fixed);
    m_view->setMinimumHeight(240);
    layout->addWidget(m_view, 1);

    m_note = new QLabel(this);
    m_note->setObjectName(QStringLiteral("exportPreviewNote"));
    m_note->setWordWrap(true);
    m_note->setStyleSheet(QStringLiteral("color: palette(mid);"));
    layout->addWidget(m_note);

    connect(m_combo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        if (m_updating)
        {
            return;
        }
        const QString id = m_combo->currentData().toString();
        m_fieldId = id.isEmpty() ? core::ObjectId() : core::ObjectId::fromString(id.toStdString()).value_or(core::ObjectId());
        updateText();
    });
}

void ExportPreviewPanel::setDocument(CaseDocumentController* document)
{
    m_document = document;
    m_fieldId = core::ObjectId();
    refresh();
}

void ExportPreviewPanel::refresh()
{
    if (!m_document)
    {
        m_combo->clear();
        m_view->clear();
        m_note->clear();
        return;
    }
    rebuildCombo();
    updateText();
}

void ExportPreviewPanel::selectField(const core::ObjectId& fieldId)
{
    m_fieldId = fieldId;
    for (int index = 0; index < m_combo->count(); ++index)
    {
        if (m_combo->itemData(index).toString() == QString::fromStdString(fieldId.toString()))
        {
            m_combo->setCurrentIndex(index);
            break;
        }
    }
    updateText();
}

QStringList ExportPreviewPanel::availableFiles() const
{
    QStringList files;
    for (int index = 0; index < m_combo->count(); ++index)
    {
        const QString text = m_combo->itemText(index);
        if (!text.isEmpty())
        {
            files.append(text);
        }
    }
    return files;
}

QString ExportPreviewPanel::currentText() const
{
    return m_view->toPlainText();
}

QComboBox* ExportPreviewPanel::fileCombo() const noexcept
{
    return m_combo;
}

QPlainTextEdit* ExportPreviewPanel::view() const noexcept
{
    return m_view;
}

void ExportPreviewPanel::rebuildCombo()
{
    m_updating = true;
    const QString previous =
        m_fieldId.isValid() ? QString::fromStdString(m_fieldId.toString()) : QString();
    m_combo->clear();

    const core::CaseProject& project = m_document->project();
    int restoreIndex = -1;
    for (const core::FieldDefinition& field : project.fields())
    {
        const QString id = QString::fromStdString(field.id().toString());
        m_combo->addItem(QStringLiteral("0/") + QString::fromStdString(field.name()), id);
        if (!previous.isEmpty() && id == previous)
        {
            restoreIndex = m_combo->count() - 1;
        }
    }

    if (m_combo->count() == 0)
    {
        m_combo->addItem(tr("（无场文件）"), QString());
        m_combo->setEnabled(false);
        m_fieldId = core::ObjectId();
    }
    else
    {
        m_combo->setEnabled(true);
        if (restoreIndex < 0)
        {
            restoreIndex = 0;
        }
        m_combo->setCurrentIndex(restoreIndex);
        m_fieldId = core::ObjectId::fromString(
            m_combo->itemData(restoreIndex).toString().toStdString()).value_or(core::ObjectId());
    }
    m_updating = false;
}

void ExportPreviewPanel::updateText()
{
    if (!m_document || !m_fieldId.isValid())
    {
        m_view->setPlainText(tr("项目还没有场文件。导入算例或在场编辑器中新增 0/p、0/U 后，这里显示将写出的内容。"));
        m_note->setText(tr("预览只显示 0/ 场文件；system/、constant/ 的完整导出预览属于后续导出任务。"));
        return;
    }

    const core::CaseProject& project = m_document->project();
    const core::FieldDefinition* field = project.findField(m_fieldId);
    if (!field)
    {
        m_view->clear();
        m_note->clear();
        return;
    }

    const std::string text = io::FieldFileWriter::write(*field, project);
    m_view->setPlainText(QString::fromStdString(text));

    const core::ValidationResult validation = core::CaseValidator().validate(project);
    m_note->setText(tr("与导出器共用 FieldFileWriter，内容与写出结果一致；当前校验：阻断 %1 项、警告 %2 项。")
                        .arg(validation.count(core::Severity::BlockingError))
                        .arg(validation.count(core::Severity::Warning)));
}

} // namespace ofgui
