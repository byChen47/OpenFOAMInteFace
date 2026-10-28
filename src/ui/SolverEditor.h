#pragma once

#include <QString>
#include <QWidget>

#include <functional>

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QPlainTextEdit;
class QSpinBox;
class QTabWidget;

namespace ofgui {

class CaseDocumentController;

/// Solver settings page of the inspector.
///
/// Columns:
///  - `controlDict`: `application`, `startTime`, `endTime`, `deltaT` and
///    `writeInterval` are edited through `core::SolverSettings`; the keys the
///    model does not carry yet (`startFrom`, `stopAt`, `writeControl`) are shown
///    with their imported value and the fixed exporter value, never as editable
///    controls that would silently do nothing.
///  - `constant`: `transportModel` and `nu` are edited through
///    `io::TransportModelSettings`; the same object the exporter consumes.
///    `simulationType` is read only (only `laminar` is supported today).
///  - Source dictionaries (`fvSchemes`, `fvSolution`, `controlDict`, ...) are
///    shown as their imported text, read only.
///  - `导出预览`: `system/controlDict` and `constant/transportProperties` are
///    rendered by the writer classes, so preview and export cannot drift.
class SolverEditor final : public QWidget
{
    Q_OBJECT

public:
    explicit SolverEditor(QWidget* parent = nullptr);

    void setDocument(CaseDocumentController* document);

    /// Re-reads the document into the widgets; never writes back to the model.
    void refresh();

    [[nodiscard]] QComboBox* applicationCombo() const noexcept;
    [[nodiscard]] QDoubleSpinBox* startTimeSpin() const noexcept;
    [[nodiscard]] QDoubleSpinBox* endTimeSpin() const noexcept;
    [[nodiscard]] QDoubleSpinBox* deltaTSpin() const noexcept;
    [[nodiscard]] QSpinBox* writeIntervalSpin() const noexcept;
    [[nodiscard]] QLabel* controlDictNote() const noexcept;
    [[nodiscard]] QLabel* controlDictError() const noexcept;

    [[nodiscard]] QComboBox* transportModelCombo() const noexcept;
    [[nodiscard]] QDoubleSpinBox* nuSpin() const noexcept;
    [[nodiscard]] QLabel* turbulenceLabel() const noexcept;
    [[nodiscard]] QLabel* transportNote() const noexcept;

    [[nodiscard]] QComboBox* previewCombo() const noexcept;
    [[nodiscard]] QPlainTextEdit* previewText() const noexcept;
    [[nodiscard]] QComboBox* rawCombo() const noexcept;
    [[nodiscard]] QPlainTextEdit* rawText() const noexcept;
    [[nodiscard]] QTabWidget* tabs() const noexcept;

    /// `system/controlDict` / `constant/transportProperties` currently shown.
    [[nodiscard]] QString currentPreviewPath() const;

private:
    void applySolverChange(const std::function<void()>& change);
    void applyTransportChange(const std::function<void()>& change);
    void updatePreview();
    void updateRawText();
    void updateRaw();
    void showError(const QString& text);
    void clearError();
    void populateApplicationCombo();
    void populateTransportModelCombo();

    CaseDocumentController* m_document = nullptr;
    bool m_updating = false;

    QTabWidget* m_tabs = nullptr;

    QComboBox* m_application = nullptr;
    QDoubleSpinBox* m_startTime = nullptr;
    QDoubleSpinBox* m_endTime = nullptr;
    QDoubleSpinBox* m_deltaT = nullptr;
    QSpinBox* m_writeInterval = nullptr;
    QLabel* m_controlDictNote = nullptr;
    QLabel* m_error = nullptr;

    QComboBox* m_transportModel = nullptr;
    QDoubleSpinBox* m_nu = nullptr;
    QLabel* m_turbulence = nullptr;
    QLabel* m_transportNote = nullptr;

    QComboBox* m_previewCombo = nullptr;
    QPlainTextEdit* m_previewText = nullptr;
    QComboBox* m_rawCombo = nullptr;
    QPlainTextEdit* m_rawText = nullptr;
};

} // namespace ofgui
