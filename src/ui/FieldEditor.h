#pragma once

#include "core/ObjectId.h"

#include <QWidget>

class QLabel;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QToolButton;
class QVBoxLayout;

namespace ofgui {

class CaseDocumentController;
class FieldBoundaryTable;
class UniformValueEditor;

/// Editor page for one `0/<field>` file: dimensions, `internalField` and the
/// per-patch `boundaryField` table.
///
/// The page is addressed by the field's stable id. `dimensions` is read only in
/// the first version (D-024), imported values are never narrowed. Edits go
/// through `CaseDocumentController::notifyProjectEdited()` so the tree and the
/// preview refresh from the model.
class FieldEditor final : public QWidget
{
    Q_OBJECT

public:
    explicit FieldEditor(QWidget* parent = nullptr);

    void setDocument(CaseDocumentController* document);

    /// Shows a field; keeps the current page when the id did not change.
    void showField(const core::ObjectId& fieldId);

    /// Shows a field and selects one patch row (tree `FieldBoundary` node).
    void showBoundary(const core::ObjectId& fieldId, const core::ObjectId& patchId);

    void clear();
    void refresh();

    [[nodiscard]] core::ObjectId fieldId() const noexcept;
    [[nodiscard]] FieldBoundaryTable* boundaryTable() const noexcept;
    [[nodiscard]] UniformValueEditor* internalFieldEditor() const noexcept;
    [[nodiscard]] QPlainTextEdit* previewView() const noexcept;
    [[nodiscard]] QString errorText() const;
    [[nodiscard]] QString previewText() const;
    [[nodiscard]] QToolButton* addFieldButton() const noexcept;
    [[nodiscard]] QPushButton* removeFieldButton() const noexcept;

    /// Adds `p` (`volScalarField`) or `U` (`volVectorField`) with zero interior
    /// value and `zeroGradient` on every patch. False when the name already
    /// exists or is not one of the two predefined names.
    [[nodiscard]] bool addPredefinedField(const QString& name);

    /// Removes the shown field when it is one of the predefined fields.
    [[nodiscard]] bool removeCurrentField();

signals:
    void fieldAdded(const ofgui::core::ObjectId& fieldId);
    void fieldRemoved();

private:
    QWidget* buildContent();
    void buildDimensionsGroup(QWidget* parent, QVBoxLayout* layout);
    void buildInternalFieldGroup(QWidget* parent, QVBoxLayout* layout);
    void buildBoundaryGroup(QWidget* parent, QVBoxLayout* layout);
    void buildPreviewGroup(QWidget* parent, QVBoxLayout* layout);

    void commitInternalField();
    void updatePreview();
    void setEnabledState(bool enabled);
    void setError(const QString& message);
    void clearError();

    CaseDocumentController* m_document = nullptr;
    core::ObjectId m_fieldId;
    QLabel* m_class = nullptr;
    QLabel* m_internalNote = nullptr;
    QLabel* m_error = nullptr;
    QSpinBox* m_dimensions[7] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
    UniformValueEditor* m_internalField = nullptr;
    FieldBoundaryTable* m_boundaryTable = nullptr;
    QPlainTextEdit* m_preview = nullptr;
    QToolButton* m_addButton = nullptr;
    QPushButton* m_removeButton = nullptr;
    bool m_updating = false;
};

} // namespace ofgui
