#pragma once

#include "ui/ProjectNodeRef.h"

#include <QWidget>

class QLabel;
class QStackedWidget;
class QTextBrowser;

namespace ofgui {

class CaseDocumentController;
class DomainEditor;
class ExportPreviewPanel;
class FieldEditor;
class SolverEditor;

/// Detail panel for the current tree selection.
///
/// The panel owns no selection state beyond the last shown reference; it resolves
/// that reference against the document and shows the editable page for the node
/// kind (domain/mesh, field, export preview) or the read-only
/// `NodeDetailFormatter` page. All pages are addressed by stable IDs only.
class InspectorPanel final : public QWidget
{
    Q_OBJECT

public:
    explicit InspectorPanel(QWidget* parent = nullptr);

    void setDocument(CaseDocumentController* document);

    [[nodiscard]] ProjectNodeRef currentNode() const;
    [[nodiscard]] QString bodyText() const;

    /// The editable pages, valid after `setDocument()`.
    [[nodiscard]] DomainEditor* domainEditor() const noexcept;
    [[nodiscard]] FieldEditor* fieldEditor() const noexcept;
    [[nodiscard]] SolverEditor* solverEditor() const noexcept;
    [[nodiscard]] ExportPreviewPanel* exportPreview() const noexcept;

    /// True when the current selection is shown by the domain editor.
    [[nodiscard]] bool isEditingDomain() const;
    [[nodiscard]] bool isEditingField() const;
    [[nodiscard]] bool isEditingSolver() const;

public slots:
    /// Shows the node addressed by `ref`; falls back to the empty state when the
    /// node no longer exists in the document.
    void showNode(const ofgui::ProjectNodeRef& ref);

    /// Re-renders the current node after the document changed.
    void refresh();

signals:
    /// Raised when an editor created or deleted an object and the tree has to
    /// follow the new selection.
    void selectionRequested(const ofgui::ProjectNodeRef& ref);

private:
    void showPlaceholder();

    CaseDocumentController* m_document = nullptr;
    ProjectNodeRef m_current;
    QLabel* m_title = nullptr;
    QLabel* m_subtitle = nullptr;
    QStackedWidget* m_stack = nullptr;
    QTextBrowser* m_body = nullptr;
    DomainEditor* m_domainEditor = nullptr;
    FieldEditor* m_fieldEditor = nullptr;
    SolverEditor* m_solverEditor = nullptr;
    ExportPreviewPanel* m_exportPreview = nullptr;
};

} // namespace ofgui
