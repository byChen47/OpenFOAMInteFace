#pragma once

#include <QMainWindow>

namespace ofgui {

class ConfigurationPanel;
class VtkViewportWidget;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void createMenus();
    void createToolBar();
    void createCentralLayout();
    void showAboutDialog();

    ConfigurationPanel* m_configurationPanel = nullptr;
    VtkViewportWidget* m_viewport = nullptr;
};

} // namespace ofgui