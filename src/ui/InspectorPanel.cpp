#include "ui/InspectorPanel.h"

#include "ui/CaseDocumentController.h"
#include "ui/DomainEditor.h"
#include "ui/ExportPreviewPanel.h"
#include "ui/FieldEditor.h"
#include "ui/NodeDetailFormatter.h"
#include "ui/SolverEditor.h"

#include <QFont>
#include <QLabel>
#include <QStackedWidget>
#include <QTextBrowser>
#include <QVBoxLayout>

namespace ofgui {

InspectorPanel::InspectorPanel(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("inspectorPanel"));

    m_title = new QLabel(this);
    m_title->setObjectName(QStringLiteral("inspectorTitle"));
    m_title->setWordWrap(true);

    QFont titleFont = m_title->font();
    titleFont.setPointSizeF(titleFont.pointSizeF() * 1.15);
    titleFont.setBold(true);
    m_title->setFont(titleFont);

    m_subtitle = new QLabel(this);
    m_subtitle->setObjectName(QStringLiteral("inspectorSubtitle"));
    m_subtitle->setWordWrap(true);

    m_body = new QTextBrowser(this);
    m_body->setObjectName(QStringLiteral("inspectorBody"));
    m_body->setOpenExternalLinks(false);
    m_body->setReadOnly(true);

    m_domainEditor = new DomainEditor(this);
    m_fieldEditor = new FieldEditor(this);
    m_solverEditor = new SolverEditor(this);
    m_exportPreview = new ExportPreviewPanel(this);

    m_stack = new QStackedWidget(this);
    m_stack->setObjectName(QStringLiteral("inspectorStack"));
    m_stack->addWidget(m_body);
    m_stack->addWidget(m_domainEditor);
    m_stack->addWidget(m_fieldEditor);
    m_stack->addWidget(m_solverEditor);
    m_stack->addWidget(m_exportPreview);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    layout->addWidget(m_title);
    layout->addWidget(m_subtitle);
    layout->addWidget(m_stack, 1);

    connect(m_fieldEditor, &FieldEditor::fieldAdded, this, [this](const core::ObjectId& fieldId) {
        emit selectionRequested(ProjectNodeRef::forField(fieldId));
    });
    connect(m_fieldEditor, &FieldEditor::fieldRemoved, this, [this]() {
        emit selectionRequested(ProjectNodeRef::section(NodeKind::Fields));
    });

    showPlaceholder();
}

void InspectorPanel::setDocument(CaseDocumentController* document)
{
    if (m_document != nullptr)
    {
        disconnect(m_document, nullptr, this, nullptr);
    }
    m_document = document;
    m_domainEditor->setDocument(document);
    m_fieldEditor->setDocument(document);
    m_solverEditor->setDocument(document);
    m_exportPreview->setDocument(document);

    if (m_document == nullptr)
    {
        showPlaceholder();
        return;
    }

    connect(m_document, &CaseDocumentController::projectChanged,
            this, &InspectorPanel::refresh);
    connect(m_document, &CaseDocumentController::validationChanged,
            this, &InspectorPanel::refresh);
    connect(m_document, &CaseDocumentController::documentStateChanged,
            this, &InspectorPanel::refresh);
    connect(m_document, &CaseDocumentController::importReportChanged,
            this, &InspectorPanel::refresh);

    refresh();
}

ProjectNodeRef InspectorPanel::currentNode() const
{
    return m_current;
}

QString InspectorPanel::bodyText() const
{
    return m_body->toPlainText();
}

DomainEditor* InspectorPanel::domainEditor() const noexcept
{
    return m_domainEditor;
}

FieldEditor* InspectorPanel::fieldEditor() const noexcept
{
    return m_fieldEditor;
}

SolverEditor* InspectorPanel::solverEditor() const noexcept
{
    return m_solverEditor;
}

ExportPreviewPanel* InspectorPanel::exportPreview() const noexcept
{
    return m_exportPreview;
}

bool InspectorPanel::isEditingDomain() const
{
    return m_stack->currentWidget() == m_domainEditor;
}

bool InspectorPanel::isEditingField() const
{
    return m_stack->currentWidget() == m_fieldEditor;
}

bool InspectorPanel::isEditingSolver() const
{
    return m_stack->currentWidget() == m_solverEditor;
}

void InspectorPanel::showNode(const ProjectNodeRef& ref)
{
    if (m_document == nullptr)
    {
        showPlaceholder();
        return;
    }

    bool resolvable = true;
    if (ref.kind == NodeKind::Field)
    {
        resolvable = m_document->project().findField(ref.fieldId) != nullptr;
    }
    else if (ref.kind == NodeKind::FieldBoundary)
    {
        resolvable = m_document->project().findField(ref.fieldId) != nullptr
                     && m_document->project().findPatch(ref.patchId) != nullptr;
    }
    if (!ref.isValid() || !resolvable)
    {
        showPlaceholder();
        return;
    }

    m_current = ref;
    const NodeDetail detail = NodeDetailFormatter::describe(*m_document, ref);
    m_title->setText(detail.title);
    m_subtitle->setText(detail.subtitle);

    if (ref.kind == NodeKind::Domain || ref.kind == NodeKind::Mesh)
    {
        m_domainEditor->refresh();
        m_stack->setCurrentWidget(m_domainEditor);
        return;
    }
    if (ref.kind == NodeKind::Field)
    {
        m_fieldEditor->showField(ref.fieldId);
        m_stack->setCurrentWidget(m_fieldEditor);
        return;
    }
    if (ref.kind == NodeKind::FieldBoundary)
    {
        m_fieldEditor->showBoundary(ref.fieldId, ref.patchId);
        m_stack->setCurrentWidget(m_fieldEditor);
        return;
    }
    if (ref.kind == NodeKind::Solver)
    {
        m_solverEditor->refresh();
        m_stack->setCurrentWidget(m_solverEditor);
        return;
    }
    if (ref.kind == NodeKind::ExportPreview)
    {
        m_exportPreview->refresh();
        m_stack->setCurrentWidget(m_exportPreview);
        return;
    }

    m_body->setHtml(detail.html);
    m_stack->setCurrentWidget(m_body);
}

void InspectorPanel::refresh()
{
    if (m_current.isValid())
    {
        showNode(m_current);
        return;
    }
    showPlaceholder();
}

void InspectorPanel::showPlaceholder()
{
    m_current = {};
    m_title->setText(tr("属性"));
    m_subtitle->setText(tr("未选择对象"));
    m_body->setHtml(QStringLiteral("<p>%1</p>")
                        .arg(tr("在左侧工程树中选择计算域、网格、边界或场以查看详情。")));
    m_stack->setCurrentWidget(m_body);
}

} // namespace ofgui
