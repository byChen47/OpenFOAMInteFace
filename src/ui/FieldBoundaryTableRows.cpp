// Row building concern of FieldBoundaryTable: rebuilding one row per patch and
// committing edits back into the model. Kept in its own translation unit because
// the table file is over the 400-line limit (D-021).
#include "ui/FieldBoundaryTable.h"

#include "core/BoundaryCondition.h"
#include "core/BoundaryPatch.h"
#include "core/CaseProject.h"
#include "ui/CaseDocumentController.h"

#include <QComboBox>
#include <QLineEdit>
#include <QTableWidget>
#include <QTableWidgetItem>

#include <string>
#include <vector>

namespace ofgui {
namespace {

QString typeOfCondition(const core::BoundaryCondition& condition)
{
    return QString::fromStdString(condition.type());
}

} // namespace

void FieldBoundaryTable::rebuild()
{
    if (m_rebuilding || m_applying)
    {
        return;
    }
    m_rebuilding = true;
    const core::ObjectId previous = currentPatchId();

    m_table->setRowCount(0);
    m_rows.clear();
    clearError();

    const core::CaseProject* project = m_document ? &m_document->project() : nullptr;
    const core::FieldDefinition* field = project ? project->findField(m_fieldId) : nullptr;
    if (!project || !field)
    {
        m_rebuilding = false;
        return;
    }

    const std::vector<core::BoundaryPatch>& patches = project->patches();
    m_table->setRowCount(static_cast<int>(patches.size()));
    m_rows.reserve(patches.size());

    const QStringList types = offeredTypes();
    for (std::size_t index = 0; index < patches.size(); ++index)
    {
        const core::BoundaryPatch& patch = patches[index];
        const int row = static_cast<int>(index);
        Row entry;
        entry.patchId = patch.id();

        entry.nameItem = new QTableWidgetItem(QString::fromStdString(patch.name()));
        entry.nameItem->setToolTip(
            tr("patch 类型：%1").arg(QString::fromStdString(std::string(core::patchTypeName(patch.type())))));
        m_table->setItem(row, 0, entry.nameItem);

        entry.type = new QComboBox(m_table);
        entry.type->setObjectName(QStringLiteral("bcTypeCombo%1").arg(row));
        entry.type->setProperty("patchId", QString::fromStdString(patch.id().toString()));

        entry.value = new QLineEdit(m_table);
        entry.value->setObjectName(QStringLiteral("bcValueEdit%1").arg(row));

        const core::BoundaryCondition* condition = field->findBoundaryCondition(patch.id());
        const bool hasCondition = condition != nullptr;
        bool importedReadOnly = false;
        if (hasCondition)
        {
            const QString type = typeOfCondition(*condition);
            importedReadOnly = !types.contains(type);
            if (importedReadOnly)
            {
                entry.type->addItem(type, type);
                entry.type->setToolTip(
                    tr("导入的边界条件类型不在首版可编辑清单内，只读保存（D-024）。"));
                const auto parameters = condition->parameters();
                const auto value = parameters.find("value");
                if (value != parameters.end())
                {
                    entry.value->setText(QString::fromStdString(value->second));
                }
            }
            else
            {
                for (const QString& type : types)
                {
                    entry.type->addItem(type, type);
                }
                entry.type->setCurrentText(type);
                const auto parameters = condition->parameters();
                const auto value = parameters.find("value");
                if (value != parameters.end())
                {
                    entry.value->setText(QString::fromStdString(value->second));
                }
            }
        }
        else
        {
            entry.type->addItem(tr("未设置"), QString());
            for (const QString& type : types)
            {
                entry.type->addItem(type, type);
            }
        }

        entry.type->setEnabled(!importedReadOnly);
        m_table->setCellWidget(row, 1, entry.type);
        m_table->setCellWidget(row, 2, entry.value);

        const QString type = entry.type->currentData().toString();
        updateValueEditor(entry, type, hasCondition && importedReadOnly);

        connect(entry.type, qOverload<int>(&QComboBox::currentIndexChanged), this,
                [this, row](int) {
                    if (!m_rebuilding)
                    {
                        applyRow(row);
                    }
                });
        connect(entry.value, &QLineEdit::editingFinished, this,
                [this, row]() {
                    if (!m_rebuilding)
                    {
                        applyRow(row);
                    }
                });

        m_rows.push_back(entry);
    }

    if (previous.isValid())
    {
        for (std::size_t row = 0; row < m_rows.size(); ++row)
        {
            if (m_rows[row].patchId == previous)
            {
                m_table->selectRow(static_cast<int>(row));
                break;
            }
        }
    }
    m_rebuilding = false;
}

void FieldBoundaryTable::applyRow(int row)
{
    if (m_applying || m_rebuilding || !m_document)
    {
        return;
    }
    if (row < 0 || row >= static_cast<int>(m_rows.size()))
    {
        return;
    }

    core::CaseProject& project = m_document->project();
    core::FieldDefinition* field = project.findField(m_fieldId);
    if (!field)
    {
        return;
    }

    Row& entry = m_rows[static_cast<std::size_t>(row)];
    const QString type = entry.type->currentData().toString();
    if (type.isEmpty() || !entry.type->isEnabled())
    {
        return;
    }

    updateValueEditor(entry, type, field->hasBoundaryCondition(entry.patchId));

    QString error;
    if (!validateValue(field->fieldClass(), type, entry.value->text(), &error))
    {
        setError(error);
        entry.value->setFocus();
        return;
    }

    core::BoundaryCondition condition(type.toStdString());
    const QString valueText = entry.value->text().trimmed();
    if (typeNeedsValue(type) && !valueText.isEmpty())
    {
        condition.setParameter("value", valueText.toStdString());
    }
    if (const core::BoundaryCondition* existing = field->findBoundaryCondition(entry.patchId))
    {
        for (const auto& [name, value] : existing->parameters())
        {
            if (name != "value")
            {
                condition.setParameter(name, value);
            }
        }
    }

    field->setBoundaryCondition(entry.patchId, condition);
    clearError();

    m_applying = true;
    m_document->notifyProjectEdited();
    m_applying = false;

    emit boundaryEdited(entry.patchId);
}

} // namespace ofgui
