#pragma once

#include "ui/ProjectNodeRef.h"

#include <QTreeView>

namespace ofgui {

class CaseDocumentController;
class ProjectTreeModel;

/// `QTreeView` bound to `ProjectTreeModel`.
///
/// The view owns no business state: it translates a row into `ProjectNodeRef`
/// and publishes `nodeSelected`, which is the only identity other panels use.
/// It keeps the selected node across model resets so an edit that republishes
/// the document does not move the user back to the project root.
class ProjectTreeView final : public QTreeView
{
    Q_OBJECT

public:
    explicit ProjectTreeView(QWidget* parent = nullptr);

    void setDocument(CaseDocumentController* document);

    [[nodiscard]] ProjectTreeModel* projectModel() const;
    [[nodiscard]] ProjectNodeRef currentNode() const;

public slots:
    /// Selects `ref`; falls back to the project node when it no longer exists.
    void selectNode(const ProjectNodeRef& ref);

    /// Selects the root project node.
    void selectProject();

signals:
    void nodeSelected(const ofgui::ProjectNodeRef& ref);

private:
    void restoreSelection();

    ProjectTreeModel* m_model = nullptr;
    ProjectNodeRef m_pendingRef;
    ProjectNodeRef m_lastEmitted;
    bool m_restoring = false;
};

} // namespace ofgui
