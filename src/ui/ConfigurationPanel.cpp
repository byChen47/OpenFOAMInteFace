#include "ui/ConfigurationPanel.h"

#include "ui/InspectorPanel.h"
#include "ui/ProjectTreeView.h"

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

    m_tree = new ProjectTreeView(splitter);
    m_inspector = new InspectorPanel(splitter);

    splitter->addWidget(m_tree);
    splitter->addWidget(m_inspector);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);
    splitter->setSizes({320, 520});

    layout->addWidget(splitter, 1);

    connect(m_tree, &ProjectTreeView::nodeSelected,
            m_inspector, &InspectorPanel::showNode);
    connect(m_inspector, &InspectorPanel::selectionRequested,
            m_tree, &ProjectTreeView::selectNode);
}

void ConfigurationPanel::setDocument(CaseDocumentController* document)
{
    m_tree->setDocument(document);
    m_inspector->setDocument(document);
}

ProjectTreeView* ConfigurationPanel::treeView() const noexcept
{
    return m_tree;
}

InspectorPanel* ConfigurationPanel::inspector() const noexcept
{
    return m_inspector;
}

} // namespace ofgui
