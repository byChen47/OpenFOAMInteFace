// Read/refresh concern of SolverEditor: pulls model, transport projection and
// imported read-only values into the widgets without writing them back. Kept in
// its own translation unit because the editor file is over the 400-line limit
// (D-021).
#include "ui/SolverEditor.h"

#include "core/CaseProject.h"
#include "core/SolverSettings.h"
#include "ui/CaseDocumentController.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTabWidget>

#include <vector>

namespace ofgui {
namespace {

QString settingValue(const std::vector<io::ImportedSetting>& settings, const QString& path)
{
    for (const io::ImportedSetting& setting : settings)
    {
        if (QString::fromStdString(setting.path) == path)
        {
            return QString::fromStdString(setting.text);
        }
    }
    return {};
}

} // namespace

void SolverEditor::setDocument(CaseDocumentController* document)
{
    m_document = document;
    refresh();
}

void SolverEditor::refresh()
{
    m_updating = true;
    clearError();
    const bool enabled = m_document != nullptr;
    m_tabs->setEnabled(enabled);

    if (!enabled)
    {
        m_controlDictNote->setText(tr("未加载工程。"));
        m_transportNote->clear();
        m_turbulence->clear();
        m_previewText->clear();
        m_rawText->clear();
        m_rawCombo->clear();
        m_updating = false;
        return;
    }

    const core::CaseProject& project = m_document->project();
    const core::SolverSettings& solver = project.solver();

    populateApplicationCombo();
    m_application->setCurrentText(QString::fromStdString(solver.application()));
    m_startTime->setValue(solver.startTime());
    m_endTime->setValue(solver.endTime());
    m_deltaT->setValue(solver.deltaT());
    m_writeInterval->setValue(solver.writeInterval());

    const QString startFrom =
        settingValue(m_document->readOnlySettings(), QStringLiteral("system/controlDict.startFrom"));
    const QString stopAt =
        settingValue(m_document->readOnlySettings(), QStringLiteral("system/controlDict.stopAt"));
    const QString writeControl =
        settingValue(m_document->readOnlySettings(), QStringLiteral("system/controlDict.writeControl"));
    m_controlDictNote->setText(
        tr("startFrom / stopAt / writeControl 尚未进入领域模型，导出固定写为 "
           "startTime / endTime / timeStep，这里显示导入原值（只读）：\n"
           "startFrom = %1，stopAt = %2，writeControl = %3。")
            .arg(startFrom.isEmpty() ? tr("（未声明）") : startFrom,
                 stopAt.isEmpty() ? tr("（未声明）") : stopAt,
                 writeControl.isEmpty() ? tr("（未声明）") : writeControl));

    const io::TransportModelSettings& transport = m_document->transportSettings();
    populateTransportModelCombo();
    const QString model = QString::fromStdString(transport.transportModel);
    const bool modelEditable = model.isEmpty() || model == QStringLiteral("Newtonian");
    m_transportModel->setEnabled(modelEditable);
    const int modelIndex = m_transportModel->findData(model);
    if (modelIndex >= 0)
    {
        m_transportModel->setCurrentIndex(modelIndex);
    }
    else
    {
        m_transportModel->addItem(model, model);
        m_transportModel->setCurrentIndex(m_transportModel->count() - 1);
    }

    const bool nuEditable = m_document->isKinematicViscosityEditable();
    m_nu->setEnabled(nuEditable);
    m_nu->setValue(transport.kinematicViscosity);

    // F-05（t14 评审）：transportModel/nu 目前只存在于会话内与源算例投影，
    // 不写入 .ofgproj；必须在此明示，避免保存/重开后静默回退被误认为丢失编辑。
    const QString persistenceNote =
        tr("transportModel/nu 不随 .ofgproj 保存；重新打开工程时会按源算例的 "
           "constant/transportProperties 重读。");
    if (nuEditable)
    {
        m_transportNote->setText(
            tr("nu 与 transportModel 会写入 constant/transportProperties（导出预览可核对）。")
            + QLatin1Char('\n') + persistenceNote);
    }
    else
    {
        m_transportNote->setText(
            tr("nu 使用了带量纲或表达式的写法（%1），为避免改变含义，首版保持只读并原样回写。")
                .arg(QString::fromStdString(transport.kinematicViscosityText))
            + QLatin1Char('\n') + persistenceNote);
    }

    const QString turbulence = m_document->turbulenceSimulationType();
    if (turbulence.isEmpty())
    {
        m_turbulence->setText(tr("（未声明，按 laminar 处理；只读）"));
    }
    else if (turbulence == QStringLiteral("laminar"))
    {
        m_turbulence->setText(tr("laminar（只读）"));
    }
    else
    {
        m_turbulence->setText(
            tr("%1（首版只写 laminar；该值只读，导出保持导入值）").arg(turbulence));
    }

    updateRawText();
    updatePreview();
    m_updating = false;
}

void SolverEditor::showError(const QString& text)
{
    m_error->setText(text);
    m_error->setVisible(true);
}

void SolverEditor::clearError()
{
    m_error->clear();
    m_error->setVisible(false);
}

void SolverEditor::populateApplicationCombo()
{
    // The combo is editable, so an application outside the list keeps its text.
}

void SolverEditor::populateTransportModelCombo()
{
    const QSignalBlocker blocker(m_transportModel);
    const QString previous = m_transportModel->currentData().toString();
    m_transportModel->clear();
    m_transportModel->addItem(tr("（默认 Newtonian）"), QString());
    for (const char* model : {"Newtonian", "BinghamPlastic", "HerschelBulkley", "BirdCarreau",
                              "CrossPowerLaw", "powerLaw"})
    {
        const QString text = QString::fromLatin1(model);
        m_transportModel->addItem(text, text);
    }
    const int index = m_transportModel->findData(previous);
    if (index >= 0)
    {
        m_transportModel->setCurrentIndex(index);
    }
}

} // namespace ofgui
