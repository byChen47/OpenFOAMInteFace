#include "ui/ProjectTreeView.h"

#include "ui/CaseDocumentController.h"
#include "ui/ProjectTreeModel.h"

#include <QItemSelectionModel>

namespace ofgui {

ProjectTreeView::ProjectTreeView(QWidget* parent)
    : QTreeView(parent)
{
    setObjectName(QStringLiteral("projectTree"));
    setHeaderHidden(true);
    setUniformRowHeights(true);
    setAnimated(false);
    setExpandsOnDoubleClick(false);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setAllColumnsShowFocus(true);
}

void ProjectTreeView::setDocument(CaseDocumentController* document)
{
    if (m_model != nullptr)
    {
        disconnect(m_model, nullptr, this, nullptr);
    }

    delete m_model;
    m_model = new ProjectTreeModel(document, this);
    setModel(m_model);

    connect(m_model, &QAbstractItemModel::modelAboutToBeReset, this, [this]()
    {
        m_pendingRef = currentNode();
    });
    connect(m_model, &QAbstractItemModel::modelReset, this, &ProjectTreeView::restoreSelection);
    connect(selectionModel(), &QItemSelectionModel::currentChanged,
            this, [this](const QModelIndex& current, const QModelIndex&)
    {
        if (m_restoring || !current.isValid())
        {
            return;
        }
        const ProjectNodeRef ref = m_model->nodeRef(current);
        if (!ref.isValid() || ref == m_lastEmitted)
        {
            return;
        }
        m_lastEmitted = ref;
        emit nodeSelected(ref);
    });

    expandAll();
    selectProject();
}

ProjectTreeModel* ProjectTreeView::projectModel() const
{
    return m_model;
}

ProjectNodeRef ProjectTreeView::currentNode() const
{
    const QModelIndex current = currentIndex();
    return m_model != nullptr ? m_model->nodeRef(current) : ProjectNodeRef{};
}

void ProjectTreeView::selectNode(const ProjectNodeRef& ref)
{
    if (m_model == nullptr)
    {
        return;
    }
    const QModelIndex index = m_model->indexFor(ref);
    if (!index.isValid())
    {
        selectProject();
        return;
    }

    m_restoring = true;
    setCurrentIndex(index);
    scrollTo(index);
    m_restoring = false;

    m_lastEmitted = ref;
    emit nodeSelected(ref);
}

void ProjectTreeView::selectProject()
{
    if (m_model == nullptr)
    {
        return;
    }
    const QModelIndex index = m_model->index(0, 0);
    if (!index.isValid())
    {
        return;
    }

    m_restoring = true;
    setCurrentIndex(index);
    m_restoring = false;

    m_lastEmitted = ProjectNodeRef::section(NodeKind::Project);
    emit nodeSelected(m_lastEmitted);
}

void ProjectTreeView::restoreSelection()
{
    expandAll();
    selectNode(m_pendingRef.isValid() ? m_pendingRef : ProjectNodeRef::section(NodeKind::Project));
}

} // namespace ofgui
