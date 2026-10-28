#pragma once

#include <QMainWindow>
#include <QString>

class QAction;
class QCloseEvent;
class QDockWidget;
class QLabel;
class QProgressBar;

namespace ofgui {

class CaseDocumentController;
class ConfigurationPanel;
class ExportController;
class ValidationPanel;
class VtkViewportWidget;

/// Application window: owns the document controller and wires the tree to the
/// inspector. All file operations go through `CaseDocumentController`; the window
/// only chooses paths and shows errors (AGENTS.md §5).
class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    [[nodiscard]] CaseDocumentController* document() const noexcept;
    [[nodiscard]] ConfigurationPanel* configurationPanel() const noexcept;
    [[nodiscard]] VtkViewportWidget* viewport() const noexcept;
    [[nodiscard]] ExportController* exportController() const noexcept;
    [[nodiscard]] ValidationPanel* validationPanel() const noexcept;

public slots:
    /// Imports `directory` as a new OpenFOAM case (asks to save pending edits).
    bool openCaseDirectory(const QString& directory);

    /// Loads a `.ofgproj` project file (asks to save pending edits).
    bool openProjectFile(const QString& path);

    /// Saves to the known project file, or asks for a path when none is known.
    bool saveProject();

    /// Runs the export workflow without a dialog (used by tests and automation).
    bool exportCase(const QString& targetDirectory, bool allowExistingTarget);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void createActions();
    void createMenus();
    void createToolBar();
    void createCentralLayout();
    void createValidationDock();
    void createStatusBar();
    void connectDocument();

    void newProject();
    void promptOpenCase();
    void promptOpenProject();
    void promptExport();
    bool saveProjectAs();
    void closeDocument();
    bool maybeSaveChanges();
    void updateWindowTitle();
    void updateValidationStatus();
    void updateExportAvailability();
    /// Pushes the document domain into the VTK scene (no camera changes).
    void syncDomainToViewport();
    [[nodiscard]] QString defaultDirectory() const;
    [[nodiscard]] QString defaultExportDirectory() const;
    void showError(const QString& message);
    void showAboutDialog();

    CaseDocumentController* m_document = nullptr;
    ExportController* m_exportController = nullptr;
    ConfigurationPanel* m_configurationPanel = nullptr;
    ValidationPanel* m_validationPanel = nullptr;
    QDockWidget* m_validationDock = nullptr;
    VtkViewportWidget* m_viewport = nullptr;
    QLabel* m_validationLabel = nullptr;
    QProgressBar* m_progressBar = nullptr;

    QAction* m_newAction = nullptr;
    QAction* m_openCaseAction = nullptr;
    QAction* m_openProjectAction = nullptr;
    QAction* m_saveAction = nullptr;
    QAction* m_saveAsAction = nullptr;
    QAction* m_exportAction = nullptr;
    QAction* m_closeAction = nullptr;

    QString m_lastDirectory;
    QString m_lastExportMessage;
};

} // namespace ofgui
