// Boundary-condition type rules of FieldBoundaryTable: which BC types are offered
// per field class, which ones require a value, and the uniform-value validation
// used before writing the model. Kept in its own translation unit because the
// table file is over the 400-line limit (D-021).
#include "ui/FieldBoundaryTable.h"

#include "core/BoundaryCondition.h"
#include "core/BoundaryPatch.h"
#include "core/CaseProject.h"
#include "ui/CaseDocumentController.h"
#include "ui/UniformValueText.h"

#include <QComboBox>
#include <QLineEdit>

namespace ofgui {
namespace {

const QStringList& scalarTypeNames()
{
    static const QStringList names = {
        QStringLiteral("zeroGradient"),
        QStringLiteral("fixedValue"),
        QStringLiteral("fixedFluxPressure"),
        QStringLiteral("calculated"),
        QStringLiteral("empty"),
        QStringLiteral("symmetry"),
        QStringLiteral("symmetryPlane"),
        QStringLiteral("slip"),
    };
    return names;
}

const QStringList& vectorTypeNames()
{
    static const QStringList names = {
        QStringLiteral("zeroGradient"),
        QStringLiteral("fixedValue"),
        QStringLiteral("fixedFluxPressure"),
        QStringLiteral("calculated"),
        QStringLiteral("noSlip"),
        QStringLiteral("empty"),
        QStringLiteral("symmetry"),
        QStringLiteral("symmetryPlane"),
        QStringLiteral("slip"),
    };
    return names;
}

} // namespace

QStringList FieldBoundaryTable::offeredTypes() const
{
    const core::CaseProject* project = m_document ? &m_document->project() : nullptr;
    const core::FieldDefinition* field = project ? project->findField(m_fieldId) : nullptr;
    if (field && field->fieldClass() == core::FieldClass::VolVectorField)
    {
        return vectorTypeNames();
    }
    return scalarTypeNames();
}

bool FieldBoundaryTable::typeNeedsValue(const QString& type) const
{
    return type == QStringLiteral("fixedValue") || type == QStringLiteral("fixedFluxPressure")
           || type == QStringLiteral("calculated");
}

bool FieldBoundaryTable::validateValue(core::FieldClass fieldClass,
                                       const QString& type,
                                       const QString& text,
                                       QString* error) const
{
    const QString trimmed = text.trimmed();
    const bool required = type == QStringLiteral("fixedValue");
    if (trimmed.isEmpty())
    {
        if (required)
        {
            *error = tr("%1 需要值，例如“uniform 0”或“$internalField”。").arg(type);
            return false;
        }
        return true;
    }
    if (trimmed.startsWith(QLatin1Char('$')))
    {
        return true;
    }
    if (fieldClass == core::FieldClass::VolScalarField)
    {
        core::Real scalar = 0.0;
        if (!ui::parseUniformScalar(trimmed, &scalar))
        {
            *error = tr("%1 的值必须是 uniform 标量，例如“uniform 0”。").arg(type);
            return false;
        }
    }
    else
    {
        core::Vec3d vector;
        if (!ui::parseUniformVector(trimmed, &vector))
        {
            *error = tr("%1 的值必须是 uniform 向量，例如“uniform (0 0 0)”。").arg(type);
            return false;
        }
    }
    return true;
}

void FieldBoundaryTable::updateValueEditor(Row& row, const QString& type, bool hasCondition)
{
    const bool needsValue = typeNeedsValue(type);
    const bool importedReadOnly = hasCondition && !row.type->isEnabled();
    row.value->setEnabled(needsValue && !importedReadOnly);
    if (!needsValue)
    {
        row.value->setPlaceholderText(tr("无 value"));
    }
    else if (type == QStringLiteral("fixedValue"))
    {
        row.value->setPlaceholderText(tr("必填：uniform 0 或 uniform (0 0 0)"));
    }
    else
    {
        row.value->setPlaceholderText(tr("可选：uniform 0"));
    }
}

} // namespace ofgui
