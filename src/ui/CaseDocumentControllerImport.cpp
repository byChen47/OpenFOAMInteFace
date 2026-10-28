// Import concern of CaseDocumentController: reading an existing OpenFOAM case
// directory into the project. Kept in its own translation unit because the
// controller file is over the 400-line limit (D-021).
#include "ui/CaseDocumentController.h"

#include "core/ValidationIssue.h"

#include "io/CaseImporter.h"

#include "ui/CaseDocumentControllerPaths.h"

#include <QDir>
#include <QFileInfo>
#include <QStringList>

namespace ofgui {
namespace {

QString importFailureText(const io::ImportReport& report)
{
    QStringList lines;
    for (const core::ValidationIssue& issue : report.issues)
    {
        if (!issue.isBlocking())
        {
            continue;
        }
        if (lines.size() >= 5)
        {
            break;
        }
        lines.append(QString::fromStdString(issue.format()));
    }
    return lines.join(QLatin1Char('\n'));
}

} // namespace

bool CaseDocumentController::openCaseDirectory(const QString& directory)
{
    const QString trimmed = directory.trimmed();
    if (trimmed.isEmpty())
    {
        setError(tr("请选择一个 OpenFOAM 算例目录。"));
        return false;
    }

    const QFileInfo info(trimmed);
    if (!info.exists() || !info.isDir())
    {
        setError(tr("算例目录不存在或不是目录：\n%1").arg(QDir::toNativeSeparators(trimmed)));
        return false;
    }

    const io::CaseImportResult result = io::CaseImporter().import(detail::toPath(trimmed));
    if (!result.ok)
    {
        const QString details = importFailureText(result.report);
        setError(details.isEmpty()
                ? tr("无法导入算例：%1").arg(QDir::toNativeSeparators(trimmed))
                : tr("无法导入算例：\n%1").arg(details));
        return false;
    }

    m_project = result.project;
    if (m_project.name().empty())
    {
        m_project.setName(info.fileName().toStdString());
    }
    m_project.setCaseDirectory(QDir(trimmed).absolutePath().toUtf8().toStdString());
    m_extras = {};
    m_importReport = result.report;
    m_readOnlySettings = result.readOnlySettings;
    m_transport = result.transport;
    m_projectFilePath.clear();
    loadCaseDictionaries(trimmed);
    refreshValidation();
    // The imported model is not written to a `.ofgproj` yet, so closing the
    // application must warn the user before the import result is lost.
    setDirty(true);
    setError(QString());

    emit projectChanged();
    emit importReportChanged();
    emit validationChanged();
    emit documentStateChanged();
    return true;
}

} // namespace ofgui
