#include "ui/NodeDetailFormatter.h"

#include "ui/CaseDocumentController.h"
#include "ui/NodeDetailHtml.h"
#include "ui/NodeDetailSections.h"

#include "core/BoundaryPatch.h"
#include "core/CaseProject.h"
#include "core/DomainBox.h"
#include "core/MeshSettings.h"
#include "core/SolverSettings.h"

#include <QStringList>

#include <cstddef>
#include <cstdint>

namespace ofgui {
namespace {

using detail::esc;
using detail::fromStd;
using detail::number;
using detail::row;

QString modeText(core::DomainMode mode)
{
    return mode == core::DomainMode::TwoD
        ? NodeDetailFormatter::tr("2D（z 方向单层网格）")
        : NodeDetailFormatter::tr("3D");
}

QString sizeText(std::size_t count)
{
    return QString::number(static_cast<qulonglong>(count));
}

QString cellCountsText(const core::MeshSettings& mesh)
{
    const core::MeshSettings::CellCounts& cells = mesh.cells();
    return QStringLiteral("%1 × %2 × %3")
        .arg(cells[0])
        .arg(cells[1])
        .arg(cells[2]);
}

QString gradingText(const core::MeshSettings& mesh)
{
    const core::MeshSettings::Grading& grading = mesh.grading();
    return QStringLiteral("%1 × %2 × %3")
        .arg(number(grading[0]), number(grading[1]), number(grading[2]));
}

NodeDetail describeProject(const CaseDocumentController& document)
{
    const core::CaseProject& project = document.project();

    NodeDetail node;
    node.title = NodeDetailFormatter::tr("工程");
    node.subtitle = document.caseDirectory().isEmpty()
        ? NodeDetailFormatter::tr("新建工程 · 尚未加载算例")
        : NodeDetailFormatter::tr("来源算例：%1").arg(document.caseDirectory());

    QString rows;
    rows += row(NodeDetailFormatter::tr("名称"), esc(fromStd(project.name())));
    rows += row(NodeDetailFormatter::tr("来源目录"),
                project.caseDirectory().empty()
                    ? esc(NodeDetailFormatter::tr("未设置"))
                    : esc(fromStd(project.caseDirectory())));
    rows += row(NodeDetailFormatter::tr("patch 数"), esc(sizeText(project.patches().size())));
    rows += row(NodeDetailFormatter::tr("场文件数"), esc(sizeText(project.fields().size())));
    rows += row(NodeDetailFormatter::tr("求解器"),
                esc(fromStd(project.solver().application())));
    rows += row(NodeDetailFormatter::tr("校验"),
                esc(fromStd(document.validation().summary())));

    if (project.hasSource())
    {
        const core::CaseSourceInfo& source = project.source();
        rows += row(NodeDetailFormatter::tr("导入时间"), esc(fromStd(source.importedAt)));
        rows += row(NodeDetailFormatter::tr("记录文件"), esc(sizeText(source.files.size())));
    }
    node.html = detail::table(rows);

    if (const io::ImportReport* report = document.importReport(); report != nullptr)
    {
        QString reportRows;
        reportRows += row(NodeDetailFormatter::tr("起始目录"),
                          esc(fromStd(report->startDirectory)));
        reportRows += row(NodeDetailFormatter::tr("识别文件"),
                          esc(sizeText(report->files.size())));
        reportRows += row(NodeDetailFormatter::tr("部分支持"),
                          esc(sizeText(report->partialCount())));
        reportRows += row(NodeDetailFormatter::tr("导入问题"),
                          esc(sizeText(report->issues.size())));
        node.html += detail::section(NodeDetailFormatter::tr("导入结果"))
                     + detail::table(reportRows);
    }

    node.html += detail::issuesHtml(document.validation(), {}, QString());
    return node;
}

NodeDetail describeDomain(const CaseDocumentController& document)
{
    const core::CaseProject& project = document.project();
    const core::DomainBox& domain = project.domain();

    NodeDetail node;
    node.title = NodeDetailFormatter::tr("计算域");
    node.subtitle = NodeDetailFormatter::tr("%1 · 尺寸 %2 %3")
                        .arg(modeText(domain.mode()),
                             detail::vecText(domain.size()),
                             esc(fromStd(domain.unit())));

    QString rows;
    rows += row(NodeDetailFormatter::tr("模式"), esc(modeText(domain.mode())));
    rows += row(NodeDetailFormatter::tr("最小点"), esc(detail::vecText(domain.min())));
    rows += row(NodeDetailFormatter::tr("最大点"), esc(detail::vecText(domain.max())));
    rows += row(NodeDetailFormatter::tr("尺寸"), esc(detail::vecText(domain.size())));
    rows += row(NodeDetailFormatter::tr("体积"),
                esc(QStringLiteral("%1 %2³").arg(number(domain.volume()), fromStd(domain.unit()))));
    rows += row(NodeDetailFormatter::tr("单位"), esc(fromStd(domain.unit())));

    if (domain.mode() == core::DomainMode::ThreeD
        && domain.suggestMode() == core::DomainMode::TwoD)
    {
        rows += row(NodeDetailFormatter::tr("提示"),
                    esc(NodeDetailFormatter::tr("最薄方向不足最厚方向的 1%，可能是 2D 算例。")));
    }

    node.html = detail::table(rows)
                + detail::issuesHtml(document.validation(), {}, QStringLiteral("domain"));
    return node;
}

NodeDetail describeMesh(const CaseDocumentController& document)
{
    const core::CaseProject& project = document.project();
    const core::MeshSettings& mesh = project.mesh();

    NodeDetail node;
    node.title = NodeDetailFormatter::tr("网格");
    const std::int64_t total = mesh.totalCells();
    node.subtitle = NodeDetailFormatter::tr("%1 个单元")
                        .arg(QString::number(total > 0 ? total : 0));

    QString rows;
    rows += row(NodeDetailFormatter::tr("网格数"), esc(cellCountsText(mesh)));
    rows += row(NodeDetailFormatter::tr("单元总数"), esc(QString::number(total)));
    rows += row(NodeDetailFormatter::tr("grading"), esc(gradingText(mesh)));
    rows += row(NodeDetailFormatter::tr("scale"), esc(number(mesh.scale())));
    rows += row(NodeDetailFormatter::tr("单层单元"),
                esc(NodeDetailFormatter::tr("z 方向 %1 层").arg(mesh.cellCount(2))));
    rows += row(NodeDetailFormatter::tr("单元预算"),
                mesh.exceedsCellBudget()
                    ? esc(NodeDetailFormatter::tr("超过 %1 预警阈值")
                              .arg(static_cast<qlonglong>(core::MeshSettings::kCellBudgetWarning)))
                    : esc(NodeDetailFormatter::tr("在 %1 以内")
                              .arg(static_cast<qlonglong>(core::MeshSettings::kCellBudgetWarning))));

    node.html = detail::table(rows)
                + detail::issuesHtml(document.validation(), {}, QStringLiteral("mesh"));
    return node;
}

NodeDetail describeSolver(const CaseDocumentController& document)
{
    const core::CaseProject& project = document.project();
    const core::SolverSettings& solver = project.solver();

    NodeDetail node;
    node.title = NodeDetailFormatter::tr("求解器设置");
    node.subtitle = fromStd(solver.application());

    QString rows;
    rows += row(NodeDetailFormatter::tr("求解器"), esc(fromStd(solver.application())));
    rows += row(NodeDetailFormatter::tr("startTime"), esc(number(solver.startTime())));
    rows += row(NodeDetailFormatter::tr("endTime"), esc(number(solver.endTime())));
    rows += row(NodeDetailFormatter::tr("deltaT"), esc(number(solver.deltaT())));
    rows += row(NodeDetailFormatter::tr("writeInterval"),
                esc(QString::number(solver.writeInterval())));

    node.html = detail::table(rows)
                + detail::readOnlySettingsHtml(
                    document.readOnlySettings(), QStringLiteral("system/controlDict"), 8)
                + detail::issuesHtml(document.validation(), {}, QStringLiteral("solver"));
    return node;
}

NodeDetail describeExportPreview(const CaseDocumentController& document)
{
    const core::CaseProject& project = document.project();
    const core::ValidationResult& validation = document.validation();

    NodeDetail node;
    node.title = NodeDetailFormatter::tr("导出预览");
    node.subtitle = project.caseDirectory().empty()
        ? NodeDetailFormatter::tr("尚未设置导出目录")
        : NodeDetailFormatter::tr("目标目录：%1").arg(document.caseDirectory());

    QString rows;
    rows += row(NodeDetailFormatter::tr("目标目录"),
                project.caseDirectory().empty()
                    ? esc(NodeDetailFormatter::tr("未设置"))
                    : esc(fromStd(project.caseDirectory())));
    rows += row(NodeDetailFormatter::tr("patch 数"), esc(sizeText(project.patches().size())));
    rows += row(NodeDetailFormatter::tr("场文件数"), esc(sizeText(project.fields().size())));
    rows += row(NodeDetailFormatter::tr("阻断问题"),
                esc(QString::number(validation.count(core::Severity::BlockingError))));
    node.html = detail::table(rows);
    node.html += detail::paragraph(esc(NodeDetailFormatter::tr(
        "导出尚未在本版本启用：这里只汇总将要写出的内容与阻断校验问题。")));
    node.html += detail::issuesHtml(validation, {}, QString());
    return node;
}

NodeDetail describeUnknown(const ProjectNodeRef& ref)
{
    NodeDetail node;
    node.title = NodeDetailFormatter::tr("未知节点");
    node.subtitle = NodeDetailFormatter::tr("节点类型 %1")
                        .arg(static_cast<int>(ref.kind));
    node.html = detail::paragraph(esc(NodeDetailFormatter::tr("没有可显示的内容。")));
    return node;
}

} // namespace

NodeDetail NodeDetailFormatter::describe(
    const CaseDocumentController& document,
    const ProjectNodeRef& ref)
{
    if (!ref.isValid())
    {
        return describeUnknown(ref);
    }

    switch (ref.kind)
    {
        case NodeKind::Project:
            return describeProject(document);
        case NodeKind::Domain:
            return describeDomain(document);
        case NodeKind::Mesh:
            return describeMesh(document);
        case NodeKind::Solver:
            return describeSolver(document);
        case NodeKind::ExportPreview:
            return describeExportPreview(document);
        case NodeKind::Boundaries:
        case NodeKind::BoundaryFace:
        case NodeKind::Fields:
        case NodeKind::Field:
        case NodeKind::FieldBoundary:
            return detail::describeBoundaryNode(document, ref);
    }

    return describeUnknown(ref);
}

} // namespace ofgui
