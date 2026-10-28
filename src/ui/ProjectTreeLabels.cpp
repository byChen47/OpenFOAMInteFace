#include "ui/ProjectTreeLabels.h"

#include "ui/CaseDocumentController.h"

#include "core/BoundaryPatch.h"
#include "core/FieldDefinition.h"

#include <QCoreApplication>
#include <QStringList>

namespace ofgui {
namespace {

QString tr(const char* text)
{
    return QCoreApplication::translate("ProjectTreeModel", text);
}

QString fieldClassText(core::FieldClass fieldClass)
{
    return QString::fromStdString(std::string(core::fieldClassName(fieldClass)));
}

const core::BoundaryPatch* owningPatch(const core::CaseProject& project, core::FaceId face)
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

QString treeNodeLabel(const CaseDocumentController& document, const ProjectNodeRef& ref)
{
    const core::CaseProject& project = document.project();

    switch (ref.kind)
    {
        case NodeKind::Project:
        {
            QString text = tr("工程 · %1").arg(document.displayName());
            if (document.isDirty())
            {
                text += QStringLiteral(" *");
            }
            return text;
        }
        case NodeKind::Domain:
            return tr("计算域");
        case NodeKind::Mesh:
            return tr("网格");
        case NodeKind::Boundaries:
            return tr("边界（六面）");
        case NodeKind::BoundaryFace:
        {
            const QString faceName = QString::fromLatin1(core::faceName(ref.face));
            const core::BoundaryPatch* patch = owningPatch(project, ref.face);
            if (patch == nullptr)
            {
                return tr("%1 — 未分配").arg(faceName);
            }
            return tr("%1 → %2 (%3)")
                .arg(faceName,
                     QString::fromStdString(patch->name()),
                     QString::fromStdString(std::string(core::patchTypeName(patch->type()))));
        }
        case NodeKind::Fields:
            return tr("场（0/）");
        case NodeKind::Field:
        {
            const core::FieldDefinition* field = project.findField(ref.fieldId);
            if (field == nullptr)
            {
                return tr("未知场");
            }
            return tr("%1 [%2]")
                .arg(QString::fromStdString(field->name()), fieldClassText(field->fieldClass()));
        }
        case NodeKind::FieldBoundary:
        {
            const core::BoundaryPatch* patch = project.findPatch(ref.patchId);
            const QString patchName = patch != nullptr
                ? QString::fromStdString(patch->name())
                : tr("未知边界");
            const core::FieldDefinition* field = project.findField(ref.fieldId);
            const core::BoundaryCondition* condition =
                field != nullptr ? field->findBoundaryCondition(ref.patchId) : nullptr;
            if (condition == nullptr || condition->isEmpty())
            {
                return tr("%1 — 未设置").arg(patchName);
            }
            return tr("%1 — %2").arg(patchName, QString::fromStdString(condition->type()));
        }
        case NodeKind::Solver:
            return tr("求解器设置");
        case NodeKind::ExportPreview:
            return tr("导出预览");
    }
    return {};
}

QString treeNodeToolTip(const CaseDocumentController& document, const ProjectNodeRef& ref)
{
    const core::CaseProject& project = document.project();

    switch (ref.kind)
    {
        case NodeKind::Project:
        {
            QStringList lines;
            lines << tr("工程：%1").arg(document.displayName());
            const QString caseDirectory = document.caseDirectory();
            if (!caseDirectory.isEmpty())
            {
                lines << tr("源算例：%1").arg(caseDirectory);
            }
            const QString projectFile = document.projectFilePath();
            if (!projectFile.isEmpty())
            {
                lines << tr("工程文件：%1").arg(projectFile);
            }
            lines << tr("状态：%1").arg(document.isDirty() ? tr("未保存") : tr("已保存"));
            return lines.join(QLatin1Char('\n'));
        }
        case NodeKind::Domain:
            return tr("长方体计算域：最小点、最大点、单位与 2D/3D 模式。");
        case NodeKind::Mesh:
            return tr("blockMesh 网格：各方向单元数、渐变与缩放。");
        case NodeKind::Boundaries:
            return tr("六个边界面及其所属 patch。选择一面可查看该 patch 上各场的边界条件。");
        case NodeKind::BoundaryFace:
        {
            const core::BoundaryPatch* patch = owningPatch(project, ref.face);
            const QString faceName = QString::fromLatin1(core::faceName(ref.face));
            if (patch == nullptr)
            {
                return tr("面 %1 尚未分配给任何 patch。").arg(faceName);
            }
            return tr("面 %1\n所属 patch：%2\n类型：%3")
                .arg(faceName,
                     QString::fromStdString(patch->name()),
                     QString::fromStdString(std::string(core::patchTypeName(patch->type()))));
        }
        case NodeKind::Fields:
            return tr("从 0/ 目录识别的场文件（volScalarField / volVectorField）。");
        case NodeKind::Field:
        {
            const core::FieldDefinition* field = project.findField(ref.fieldId);
            if (field == nullptr)
            {
                return tr("该场已从工程中删除。");
            }
            return tr("场 %1\n类型：%2\n边界条件：%3 个")
                .arg(QString::fromStdString(field->name()),
                     fieldClassText(field->fieldClass()))
                .arg(field->boundaryConditions().size());
        }
        case NodeKind::FieldBoundary:
        {
            const core::BoundaryPatch* patch = project.findPatch(ref.patchId);
            const core::FieldDefinition* field = project.findField(ref.fieldId);
            const QString patchName = patch != nullptr
                ? QString::fromStdString(patch->name())
                : tr("未知边界");
            const core::BoundaryCondition* condition =
                field != nullptr ? field->findBoundaryCondition(ref.patchId) : nullptr;
            QStringList lines;
            lines << tr("patch：%1").arg(patchName);
            lines << tr("边界条件：%1")
                .arg(condition != nullptr && !condition->isEmpty()
                        ? QString::fromStdString(condition->type())
                        : tr("未设置"));
            if (condition != nullptr)
            {
                lines << tr("参数：%1 项").arg(condition->parameters().size());
            }
            return lines.join(QLatin1Char('\n'));
        }
        case NodeKind::Solver:
            return tr("controlDict 求解设置：求解器、时间步、结束时间与写出间隔。");
        case NodeKind::ExportPreview:
            return tr("导出前预览将要写出的文件与校验结果。");
    }
    return {};
}

} // namespace ofgui
