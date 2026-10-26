#include "ui/InspectorPanel.h"

#include <QLabel>
#include <QVBoxLayout>

namespace ofgui {

InspectorPanel::InspectorPanel(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("inspectorPanel"));
    setMinimumHeight(220);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 8, 4, 4);
    layout->setSpacing(10);

    auto* heading = new QLabel(tr("属性"), this);
    heading->setObjectName(QStringLiteral("panelTitle"));
    layout->addWidget(heading);

    m_title = new QLabel(tr("请选择左侧配置项"), this);
    m_title->setObjectName(QStringLiteral("inspectorTitle"));
    m_title->setWordWrap(true);
    layout->addWidget(m_title);

    m_description = new QLabel(tr("选择一个配置节点后，此处将显示对应属性。"), this);
    m_description->setObjectName(QStringLiteral("inspectorDescription"));
    m_description->setWordWrap(true);
    m_description->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    layout->addWidget(m_description);

    auto* hint = new QLabel(
        tr("当前为 M0 界面骨架。参数编辑、校验和工程保存将在后续独立模块中实现。"),
        this);
    hint->setObjectName(QStringLiteral("inspectorHint"));
    hint->setWordWrap(true);
    layout->addWidget(hint);

    layout->addStretch(1);
}

void InspectorPanel::showNode(const QString& title, const QString& description)
{
    m_title->setText(title);
    m_description->setText(description);
}

} // namespace ofgui