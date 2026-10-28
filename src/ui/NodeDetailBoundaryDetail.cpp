#include "ui/NodeDetailSections.h"

#include "ui/CaseDocumentController.h"
#include "ui/NodeDetailHtml.h"

#include "core/BoundaryCondition.h"
#include "core/BoundaryPatch.h"
#include "core/CaseProject.h"
#include "core/FieldDefinition.h"

#include <string>

namespace ofgui::detail {
namespace {

QString dimensionsLine(const core::FieldDefinition& field)
{
    return NodeDetailFormatter::tr("%1 · dimensions %2 · 边界条件 %3 个")
        .arg(fromStd(std::string(core::fieldClassName(field.fieldClass()))),
             esc(dimensionsText(field.dimensions())))
        .arg(static_cast<qulonglong>(field.boundaryConditions().size()));
}

QString boundaryConditionText(const core::BoundaryCondition* condition)
{
    return condition != nullptr && !condition->isEmpty()
        ? fromStd(condition->type())
        : NodeDetailFormatter::tr("未设置");
}

const core::BoundaryPatch* findPatchOwning(const core::CaseProject& project, core::FaceId face)
{
    for (const core::BoundaryPatch& patch : project.patches())
    {
        if (patch.hasFace(face))
        {
            return &patch;
        }
    }
    return nullptr;
}

} // namespace

NodeDetail describeBoundaryNode(const CaseDocumentController& document, const ProjectNodeRef& ref)
{
    const core::CaseProject& project = document.project();
    const core::ValidationResult& validation = document.validation();
    NodeDetail detail;

    switch (ref.kind)
    {
        case NodeKind::Boundaries:
        {
            detail.title = NodeDetailFormatter::tr("边界（六面）");
            detail.subtitle = NodeDetailFormatter::tr("%1 个 patch")
                                  .arg(static_cast<qulonglong>(project.patches().size()));

            QString rows;
            for (const core::BoundaryPatch& patch : project.patches())
            {
                rows += row(fromStd(patch.name()), esc(patchSummary(patch)));
            }
            for (const core::FaceId face : core::kAllFaceIds)
            {
                bool assigned = false;
                for (const core::BoundaryPatch& patch : project.patches())
                {
                    assigned = assigned || patch.hasFace(face);
                }
                if (!assigned)
                {
                    rows += row(QString::fromLatin1(core::faceName(face)), esc(NodeDetailFormatter::tr("未分配")));
                }
            }
            detail.html = table(rows);
            detail.html += issuesHtml(validation, project.patchIds(), QString());
            break;
        }
        case NodeKind::BoundaryFace:
        {
            const QString faceName = QString::fromLatin1(core::faceName(ref.face));
            detail.title = faceName;
            detail.subtitle = NodeDetailFormatter::tr("边界面");

            const core::BoundaryPatch* patch = findPatchOwning(project, ref.face);
            QString rows;
            rows += row(NodeDetailFormatter::tr("面"), esc(faceName));
            if (patch == nullptr)
            {
                rows += row(NodeDetailFormatter::tr("所属 patch"),
                            esc(NodeDetailFormatter::tr("未分配")));
            }
            else
            {
                rows += row(NodeDetailFormatter::tr("所属 patch"), esc(fromStd(patch->name())));
                rows += row(NodeDetailFormatter::tr("patch 类型"), esc(patchTypeText(*patch)));
                rows += row(NodeDetailFormatter::tr("该 patch 的面"), esc(patchSummary(*patch)));
            }
            detail.html = table(rows)
                          + issuesHtml(validation,
                                       patch != nullptr
                                           ? std::vector<core::ObjectId>{patch->id()}
                                           : std::vector<core::ObjectId>{},
                                       QString());

            if (patch != nullptr && !project.fields().empty())
            {
                QString bcRows;
                for (const core::FieldDefinition& field : project.fields())
                {
                    bcRows += row(fromStd(field.name()),
                                  esc(boundaryConditionText(field.findBoundaryCondition(patch->id()))));
                }
                detail.html += section(NodeDetailFormatter::tr("该 patch 上的边界条件"))
                               + table(bcRows);
            }
            break;
        }
        case NodeKind::Fields:
        {
            detail.title = NodeDetailFormatter::tr("场（0/）");
            detail.subtitle = NodeDetailFormatter::tr("%1 个场文件")
                                  .arg(static_cast<qulonglong>(project.fields().size()));

            QString rows;
            for (const core::FieldDefinition& field : project.fields())
            {
                rows += row(fromStd(field.name()), dimensionsLine(field));
            }
            detail.html = rows.isEmpty()
                ? paragraph(esc(NodeDetailFormatter::tr("工程中尚未识别到场文件。")))
                : table(rows);
            detail.html += issuesHtml(validation, {}, QStringLiteral("field"));
            break;
        }
        case NodeKind::Field:
        {
            const core::FieldDefinition* field = project.findField(ref.fieldId);
            if (field == nullptr)
            {
                detail.title = NodeDetailFormatter::tr("未知场");
                detail.subtitle = NodeDetailFormatter::tr("该场已从工程中删除");
                detail.html = paragraph(esc(NodeDetailFormatter::tr("没有可显示的内容。")));
                break;
            }

            detail.title = fromStd(field->name());
            detail.subtitle = fromStd(std::string(core::fieldClassName(field->fieldClass())));

            QString rows;
            rows += row(NodeDetailFormatter::tr("名称"), esc(fromStd(field->name())));
            rows += row(NodeDetailFormatter::tr("类型"),
                        esc(fromStd(std::string(core::fieldClassName(field->fieldClass())))));
            rows += row(NodeDetailFormatter::tr("dimensions"),
                        NodeDetailFormatter::tr("%1<br/><small>七维指数：kg m s K mol A cd</small>")
                            .arg(esc(dimensionsText(field->dimensions()))));
            rows += row(QStringLiteral("internalField"), esc(fieldValueText(field->internalField())));
            detail.html = table(rows);

            QString bcRows;
            for (const core::BoundaryPatch& patch : project.patches())
            {
                bcRows += row(fromStd(patch.name()),
                              esc(boundaryConditionText(field->findBoundaryCondition(patch.id()))));
            }
            if (!bcRows.isEmpty())
            {
                detail.html += section(NodeDetailFormatter::tr("边界条件")) + table(bcRows);
            }
            detail.html += issuesHtml(validation, {field->id()}, QString());
            break;
        }
        case NodeKind::FieldBoundary:
        {
            const core::FieldDefinition* field = project.findField(ref.fieldId);
            const core::BoundaryPatch* patch = project.findPatch(ref.patchId);
            if (field == nullptr || patch == nullptr)
            {
                detail.title = NodeDetailFormatter::tr("未知边界条件");
                detail.subtitle = NodeDetailFormatter::tr("场或 patch 已从工程中删除");
                detail.html = paragraph(esc(NodeDetailFormatter::tr("没有可显示的内容。")));
                break;
            }

            const core::BoundaryCondition* condition = field->findBoundaryCondition(patch->id());
            detail.title = NodeDetailFormatter::tr("%1 · %2")
                               .arg(fromStd(patch->name()), fromStd(field->name()));
            detail.subtitle = condition != nullptr && !condition->isEmpty()
                ? fromStd(condition->type())
                : NodeDetailFormatter::tr("未设置边界条件");

            QString rows;
            rows += row(NodeDetailFormatter::tr("场"), esc(fromStd(field->name())));
            rows += row(QStringLiteral("patch"), esc(fromStd(patch->name())));
            rows += row(NodeDetailFormatter::tr("patch 类型"), esc(patchTypeText(*patch)));
            rows += row(NodeDetailFormatter::tr("边界条件类型"),
                        esc(boundaryConditionText(condition)));
            detail.html = table(rows);

            if (condition != nullptr && !condition->parameters().empty())
            {
                QString parameterRows;
                for (const auto& [name, value] : condition->parameters())
                {
                    parameterRows += row(fromStd(name),
                                         QStringLiteral("<code>%1</code>").arg(esc(fromStd(value))));
                }
                detail.html += section(NodeDetailFormatter::tr("参数")) + table(parameterRows);
            }
            detail.html += issuesHtml(validation, {field->id()}, QString());
            break;
        }
        default:
            break;
    }

    return detail;
}

} // namespace ofgui::detail
