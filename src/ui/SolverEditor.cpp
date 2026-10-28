#include "ui/SolverEditor.h"

#include "ui/CaseDocumentController.h"
#include "ui/SolverEditorPaths.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFontDatabase>
#include <QFormLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

namespace ofgui {
namespace {

QDoubleSpinBox* makeRealSpin(QWidget* parent, const QString& objectName, int decimals)
{
    auto* spin = new QDoubleSpinBox(parent);
    spin->setObjectName(objectName);
    spin->setDecimals(decimals);
    spin->setRange(-1.0e9, 1.0e9);
    spin->setSingleStep(0.001);
    return spin;
}

QPlainTextEdit* makeReadOnlyView(QWidget* parent, const QString& objectName)
{
    auto* view = new QPlainTextEdit(parent);
    view->setObjectName(objectName);
    view->setReadOnly(true);
    QFont fixed = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    fixed.setPointSize(9);
    view->setFont(fixed);
    return view;
}

} // namespace

SolverEditor::SolverEditor(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("solverEditor"));

    m_tabs = new QTabWidget(this);
    m_tabs->setObjectName(QStringLiteral("solverTabs"));

    // ---- controlDict -------------------------------------------------------
    auto* controlPage = new QWidget(this);
    auto* controlLayout = new QVBoxLayout(controlPage);
    controlLayout->setContentsMargins(0, 0, 0, 0);
    controlLayout->setSpacing(6);

    auto* controlForm = new QFormLayout();
    controlForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    m_application = new QComboBox(controlPage);
    m_application->setObjectName(QStringLiteral("applicationCombo"));
    m_application->setEditable(true);
    m_application->addItems({QStringLiteral("icoFoam"), QStringLiteral("pisoFoam"),
                             QStringLiteral("pimpleFoam"), QStringLiteral("simpleFoam"),
                             QStringLiteral("sonicFoam"), QStringLiteral("potentialFoam")});
    controlForm->addRow(tr("application"), m_application);

    m_startTime = makeRealSpin(controlPage, QStringLiteral("startTimeSpin"), 6);
    controlForm->addRow(tr("startTime"), m_startTime);

    m_endTime = makeRealSpin(controlPage, QStringLiteral("endTimeSpin"), 6);
    controlForm->addRow(tr("endTime"), m_endTime);

    m_deltaT = new QDoubleSpinBox(controlPage);
    m_deltaT->setObjectName(QStringLiteral("deltaTSpin"));
    m_deltaT->setDecimals(9);
    m_deltaT->setRange(1.0e-9, 1.0e6);
    m_deltaT->setSingleStep(1.0e-5);
    controlForm->addRow(tr("deltaT"), m_deltaT);

    m_writeInterval = new QSpinBox(controlPage);
    m_writeInterval->setObjectName(QStringLiteral("writeIntervalSpin"));
    m_writeInterval->setRange(1, 1'000'000'000);
    m_writeInterval->setSingleStep(10);
    controlForm->addRow(tr("writeInterval"), m_writeInterval);

    controlLayout->addLayout(controlForm);

    m_error = new QLabel(controlPage);
    m_error->setObjectName(QStringLiteral("solverErrorLabel"));
    m_error->setWordWrap(true);
    m_error->setStyleSheet(QStringLiteral("color: #b00020;"));
    m_error->setVisible(false);
    controlLayout->addWidget(m_error);

    m_controlDictNote = new QLabel(controlPage);
    m_controlDictNote->setObjectName(QStringLiteral("controlDictNote"));
    m_controlDictNote->setWordWrap(true);
    m_controlDictNote->setStyleSheet(QStringLiteral("color: palette(mid);"));
    controlLayout->addWidget(m_controlDictNote);
    controlLayout->addStretch(1);

    m_tabs->addTab(controlPage, tr("controlDict"));

    // ---- constant ----------------------------------------------------------
    auto* constantPage = new QWidget(this);
    auto* constantLayout = new QVBoxLayout(constantPage);
    constantLayout->setContentsMargins(0, 0, 0, 0);
    constantLayout->setSpacing(6);

    auto* constantForm = new QFormLayout();
    constantForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    m_transportModel = new QComboBox(constantPage);
    m_transportModel->setObjectName(QStringLiteral("transportModelCombo"));
    constantForm->addRow(tr("transportModel"), m_transportModel);

    m_nu = new QDoubleSpinBox(constantPage);
    m_nu->setObjectName(QStringLiteral("nuSpin"));
    m_nu->setDecimals(12);
    m_nu->setRange(1.0e-12, 1.0);
    m_nu->setSingleStep(1.0e-5);
    constantForm->addRow(tr("nu (m²/s)"), m_nu);

    m_turbulence = new QLabel(constantPage);
    m_turbulence->setObjectName(QStringLiteral("turbulenceLabel"));
    m_turbulence->setWordWrap(true);
    constantForm->addRow(tr("simulationType"), m_turbulence);
    constantLayout->addLayout(constantForm);

    m_transportNote = new QLabel(constantPage);
    m_transportNote->setObjectName(QStringLiteral("transportNote"));
    m_transportNote->setWordWrap(true);
    m_transportNote->setStyleSheet(QStringLiteral("color: palette(mid);"));
    constantLayout->addWidget(m_transportNote);
    constantLayout->addStretch(1);

    m_tabs->addTab(constantPage, tr("constant"));

    // ---- raw source text ---------------------------------------------------
    auto* rawPage = new QWidget(this);
    auto* rawLayout = new QVBoxLayout(rawPage);
    rawLayout->setContentsMargins(0, 0, 0, 0);
    rawLayout->setSpacing(6);

    m_rawCombo = new QComboBox(rawPage);
    m_rawCombo->setObjectName(QStringLiteral("rawDictionaryCombo"));
    rawLayout->addWidget(m_rawCombo);

    m_rawText = makeReadOnlyView(rawPage, QStringLiteral("rawDictionaryText"));
    rawLayout->addWidget(m_rawText, 1);

    m_tabs->addTab(rawPage, tr("只读原文"));

    // ---- export preview ----------------------------------------------------
    auto* previewPage = new QWidget(this);
    auto* previewLayout = new QVBoxLayout(previewPage);
    previewLayout->setContentsMargins(0, 0, 0, 0);
    previewLayout->setSpacing(6);

    m_previewCombo = new QComboBox(previewPage);
    m_previewCombo->setObjectName(QStringLiteral("solverPreviewCombo"));
    m_previewCombo->addItem(QString::fromLatin1(detail::kControlDictPath));
    m_previewCombo->addItem(QString::fromLatin1(detail::kTransportPath));
    previewLayout->addWidget(m_previewCombo);

    m_previewText = makeReadOnlyView(previewPage, QStringLiteral("solverPreviewText"));
    previewLayout->addWidget(m_previewText, 1);

    m_tabs->addTab(previewPage, tr("导出预览"));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_tabs);

    connect(m_application, &QComboBox::currentTextChanged, this, [this](const QString& text) {
        if (m_updating || m_document == nullptr)
        {
            return;
        }
        const QString trimmed = text.trimmed();
        if (trimmed.isEmpty())
        {
            showError(tr("application 不能为空；未修改模型。"));
            const QSignalBlocker blocker(m_application);
            m_application->setCurrentText(
                QString::fromStdString(m_document->project().solver().application()));
            return;
        }
        clearError();
        applySolverChange([this, trimmed]() {
            m_document->project().solver().setApplication(trimmed.toStdString());
        });
    });
    connect(m_startTime, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        if (m_updating || m_document == nullptr)
        {
            return;
        }
        if (!(value < m_endTime->value()))
        {
            showError(tr("startTime 必须小于 endTime；未修改模型。"));
            const QSignalBlocker blocker(m_startTime);
            m_startTime->setValue(m_document->project().solver().startTime());
            return;
        }
        clearError();
        applySolverChange([this, value]() { m_document->project().solver().setStartTime(value); });
    });
    connect(m_endTime, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        if (m_updating || m_document == nullptr)
        {
            return;
        }
        if (!(value > m_startTime->value()))
        {
            showError(tr("endTime 必须大于 startTime；未修改模型。"));
            const QSignalBlocker blocker(m_endTime);
            m_endTime->setValue(m_document->project().solver().endTime());
            return;
        }
        clearError();
        applySolverChange([this, value]() { m_document->project().solver().setEndTime(value); });
    });
    connect(m_deltaT, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        if (m_updating || m_document == nullptr)
        {
            return;
        }
        if (!(value > 0.0))
        {
            showError(tr("deltaT 必须大于 0；未修改模型。"));
            const QSignalBlocker blocker(m_deltaT);
            m_deltaT->setValue(m_document->project().solver().deltaT());
            return;
        }
        clearError();
        applySolverChange([this, value]() { m_document->project().solver().setDeltaT(value); });
    });
    connect(m_writeInterval, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
        if (m_updating || m_document == nullptr)
        {
            return;
        }
        if (value < 1)
        {
            showError(tr("writeInterval 必须大于等于 1；未修改模型。"));
            const QSignalBlocker blocker(m_writeInterval);
            m_writeInterval->setValue(m_document->project().solver().writeInterval());
            return;
        }
        clearError();
        applySolverChange([this, value]() { m_document->project().solver().setWriteInterval(value); });
    });

    connect(m_transportModel, &QComboBox::currentIndexChanged, this, [this](int) {
        const QString model = m_transportModel->currentData().toString();
        applyTransportChange([this, model]() {
            m_document->transportSettings().transportModel = model.toStdString();
        });
    });
    connect(m_nu, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        applyTransportChange([this, value]() {
            io::TransportModelSettings& settings = m_document->transportSettings();
            settings.kinematicViscosity = value;
            settings.kinematicViscosityText.clear();
        });
    });

    connect(m_previewCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        if (!m_updating)
        {
            updatePreview();
        }
    });
    connect(m_rawCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        if (!m_updating)
        {
            updateRawText();
        }
    });

    refresh();
}

} // namespace ofgui
