// Widget accessors of SolverEditor, used by the UI tests and by the screenshot
// hooks. Kept in its own translation unit because the editor file is over the
// 400-line limit (D-021).
#include "ui/SolverEditor.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QTabWidget>

namespace ofgui {

QComboBox* SolverEditor::applicationCombo() const noexcept
{
    return m_application;
}

QDoubleSpinBox* SolverEditor::startTimeSpin() const noexcept
{
    return m_startTime;
}

QDoubleSpinBox* SolverEditor::endTimeSpin() const noexcept
{
    return m_endTime;
}

QDoubleSpinBox* SolverEditor::deltaTSpin() const noexcept
{
    return m_deltaT;
}

QSpinBox* SolverEditor::writeIntervalSpin() const noexcept
{
    return m_writeInterval;
}

QLabel* SolverEditor::controlDictNote() const noexcept
{
    return m_controlDictNote;
}

QLabel* SolverEditor::controlDictError() const noexcept
{
    return m_error;
}

QComboBox* SolverEditor::transportModelCombo() const noexcept
{
    return m_transportModel;
}

QDoubleSpinBox* SolverEditor::nuSpin() const noexcept
{
    return m_nu;
}

QLabel* SolverEditor::turbulenceLabel() const noexcept
{
    return m_turbulence;
}

QLabel* SolverEditor::transportNote() const noexcept
{
    return m_transportNote;
}

QComboBox* SolverEditor::previewCombo() const noexcept
{
    return m_previewCombo;
}

QPlainTextEdit* SolverEditor::previewText() const noexcept
{
    return m_previewText;
}

QComboBox* SolverEditor::rawCombo() const noexcept
{
    return m_rawCombo;
}

QPlainTextEdit* SolverEditor::rawText() const noexcept
{
    return m_rawText;
}

QTabWidget* SolverEditor::tabs() const noexcept
{
    return m_tabs;
}

QString SolverEditor::currentPreviewPath() const
{
    return m_previewCombo->currentText();
}

} // namespace ofgui
