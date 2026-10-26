#include "ui/ConfigurationPanel.h"

#include "ui/InspectorPanel.h"
#include "ui/ProjectTreeWidget.h"

#include <QLabel>
#include <QSplitter>
#include <QVBoxLayout>

namespace ofgui {

ConfigurationPanel::ConfigurationPanel(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("configurationPanel"));
    setMinimumWidth(300);
    setMaximumWidth(520);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 6, 10);
    layout->setSpacing(8);

    auto* heading = new QLabel(tr("配置"), this);
    heading->setObjectName(QStringLiteral("panelTitle"));
    layout->addWidget(heading);

    auto* splitter = new QSplitter(Qt::Vertical, this);
    splitter->setChildrenCollapsible(false);

    m_tree = new ProjectTreeWidget(splitter);
    m_inspector = new InspectorPanel(splitter);

    splitter->addWidget(m_tree);
    splitter->addWidget(m_inspector);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    splitter->setSizes({520, 300});

    layout->addWidget(splitter, 1);

    connect(m_tree, &ProjectTreeWidget::nodeSelected,
        m_inspector, &InspectorPanel::showNode);

    m_tree->selectFirstNode();
}

} // namespace ofgui