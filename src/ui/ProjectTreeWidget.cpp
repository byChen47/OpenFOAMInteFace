#include "ui/ProjectTreeWidget.h"

#include "render/FaceAppearance.h"
#include "render/FaceId.h"

#include <QTreeWidgetItem>

namespace ofgui {
namespace {

QTreeWidgetItem* addNode(
    QTreeWidgetItem* parent,
    const QString& title,
    const QString& description,
    int faceValue = -1)
{
    auto* item = new QTreeWidgetItem(parent, QStringList{title});
    item->setData(0, Qt::UserRole, description);
    if (faceValue >= 0)
    {
        item->setData(0, Qt::UserRole + 1, faceValue);
    }
    return item;
}

} // namespace

ProjectTreeWidget::ProjectTreeWidget(QWidget* parent)
    : QTreeWidget(parent)
{
    setObjectName(QStringLiteral("projectTree"));
    setHeaderHidden(true);
    setColumnCount(1);
    setAnimated(true);
    setIndentation(18);
    setUniformRowHeights(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setExpandsOnDoubleClick(true);

    populateTree();

    connect(this, &QTreeWidget::currentItemChanged, this,
        [this](QTreeWidgetItem* current, QTreeWidgetItem*)
        {
            if (current == nullptr)
            {
                return;
            }

            emit nodeSelected(
                current->text(0),
                current->data(0, Qt::UserRole).toString());
        });
}

void ProjectTreeWidget::populateTree()
{
    auto* project = addNode(nullptr, tr("工程"), tr("当前工程的名称、路径和保存状态。"));
    addNode(project, tr("计算域"), tr("通过最小点和最大点定义长方体计算域。"));
    addNode(project, tr("网格"), tr("设置计算域的网格数量和基础分区方式。"));
    addNode(project, tr("场"), tr("配置 p、U 等场及其边界条件。"));

    auto* boundaries = addNode(project, tr("边界"), tr("选择边界面并设置 patch 名称和类型。"));
    for (const FaceId faceId : kAllFaceIds)
    {
        const auto appearance = faceAppearance(faceId);
        const QString title = QStringLiteral("%1 (%2)")
                                  .arg(QString::fromUtf8(appearance.key), QString::fromUtf8(appearance.displayName));
        addNode(
            boundaries,
            title,
            tr("长方体边界面，后续将在此设置名称、类型、颜色和边界条件。"),
            static_cast<int>(faceId));
    }

    addNode(project, tr("求解器设置"), tr("配置应用名称、时间控制和数值求解参数。"));
    addNode(project, tr("导出预览"), tr("检查并导出 0.orig、constant 和 system 文件。"));

    addTopLevelItem(project);
    project->setExpanded(true);
    boundaries->setExpanded(true);
}

void ProjectTreeWidget::selectFirstNode()
{
    if (topLevelItemCount() > 0)
    {
        setCurrentItem(topLevelItem(0));
    }
}

} // namespace ofgui