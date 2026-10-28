#include "ui/CaseDocumentController.h"

#include "core/CaseValidator.h"

#include "ui/DefaultCaseTemplate.h"

#include <QFileInfo>

namespace ofgui {

CaseDocumentController::CaseDocumentController(QObject* parent)
    : QObject(parent)
{
    applyDefaultTemplate(tr("未命名工程"));
    refreshValidation();
}

void CaseDocumentController::applyDefaultTemplate(const QString& projectName)
{
    const DefaultCaseTemplate bundle = makeDefaultCavityTemplate(projectName.toStdString());
    m_project = bundle.project;
    m_transportSettings = bundle.transport;
    m_kinematicViscosityEditable = true;
    m_turbulenceSimulationType.clear();
    m_extras = {};
    m_importReport.reset();
    m_readOnlySettings.clear();
    m_transport = {};
    clearCaseDictionaries();
    m_projectFilePath.clear();
}

const core::CaseProject& CaseDocumentController::project() const noexcept
{
    return m_project;
}

core::CaseProject& CaseDocumentController::project() noexcept
{
    return m_project;
}

const core::ValidationResult& CaseDocumentController::validation() const noexcept
{
    return m_validation;
}

const io::ImportReport* CaseDocumentController::importReport() const noexcept
{
    return m_importReport ? &*m_importReport : nullptr;
}

const std::vector<io::ImportedSetting>& CaseDocumentController::readOnlySettings() const noexcept
{
    return m_readOnlySettings;
}

const io::TransportPropertiesProjection& CaseDocumentController::transport() const noexcept
{
    return m_transport;
}

io::TransportModelSettings& CaseDocumentController::transportSettings() noexcept
{
    return m_transportSettings;
}

const io::TransportModelSettings& CaseDocumentController::transportSettings() const noexcept
{
    return m_transportSettings;
}

bool CaseDocumentController::isKinematicViscosityEditable() const noexcept
{
    return m_kinematicViscosityEditable;
}

QString CaseDocumentController::turbulenceSimulationType() const
{
    return m_turbulenceSimulationType;
}

QString CaseDocumentController::projectFilePath() const
{
    return m_projectFilePath;
}

QString CaseDocumentController::displayName() const
{
    const QString name = QString::fromStdString(m_project.name());
    if (!name.trimmed().isEmpty())
    {
        return name;
    }
    if (!m_projectFilePath.isEmpty())
    {
        return QFileInfo(m_projectFilePath).completeBaseName();
    }
    return tr("未命名工程");
}

QString CaseDocumentController::caseDirectory() const
{
    return QString::fromStdString(m_project.caseDirectory());
}

bool CaseDocumentController::isDirty() const noexcept
{
    return m_dirty;
}

QString CaseDocumentController::lastError() const
{
    return m_lastError;
}

void CaseDocumentController::notifyProjectEdited()
{
    refreshValidation();
    setDirty(true);
    setError(QString());

    emit validationChanged();
    emit projectChanged();
    emit documentStateChanged();
}

void CaseDocumentController::notifyTransportEdited()
{
    setDirty(true);
    setError(QString());
    emit documentStateChanged();
}

void CaseDocumentController::refreshValidation()
{
    m_validation = core::CaseValidator().validate(m_project);
}

void CaseDocumentController::setDirty(bool dirty)
{
    if (m_dirty == dirty)
    {
        return;
    }
    m_dirty = dirty;
}

void CaseDocumentController::setError(const QString& message)
{
    m_lastError = message;
    if (!message.isEmpty())
    {
        emit errorOccurred(message);
    }
}

} // namespace ofgui
