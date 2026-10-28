// Field add/remove concern of FieldEditor: the first version only creates and
// deletes `p` and `U` (D-024). Kept in its own translation unit because the
// editor file is over the 400-line limit (D-021).
#include "ui/FieldEditor.h"

#include "core/BoundaryCondition.h"
#include "core/BoundaryPatch.h"
#include "core/CaseProject.h"
#include "core/FieldDefinition.h"
#include "core/FieldValue.h"
#include "ui/CaseDocumentController.h"

#include <QString>

namespace ofgui {

bool FieldEditor::addPredefinedField(const QString& name)
{
    if (!m_document)
    {
        return false;
    }
    const QString trimmed = name.trimmed();
    if (trimmed != QStringLiteral("p") && trimmed != QStringLiteral("U"))
    {
        setError(tr("首版支持新增 p（压力）或 U（速度）场。"));
        return false;
    }

    core::CaseProject& project = m_document->project();
    if (project.findFieldByName(trimmed.toStdString()))
    {
        setError(tr("场 %1 已存在。").arg(trimmed));
        return false;
    }

    const bool scalarField = trimmed == QStringLiteral("p");
    const core::FieldClass fieldClass =
        scalarField ? core::FieldClass::VolScalarField : core::FieldClass::VolVectorField;
    const core::FieldDefinition::Dimensions dimensions = scalarField
                                            ? core::FieldDefinition::Dimensions{0, 2, -2, 0, 0, 0, 0}
                                            : core::FieldDefinition::Dimensions{0, 1, -1, 0, 0, 0, 0};
    const core::ObjectId fieldId = project.addField(trimmed.toStdString(), fieldClass, dimensions);

    core::FieldDefinition* field = project.findField(fieldId);
    if (field)
    {
        if (scalarField)
        {
            field->setInternalField(core::FieldValue::uniformScalar(0.0));
        }
        else
        {
            field->setInternalField(core::FieldValue::uniformVector(core::Vec3d::zero()));
        }
        for (const core::BoundaryPatch& patch : project.patches())
        {
            field->setBoundaryCondition(patch.id(), core::BoundaryCondition("zeroGradient"));
        }
    }

    clearError();
    m_document->notifyProjectEdited();
    showField(fieldId);
    emit fieldAdded(fieldId);
    return true;
}

bool FieldEditor::removeCurrentField()
{
    if (!m_document || !m_fieldId.isValid())
    {
        return false;
    }
    core::CaseProject& project = m_document->project();
    const core::FieldDefinition* field = project.findField(m_fieldId);
    if (!field)
    {
        setError(tr("场已不存在。"));
        return false;
    }
    const QString name = QString::fromStdString(field->name());
    if (name != QStringLiteral("p") && name != QStringLiteral("U"))
    {
        setError(tr("首版只允许删除 p 或 U 场。"));
        return false;
    }

    const core::ObjectId removedId = m_fieldId;
    project.removeField(removedId);
    clear();
    m_document->notifyProjectEdited();
    emit fieldRemoved();
    return true;
}

} // namespace ofgui
