#pragma once

#include <QWidget>

namespace ofgui {

class CaseDocumentController;
class InspectorPanel;
class ProjectTreeView;

/// Left hand configuration area: project tree above, inspector below.
///
/// The panel is pure assembly: it forwards the document to both children and
/// connects the tree selection signal to the inspector slot.
class ConfigurationPanel final : public QWidget
{
    Q_OBJECT

public:
    explicit ConfigurationPanel(QWidget* parent = nullptr);

    void setDocument(CaseDocumentController* document);

    [[nodiscard]] ProjectTreeView* treeView() const noexcept;
    [[nodiscard]] InspectorPanel* inspector() const noexcept;

private:
    ProjectTreeView* m_tree = nullptr;
    InspectorPanel* m_inspector = nullptr;
};

} // namespace ofgui
