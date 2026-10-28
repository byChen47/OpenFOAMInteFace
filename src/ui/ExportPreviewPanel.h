#pragma once

#include "core/ObjectId.h"

#include <QStringList>
#include <QWidget>

class QComboBox;
class QLabel;
class QPlainTextEdit;

namespace ofgui {

class CaseDocumentController;

/// Preview of the `0/` files the exporter will write.
///
/// The files are produced by `io::FieldFileWriter`, i.e. by the same code the
/// exporter uses, so the preview cannot drift from the written dictionaries.
/// Files are addressed by field id, never by their display name.
class ExportPreviewPanel final : public QWidget
{
    Q_OBJECT

public:
    explicit ExportPreviewPanel(QWidget* parent = nullptr);

    void setDocument(CaseDocumentController* document);
    void refresh();

    void selectField(const core::ObjectId& fieldId);

    [[nodiscard]] QStringList availableFiles() const;
    [[nodiscard]] QString currentText() const;
    [[nodiscard]] QComboBox* fileCombo() const noexcept;
    [[nodiscard]] QPlainTextEdit* view() const noexcept;

private:
    void rebuildCombo();
    void updateText();

    CaseDocumentController* m_document = nullptr;
    core::ObjectId m_fieldId;
    QComboBox* m_combo = nullptr;
    QPlainTextEdit* m_view = nullptr;
    QLabel* m_note = nullptr;
    bool m_updating = false;
};

} // namespace ofgui
