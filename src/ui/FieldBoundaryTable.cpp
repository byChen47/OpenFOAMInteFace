#include "ui/FieldBoundaryTable.h"

#include "core/CaseProject.h"
#include "ui/CaseDocumentController.h"

#include <QComboBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QTableWidget>
#include <QVBoxLayout>

namespace ofgui {

FieldBoundaryTable::FieldBoundaryTable(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    m_table = new QTableWidget(this);
    m_table->setObjectName(QStringLiteral("boundaryTable"));
    m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels({tr("patch"), tr("类型"), tr("值 / 参数")});
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);
    m_table->setColumnWidth(1, 116);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setMinimumHeight(96);
    layout->addWidget(m_table);

    m_error = new QLabel(this);
    m_error->setObjectName(QStringLiteral("boundaryTableError"));
    m_error->setWordWrap(true);
    m_error->setStyleSheet(QStringLiteral("color: #b00020;"));
    m_error->hide();
    layout->addWidget(m_error);

    connect(m_table, &QTableWidget::cellClicked, this,
            [this](int row, int) {
                if (row >= 0 && row < static_cast<int>(m_rows.size()))
                {
                    emit patchSelected(m_rows[static_cast<std::size_t>(row)].patchId);
                }
            });
}

void FieldBoundaryTable::setDocument(CaseDocumentController* document)
{
    m_document = document;
    rebuild();
}

void FieldBoundaryTable::setField(const core::ObjectId& fieldId)
{
    m_fieldId = fieldId;
    rebuild();
}

core::ObjectId FieldBoundaryTable::fieldId() const noexcept
{
    return m_fieldId;
}

void FieldBoundaryTable::refresh()
{
    rebuild();
}

void FieldBoundaryTable::selectPatch(const core::ObjectId& patchId)
{
    for (std::size_t row = 0; row < m_rows.size(); ++row)
    {
        if (m_rows[row].patchId == patchId)
        {
            m_table->selectRow(static_cast<int>(row));
            emit patchSelected(patchId);
            return;
        }
    }
}

core::ObjectId FieldBoundaryTable::currentPatchId() const
{
    const int row = m_table->currentRow();
    if (row < 0 || row >= static_cast<int>(m_rows.size()))
    {
        return core::ObjectId();
    }
    return m_rows[static_cast<std::size_t>(row)].patchId;
}

QTableWidget* FieldBoundaryTable::table() const noexcept
{
    return m_table;
}

QComboBox* FieldBoundaryTable::typeCombo(int row) const
{
    if (row < 0 || row >= static_cast<int>(m_rows.size()))
    {
        return nullptr;
    }
    return m_rows[static_cast<std::size_t>(row)].type;
}

QLineEdit* FieldBoundaryTable::valueEdit(int row) const
{
    if (row < 0 || row >= static_cast<int>(m_rows.size()))
    {
        return nullptr;
    }
    return m_rows[static_cast<std::size_t>(row)].value;
}

QString FieldBoundaryTable::errorText() const
{
    return m_error->text();
}

void FieldBoundaryTable::setError(const QString& message)
{
    m_error->setText(message);
    m_error->setVisible(!message.isEmpty());
    emit errorTextChanged(message);
}

void FieldBoundaryTable::clearError()
{
    const bool wasVisible = m_error->isVisible();
    m_error->clear();
    m_error->hide();
    if (wasVisible)
    {
        emit errorTextChanged(QString());
    }
}

} // namespace ofgui
