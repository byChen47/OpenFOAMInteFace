#pragma once

#include "core/FieldDefinition.h"
#include "core/ObjectId.h"

#include <QWidget>

#include <vector>

class QComboBox;
class QLabel;
class QLineEdit;
class QTableWidget;
class QTableWidgetItem;

namespace ofgui {

class CaseDocumentController;

/// Table of per-patch boundary conditions of one field (`0/<field>` →
/// `boundaryField`).
///
/// One row per patch. The patch id is the only identity; the patch name is a
/// label. Types known to the editor are editable, imported types outside the
/// whitelist stay read only (D-024) so saving never narrows the dictionary.
class FieldBoundaryTable final : public QWidget
{
    Q_OBJECT

public:
    explicit FieldBoundaryTable(QWidget* parent = nullptr);

    void setDocument(CaseDocumentController* document);
    void setField(const core::ObjectId& fieldId);
    [[nodiscard]] core::ObjectId fieldId() const noexcept;

    /// Rebuilds the rows and keeps the selected patch when it still exists.
    void refresh();

    void selectPatch(const core::ObjectId& patchId);
    [[nodiscard]] core::ObjectId currentPatchId() const;

    [[nodiscard]] QTableWidget* table() const noexcept;
    [[nodiscard]] QComboBox* typeCombo(int row) const;
    [[nodiscard]] QLineEdit* valueEdit(int row) const;
    [[nodiscard]] QString errorText() const;

signals:
    void boundaryEdited(const ofgui::core::ObjectId& patchId);
    void patchSelected(const ofgui::core::ObjectId& patchId);
    void errorTextChanged(const QString& message);

private:
    struct Row
    {
        core::ObjectId patchId;
        QComboBox* type = nullptr;
        QLineEdit* value = nullptr;
        QTableWidgetItem* nameItem = nullptr;
    };

    void rebuild();
    void applyRow(int row);
    void updateValueEditor(Row& row, const QString& type, bool hasCondition);
    void setError(const QString& message);
    void clearError();

    [[nodiscard]] QStringList offeredTypes() const;
    [[nodiscard]] bool typeNeedsValue(const QString& type) const;
    [[nodiscard]] bool validateValue(core::FieldClass fieldClass,
                                     const QString& type,
                                     const QString& text,
                                     QString* error) const;

    CaseDocumentController* m_document = nullptr;
    core::ObjectId m_fieldId;
    QTableWidget* m_table = nullptr;
    QLabel* m_error = nullptr;
    std::vector<Row> m_rows;
    bool m_rebuilding = false;
    bool m_applying = false;
};

} // namespace ofgui
