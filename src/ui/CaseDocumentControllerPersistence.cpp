// Project-file lifecycle concern of CaseDocumentController: new project and
// `.ofgproj` load/save. Kept in its own translation unit because the controller
// file is over the 400-line limit (D-021).
#include "ui/CaseDocumentController.h"

#include "io/ProjectJsonRepository.h"

#include "ui/CaseDocumentControllerPaths.h"

#include <QFileInfo>

namespace ofgui {

void CaseDocumentController::newProject()
{
    applyDefaultTemplate(tr("未命名工程"));
    refreshValidation();
    setDirty(false);
    setError(QString());

    emit projectChanged();
    emit importReportChanged();
    emit validationChanged();
    emit documentStateChanged();
}

bool CaseDocumentController::openProjectFile(const QString& path)
{
    if (path.trimmed().isEmpty())
    {
        setError(tr("请选择一个工程文件（.ofgproj）。"));
        return false;
    }

    const io::ProjectLoadResult loaded = io::ProjectJsonRepository::load(detail::toPath(path));
    if (!loaded.ok)
    {
        setError(tr("无法加载工程文件：\n%1").arg(QString::fromStdString(loaded.diagnosticsText())));
        return false;
    }

    m_project = loaded.project;
    m_extras = loaded.extras;
    m_importReport.reset();
    m_readOnlySettings.clear();
    m_transport = {};
    clearCaseDictionaries();
    m_projectFilePath = QFileInfo(path).absoluteFilePath();
    loadCaseDictionaries(caseDirectory());
    refreshValidation();
    setDirty(false);
    setError(QString());

    emit projectChanged();
    emit importReportChanged();
    emit validationChanged();
    emit documentStateChanged();
    return true;
}

bool CaseDocumentController::saveProject()
{
    if (m_projectFilePath.isEmpty())
    {
        setError(tr("工程尚未保存到文件，请使用“另存为”。"));
        return false;
    }
    return saveProjectAs(m_projectFilePath);
}

bool CaseDocumentController::saveProjectAs(const QString& path)
{
    QString target = path.trimmed();
    if (target.isEmpty())
    {
        setError(tr("请选择一个工程文件路径。"));
        return false;
    }
    if (QFileInfo(target).suffix().isEmpty())
    {
        target += QStringLiteral(".ofgproj");
    }

    const io::ProjectSaveResult saved =
        io::ProjectJsonRepository::save(detail::toPath(target), m_project, m_extras);
    if (!saved.ok)
    {
        setError(tr("无法保存工程文件：\n%1").arg(QString::fromStdString(saved.diagnosticsText())));
        return false;
    }

    m_projectFilePath = QFileInfo(target).absoluteFilePath();
    setDirty(false);
    setError(QString());
    emit documentStateChanged();
    return true;
}

} // namespace ofgui
