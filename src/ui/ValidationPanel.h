#pragma once

#include "ui/ProjectNodeRef.h"

#include <QWidget>

class QLabel;
class QTreeWidget;
class QTreeWidgetItem;

namespace ofgui {

class CaseDocumentController;

/// Bottom panel listing every `core::ValidationIssue` of the document.
///
/// Double-clicking an issue emits `navigateRequested` with the tree node that
/// owns the offending value, so an error is always reachable without guessing
/// (`docs/ARCHITECTURE.md` §5.7). The panel never edits the model: fixes happen
/// in the domain/field/solver editors.
class ValidationPanel final : public QWidget
{
    Q_OBJECT

public:
    explicit ValidationPanel(QWidget* parent = nullptr);

    void setDocument(CaseDocumentController* document);
    void refresh();

    [[nodiscard]] int issueCount() const;
    [[nodiscard]] QTreeWidget* tree() const noexcept;
    [[nodiscard]] ProjectNodeRef nodeRefForRow(int row) const;

    /// Simulates a double-click on `row`; returns false for an empty row.
    bool activateRow(int row);

signals:
    void navigateRequested(ofgui::ProjectNodeRef ref);

private:
    void activateItem(QTreeWidgetItem* item);

    CaseDocumentController* m_document = nullptr;
    QLabel* m_summary = nullptr;
    QTreeWidget* m_tree = nullptr;
};

} // namespace ofgui
