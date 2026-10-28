#include "ui/FieldEditor.h"

#include "core/CaseProject.h"
#include "core/FieldDefinition.h"
#include "core/FieldValue.h"
#include "io/writers/FieldFileWriter.h"
#include "ui/CaseDocumentController.h"
#include "ui/FieldBoundaryTable.h"
#include "ui/UniformValueEditor.h"
#include "ui/UniformValueText.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QToolButton>
#include <QVBoxLayout>

namespace ofgui {

namespace {

QString internalFieldText(const core::FieldDefinition& field)
{
    const core::FieldValue& value = field.internalField();
    if (value.isUniformScalar())
    {
        return QStringLiteral("uniform ") + ui::formatReal(value.scalar());
    }
    if (value.isUniformVector())
    {
        const core::Vec3d& vector = value.vector();
        return QStringLiteral("uniform (") + ui::formatReal(vector.x) + QLatin1Char(' ')
               + ui::formatReal(vector.y) + QLatin1Char(' ') + ui::formatReal(vector.z)
               + QLatin1Char(')');
    }
    if (value.isNonUniformRaw())
    {
        return QString::fromStdString(value.rawText());
    }
    return QString();
}

} // namespace

FieldEditor::FieldEditor(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("fieldEditor"));

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    auto* scroll = new QScrollArea(this);
    scroll->setObjectName(QStringLiteral("fieldEditorScroll"));
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(buildContent());
    outer->addWidget(scroll);

    setEnabledState(false);
}

QWidget* FieldEditor::buildContent()
{
    auto* content = new QWidget(this);
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 6, 0);
    layout->setSpacing(8);

    auto* header = new QWidget(content);
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    m_class = new QLabel(tr("未选择场"), header);
    m_class->setObjectName(QStringLiteral("fieldClassLabel"));
    QFont classFont = m_class->font();
    classFont.setBold(true);
    m_class->setFont(classFont);
    headerLayout->addWidget(m_class, 1);

    m_addButton = new QToolButton(header);
    m_addButton->setObjectName(QStringLiteral("addFieldButton"));
    m_addButton->setText(tr("新增场"));
    m_addButton->setPopupMode(QToolButton::InstantPopup);
    auto* addMenu = new QMenu(m_addButton);
    QAction* addP = addMenu->addAction(tr("新增 p（volScalarField）"));
    connect(addP, &QAction::triggered, this, [this]() { (void)addPredefinedField(QStringLiteral("p")); });
    QAction* addU = addMenu->addAction(tr("新增 U（volVectorField）"));
    connect(addU, &QAction::triggered, this, [this]() { (void)addPredefinedField(QStringLiteral("U")); });
    m_addButton->setMenu(addMenu);
    headerLayout->addWidget(m_addButton);

    m_removeButton = new QPushButton(tr("删除场"), header);
    m_removeButton->setObjectName(QStringLiteral("removeFieldButton"));
    connect(m_removeButton, &QPushButton::clicked, this, [this]() { (void)removeCurrentField(); });
    headerLayout->addWidget(m_removeButton);
    layout->addWidget(header);

    buildDimensionsGroup(content, layout);
    buildInternalFieldGroup(content, layout);
    buildBoundaryGroup(content, layout);
    buildPreviewGroup(content, layout);

    m_error = new QLabel(content);
    m_error->setObjectName(QStringLiteral("fieldEditorError"));
    m_error->setWordWrap(true);
    m_error->setStyleSheet(QStringLiteral("color: #b00020;"));
    m_error->hide();
    layout->addWidget(m_error);
    layout->addStretch(1);

    return content;
}

void FieldEditor::setDocument(CaseDocumentController* document)
{
    m_document = document;
    m_boundaryTable->setDocument(document);
    m_addButton->setEnabled(document != nullptr);
    if (!document || !m_fieldId.isValid())
    {
        clear();
        return;
    }
    refresh();
}

void FieldEditor::showField(const core::ObjectId& fieldId)
{
    m_fieldId = fieldId;
    refresh();
}

void FieldEditor::showBoundary(const core::ObjectId& fieldId, const core::ObjectId& patchId)
{
    m_fieldId = fieldId;
    refresh();
    m_boundaryTable->selectPatch(patchId);
}

core::ObjectId FieldEditor::fieldId() const noexcept
{
    return m_fieldId;
}

FieldBoundaryTable* FieldEditor::boundaryTable() const noexcept
{
    return m_boundaryTable;
}

UniformValueEditor* FieldEditor::internalFieldEditor() const noexcept
{
    return m_internalField;
}

QPlainTextEdit* FieldEditor::previewView() const noexcept
{
    return m_preview;
}

QString FieldEditor::errorText() const
{
    return m_error->text();
}

QString FieldEditor::previewText() const
{
    return m_preview->toPlainText();
}

QToolButton* FieldEditor::addFieldButton() const noexcept
{
    return m_addButton;
}

QPushButton* FieldEditor::removeFieldButton() const noexcept
{
    return m_removeButton;
}

void FieldEditor::clear()
{
    m_updating = true;
    m_fieldId = core::ObjectId();
    m_class->setText(tr("未选择场"));
    for (QSpinBox* spin : m_dimensions)
    {
        spin->setValue(0);
    }
    m_internalField->setFieldClass(core::FieldClass::VolScalarField);
    m_internalField->setRawText(QString());
    m_internalField->setEditable(false);
    m_internalNote->hide();
    m_boundaryTable->setField(core::ObjectId());
    m_preview->clear();
    setEnabledState(false);
    clearError();
    m_updating = false;
}

void FieldEditor::refresh()
{
    if (m_updating)
    {
        return;
    }
    if (!m_document || !m_fieldId.isValid())
    {
        clear();
        return;
    }

    const core::CaseProject& project = m_document->project();
    const core::FieldDefinition* field = project.findField(m_fieldId);
    if (!field)
    {
        clear();
        return;
    }

    m_updating = true;
    m_class->setText(field->fieldClass() == core::FieldClass::VolScalarField
                         ? tr("volScalarField")
                         : tr("volVectorField"));
    const core::FieldDefinition::Dimensions& dimensions = field->dimensions();
    for (int index = 0; index < 7; ++index)
    {
        m_dimensions[index]->setValue(dimensions[static_cast<std::size_t>(index)]);
    }

    const core::FieldValue& value = field->internalField();
    const bool rawValue = value.isNonUniformRaw() || !value.isValid();
    m_internalField->setFieldClass(field->fieldClass());
    m_internalField->setRawText(internalFieldText(*field));
    m_internalField->setEditable(!rawValue);
    m_internalNote->setVisible(rawValue);

    m_boundaryTable->setField(m_fieldId);
    setEnabledState(true);
    m_removeButton->setEnabled(
        field->name() == "p" || field->name() == "U");
    updatePreview();
    m_updating = false;
}

void FieldEditor::commitInternalField()
{
    if (m_updating || !m_document || !m_fieldId.isValid())
    {
        return;
    }
    core::FieldDefinition* field = m_document->project().findField(m_fieldId);
    if (!field || !m_internalField->isEditable())
    {
        return;
    }
    if (m_internalField->rawText() == internalFieldText(*field))
    {
        return;
    }

    core::FieldValue value;
    if (field->fieldClass() == core::FieldClass::VolScalarField)
    {
        const std::optional<core::Real> scalar = m_internalField->scalarValue();
        if (!scalar)
        {
            setError(tr("internalField 需要标量，例如“uniform 0”。"));
            return;
        }
        value = core::FieldValue::uniformScalar(*scalar);
    }
    else
    {
        const std::optional<core::Vec3d> vector = m_internalField->vectorValue();
        if (!vector)
        {
            setError(tr("internalField 需要向量，例如“uniform (0 0 0)”。"));
            return;
        }
        value = core::FieldValue::uniformVector(*vector);
    }

    field->setInternalField(value);
    clearError();
    m_document->notifyProjectEdited();
    updatePreview();
}

void FieldEditor::updatePreview()
{
    if (!m_document || !m_fieldId.isValid())
    {
        m_preview->clear();
        return;
    }
    const core::CaseProject& project = m_document->project();
    const core::FieldDefinition* field = project.findField(m_fieldId);
    if (!field)
    {
        m_preview->clear();
        return;
    }
    const std::string text = io::FieldFileWriter::write(*field, project);
    m_preview->setPlainText(QString::fromStdString(text));
}

void FieldEditor::setEnabledState(bool enabled)
{
    m_boundaryTable->setEnabled(enabled);
    m_internalField->setEnabled(enabled);
    m_removeButton->setEnabled(enabled);
    m_addButton->setEnabled(m_document != nullptr);
}

void FieldEditor::setError(const QString& message)
{
    m_error->setText(message);
    m_error->setVisible(!message.isEmpty());
}

void FieldEditor::clearError()
{
    m_error->clear();
    m_error->hide();
}

} // namespace ofgui
