#pragma once

#include <QWidget>

namespace ofgui {

class InspectorPanel;
class ProjectTreeWidget;

class ConfigurationPanel final : public QWidget
{
    Q_OBJECT

public:
    explicit ConfigurationPanel(QWidget* parent = nullptr);

private:
    ProjectTreeWidget* m_tree = nullptr;
    InspectorPanel* m_inspector = nullptr;
};

} // namespace ofgui