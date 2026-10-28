// Group construction concern of FieldEditor: dimensions, internalField,
// boundaryField and preview boxes. Kept in its own translation unit because the
// editor file is over the 400-line limit (D-021).
#include "ui/FieldEditor.h"

#include "ui/FieldBoundaryTable.h"
#include "ui/UniformValueEditor.h"

#include <QFontDatabase>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QVBoxLayout>

namespace ofgui {
namespace {

const char* const kDimensionUnits[7] = {"kg", "m", "s", "K", "mol", "A", "cd"};

} // namespace

void FieldEditor::buildDimensionsGroup(QWidget* parent, QVBoxLayout* layout)
{
    auto* group = new QGroupBox(tr("dimensions（只读）"), parent);
    group->setObjectName(QStringLiteral("dimensionsGroup"));
    auto* grid = new QGridLayout(group);
    grid->setHorizontalSpacing(6);
    grid->setVerticalSpacing(4);

    for (int index = 0; index < 7; ++index)
    {
        auto* unit = new QLabel(QString::fromLatin1(kDimensionUnits[index]), group);
        unit->setAlignment(Qt::AlignHCenter);
        grid->addWidget(unit, 0, index);
    }

    for (int index = 0; index < 7; ++index)
    {
        auto* spin = new QSpinBox(group);
        spin->setObjectName(QStringLiteral("dimensionSpin%1").arg(index));
        spin->setRange(-8, 8);
        spin->setEnabled(false);
        spin->setButtonSymbols(QAbstractSpinBox::NoButtons);
        spin->setAlignment(Qt::AlignCenter);
        spin->setMaximumWidth(40);
        spin->setToolTip(tr("首版 dimensions 只读，保存时沿用导入值。"));
        grid->addWidget(spin, 1, index);
        m_dimensions[index] = spin;
    }

    auto* note = new QLabel(tr("首版只读：修改量纲会改变物理含义，需单独任务支持。"), group);
    note->setWordWrap(true);
    note->setStyleSheet(QStringLiteral("color: palette(mid);"));
    grid->addWidget(note, 2, 0, 1, 7);

    layout->addWidget(group);
}

void FieldEditor::buildInternalFieldGroup(QWidget* parent, QVBoxLayout* layout)
{
    auto* group = new QGroupBox(tr("internalField"), parent);
    group->setObjectName(QStringLiteral("internalFieldGroup"));
    auto* box = new QVBoxLayout(group);

    m_internalField = new UniformValueEditor(group);
    m_internalField->setObjectName(QStringLiteral("internalFieldEditor"));
    connect(m_internalField, &UniformValueEditor::editingFinished, this,
            &FieldEditor::commitInternalField);
    box->addWidget(m_internalField);

    m_internalNote = new QLabel(tr("导入的 internalField 不是 uniform 值（或为空），以原文只读保留。"), group);
    m_internalNote->setObjectName(QStringLiteral("internalFieldNote"));
    m_internalNote->setWordWrap(true);
    m_internalNote->setStyleSheet(QStringLiteral("color: palette(mid);"));
    m_internalNote->hide();
    box->addWidget(m_internalNote);

    layout->addWidget(group);
}

void FieldEditor::buildBoundaryGroup(QWidget* parent, QVBoxLayout* layout)
{
    auto* group = new QGroupBox(tr("boundaryField（按 patch）"), parent);
    group->setObjectName(QStringLiteral("boundaryGroup"));
    auto* box = new QVBoxLayout(group);
    box->setSpacing(4);

    m_boundaryTable = new FieldBoundaryTable(group);
    connect(m_boundaryTable, &FieldBoundaryTable::boundaryEdited, this,
            [this](core::ObjectId) { updatePreview(); });
    box->addWidget(m_boundaryTable);

    auto* note = new QLabel(
        tr("类型只列出常用值；导入的其他类型（例如 inletOutlet）保持原文只读。"), group);
    note->setWordWrap(true);
    note->setStyleSheet(QStringLiteral("color: palette(mid);"));
    box->addWidget(note);

    layout->addWidget(group);
}

void FieldEditor::buildPreviewGroup(QWidget* parent, QVBoxLayout* layout)
{
    auto* group = new QGroupBox(tr("0/ 预览"), parent);
    group->setObjectName(QStringLiteral("previewGroup"));
    auto* box = new QVBoxLayout(group);

    m_preview = new QPlainTextEdit(group);
    m_preview->setObjectName(QStringLiteral("fieldPreview"));
    m_preview->setReadOnly(true);
    m_preview->setMinimumHeight(180);
    QFont fixed = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    fixed.setPointSize(9);
    m_preview->setFont(fixed);
    box->addWidget(m_preview);

    auto* note = new QLabel(tr("预览为当前场将写出的 0/<field> 文本（LF 换行）。"), group);
    note->setWordWrap(true);
    note->setStyleSheet(QStringLiteral("color: palette(mid);"));
    box->addWidget(note);

    layout->addWidget(group);
}

} // namespace ofgui
