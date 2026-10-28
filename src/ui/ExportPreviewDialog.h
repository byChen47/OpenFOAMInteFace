#pragma once

#include "io/writers/CaseExportPlan.h"

#include <QDialog>
#include <QString>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;

namespace ofgui {

class ExportController;

/// Export preview and confirmation dialog.
///
/// Shows the complete plan (`CaseExportPlan`) as a file tree with the action per
/// file plus the generated text of the selected file. The dialog never writes
/// anything: `MainWindow` runs `ExportController::exportTo()` after `accept()`,
/// so the write happens exactly once and stays testable without a modal dialog.
class ExportPreviewDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit ExportPreviewDialog(ExportController* controller, QWidget* parent = nullptr);
    ~ExportPreviewDialog() override;

    void setTargetDirectory(const QString& directory);
    [[nodiscard]] QString targetDirectory() const;
    [[nodiscard]] bool allowExistingTarget() const;

    void refreshPreview();

    [[nodiscard]] QLineEdit* targetEdit() const noexcept;
    [[nodiscard]] QCheckBox* overwriteCheck() const noexcept;
    [[nodiscard]] QTreeWidget* fileTree() const noexcept;
    [[nodiscard]] QPlainTextEdit* textPreview() const noexcept;
    [[nodiscard]] QLabel* statusLabel() const noexcept;
    [[nodiscard]] QPushButton* exportButton() const noexcept;

    /// True when the current target directory already holds files.
    [[nodiscard]] bool targetHasContent() const;

public slots:
    void accept() override;

private:
    void chooseTarget();
    void updateTextPreview();
    void updateExportEnabled();
    void showPlanItem(QTreeWidgetItem* item);

    ExportController* m_controller = nullptr;
    QLineEdit* m_targetEdit = nullptr;
    QCheckBox* m_overwriteCheck = nullptr;
    QTreeWidget* m_fileTree = nullptr;
    QPlainTextEdit* m_textPreview = nullptr;
    QLabel* m_statusLabel = nullptr;
    QPushButton* m_exportButton = nullptr;
    io::CaseExportPlan m_plan;
    bool m_targetHasContent = false;
    bool m_previewOk = false;
    QString m_previewError;
};

} // namespace ofgui
