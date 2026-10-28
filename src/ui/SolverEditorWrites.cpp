// Commit/preview concern of SolverEditor: writes go through the document
// controller and the preview is produced by the same writers the export uses.
// Kept in its own translation unit because the editor file is over the 400-line
// limit (D-021).
#include "ui/SolverEditor.h"

#include "io/writers/ControlDictWriter.h"
#include "io/writers/TransportPropertiesWriter.h"
#include "ui/CaseDocumentController.h"
#include "ui/SolverEditorPaths.h"

#include <QComboBox>
#include <QPlainTextEdit>
#include <QSignalBlocker>

namespace ofgui {

void SolverEditor::applySolverChange(const std::function<void()>& change)
{
    if (m_updating || m_document == nullptr)
    {
        return;
    }
    m_updating = true;
    change();
    m_updating = false;
    m_document->notifyProjectEdited();
    updatePreview();
}

void SolverEditor::applyTransportChange(const std::function<void()>& change)
{
    if (m_updating || m_document == nullptr)
    {
        return;
    }
    m_updating = true;
    change();
    m_updating = false;
    m_document->notifyTransportEdited();
    updatePreview();
}

void SolverEditor::updatePreview()
{
    if (m_document == nullptr)
    {
        m_previewText->clear();
        return;
    }

    if (m_previewCombo->currentText() == QString::fromLatin1(detail::kTransportPath))
    {
        m_previewText->setPlainText(QString::fromStdString(
            io::TransportPropertiesWriter::write(m_document->transportSettings())));
        return;
    }
    m_previewText->setPlainText(QString::fromStdString(
        io::ControlDictWriter::write(m_document->project())));
}

void SolverEditor::updateRawText()
{
    const QString previous = m_rawCombo->currentText();
    const QSignalBlocker blocker(m_rawCombo);
    m_rawCombo->clear();
    if (m_document == nullptr)
    {
        m_rawText->clear();
        return;
    }
    m_rawCombo->addItems(m_document->availableSourceDictionaries());
    const int index = m_rawCombo->findText(previous);
    m_rawCombo->setCurrentIndex(index >= 0 ? index : 0);
    updateRaw();
}

void SolverEditor::updateRaw()
{
    if (m_document == nullptr)
    {
        m_rawText->clear();
        return;
    }
    const QString path = m_rawCombo->currentText();
    if (path.isEmpty())
    {
        m_rawText->setPlainText(tr("导入的算例不包含可显示的字典文件。"));
        return;
    }
    m_rawText->setPlainText(m_document->sourceDictionaryText(path));
}

} // namespace ofgui
