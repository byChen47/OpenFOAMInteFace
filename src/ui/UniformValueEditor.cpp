#include "ui/UniformValueEditor.h"

#include "ui/UniformValueText.h"

#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace ofgui {

namespace {

constexpr int kDecimals = 8;
constexpr double kSpinLimit = 1.0e12;

void configureSpin(QDoubleSpinBox* spin)
{
    spin->setDecimals(kDecimals);
    spin->setRange(-kSpinLimit, kSpinLimit);
    spin->setSingleStep(0.1);
    spin->setKeyboardTracking(false);
    spin->setMinimumWidth(96);
}

} // namespace

UniformValueEditor::UniformValueEditor(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    m_stack = new QStackedWidget(this);

    auto* scalarPage = new QWidget(m_stack);
    auto* scalarLayout = new QHBoxLayout(scalarPage);
    scalarLayout->setContentsMargins(0, 0, 0, 0);
    m_scalar = new QDoubleSpinBox(scalarPage);
    m_scalar->setObjectName(QStringLiteral("uniformScalarEdit"));
    configureSpin(m_scalar);
    scalarLayout->addWidget(m_scalar, 1);
    m_stack->addWidget(scalarPage);

    auto* vectorPage = new QWidget(m_stack);
    auto* vectorLayout = new QGridLayout(vectorPage);
    vectorLayout->setContentsMargins(0, 0, 0, 0);
    vectorLayout->setHorizontalSpacing(6);
    vectorLayout->setVerticalSpacing(2);
    for (int index = 0; index < 3; ++index)
    {
        auto* axis = new QLabel(QStringLiteral("xyz").mid(index, 1), vectorPage);
        axis->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        vectorLayout->addWidget(axis, index, 0);
        m_vector[index] = new QDoubleSpinBox(vectorPage);
        m_vector[index]->setObjectName(QStringLiteral("uniformVectorEdit%1").arg(index));
        configureSpin(m_vector[index]);
        vectorLayout->addWidget(m_vector[index], index, 1);
    }
    m_stack->addWidget(vectorPage);

    m_raw = new QLineEdit(m_stack);
    m_raw->setObjectName(QStringLiteral("uniformRawValue"));
    m_raw->setReadOnly(true);
    m_raw->setToolTip(tr("该值不是 uniform 标量/向量（例如 nonuniform 列表或 $ 引用），首版只读保存。"));
    m_stack->addWidget(m_raw);

    layout->addWidget(m_stack);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setObjectName(QStringLiteral("uniformValueError"));
    m_errorLabel->setWordWrap(true);
    m_errorLabel->setStyleSheet(QStringLiteral("color: #b00020;"));
    m_errorLabel->hide();
    layout->addWidget(m_errorLabel);

    connect(m_scalar, &QDoubleSpinBox::editingFinished, this, &UniformValueEditor::notifyEdited);
    for (QDoubleSpinBox* spin : m_vector)
    {
        connect(spin, &QDoubleSpinBox::editingFinished, this, &UniformValueEditor::notifyEdited);
    }
}

void UniformValueEditor::setFieldClass(core::FieldClass fieldClass)
{
    m_fieldClass = fieldClass;
    if (m_mode != Mode::Raw)
    {
        m_mode = fieldClass == core::FieldClass::VolVectorField ? Mode::Vector : Mode::Scalar;
        m_stack->setCurrentIndex(m_mode == Mode::Vector ? 1 : 0);
    }
}

core::FieldClass UniformValueEditor::fieldClass() const noexcept
{
    return m_fieldClass;
}

void UniformValueEditor::setRawText(const QString& text)
{
    m_updating = true;
    clearError();
    const QString cleaned = ui::stripStatementSemicolon(text);

    if (m_fieldClass == core::FieldClass::VolScalarField)
    {
        core::Real scalar = 0.0;
        if (ui::parseUniformScalar(cleaned, &scalar))
        {
            m_mode = Mode::Scalar;
            m_scalar->setValue(scalar);
            m_scalar->setEnabled(m_editable);
            m_stack->setCurrentWidget(m_scalar->parentWidget());
            m_updating = false;
            return;
        }
    }
    else
    {
        core::Vec3d vector;
        if (ui::parseUniformVector(cleaned, &vector))
        {
            m_mode = Mode::Vector;
            m_vector[0]->setValue(vector.x);
            m_vector[1]->setValue(vector.y);
            m_vector[2]->setValue(vector.z);
            for (QDoubleSpinBox* spin : m_vector)
            {
                spin->setEnabled(m_editable);
            }
            m_stack->setCurrentWidget(m_vector[0]->parentWidget());
            m_updating = false;
            return;
        }
    }

    showRaw(cleaned);
    m_updating = false;
}

QString UniformValueEditor::rawText() const
{
    switch (m_mode)
    {
        case Mode::Scalar:
            return QStringLiteral("uniform ") + ui::formatReal(m_scalar->value());
        case Mode::Vector:
            return QStringLiteral("uniform (") + ui::formatReal(m_vector[0]->value()) + QLatin1Char(' ')
                   + ui::formatReal(m_vector[1]->value()) + QLatin1Char(' ')
                   + ui::formatReal(m_vector[2]->value()) + QLatin1Char(')');
        case Mode::Raw:
            break;
    }
    return m_raw->text();
}

bool UniformValueEditor::isEditable() const noexcept
{
    return m_editable && m_mode != Mode::Raw;
}

bool UniformValueEditor::isValid() const noexcept
{
    return m_mode != Mode::Raw && m_error.isEmpty();
}

QString UniformValueEditor::errorText() const
{
    return m_error;
}

void UniformValueEditor::clearError()
{
    m_error.clear();
    m_errorLabel->clear();
    m_errorLabel->hide();
}

std::optional<core::Real> UniformValueEditor::scalarValue() const
{
    if (m_mode != Mode::Scalar)
    {
        return std::nullopt;
    }
    return m_scalar->value();
}

std::optional<core::Vec3d> UniformValueEditor::vectorValue() const
{
    if (m_mode != Mode::Vector)
    {
        return std::nullopt;
    }
    return core::Vec3d(m_vector[0]->value(), m_vector[1]->value(), m_vector[2]->value());
}

void UniformValueEditor::setEditable(bool editable)
{
    m_editable = editable;
    m_scalar->setEnabled(editable && m_mode == Mode::Scalar);
    for (QDoubleSpinBox* spin : m_vector)
    {
        spin->setEnabled(editable && m_mode == Mode::Vector);
    }
    m_raw->setReadOnly(true);
}

void UniformValueEditor::showRaw(const QString& text)
{
    m_mode = Mode::Raw;
    m_raw->setText(text);
    m_stack->setCurrentWidget(m_raw);
    if (!text.isEmpty())
    {
        m_raw->setToolTip(tr("非 uniform 值，首版只读以避免改写导入的字典内容：%1").arg(text));
    }
}

void UniformValueEditor::notifyEdited()
{
    if (m_updating)
    {
        return;
    }
    clearError();
    emit editingFinished();
}

} // namespace ofgui
