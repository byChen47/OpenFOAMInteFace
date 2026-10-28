#include "ui/ExportController.h"

#include "ui/CaseDocumentController.h"
#include "ui/ExportDictionaryCheck.h"

#include "core/ValidationResult.h"
#include "io/ImportCodes.h"
#include "io/writers/ExportManifestSchema.h"
#include "io/writers/ExportPlanBuilder.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

#include <algorithm>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace ofgui {
namespace {

std::filesystem::path toPath(const QString& text)
{
    return std::filesystem::path(text.toStdU16String());
}

QString severityText(core::Severity severity)
{
    switch (severity)
    {
    case core::Severity::Info:
        return QCoreApplication::translate("ExportController", "提示");
    case core::Severity::Warning:
        return QCoreApplication::translate("ExportController", "警告");
    case core::Severity::Error:
        return QCoreApplication::translate("ExportController", "错误");
    case core::Severity::BlockingError:
        return QCoreApplication::translate("ExportController", "阻断");
    }
    return QCoreApplication::translate("ExportController", "提示");
}

/// True when an existing file or subdirectory would be replaced.
bool directoryHasContent(const QString& target)
{
    if (target.trimmed().isEmpty())
    {
        return false;
    }
    const QDir directory(target);
    if (!directory.exists())
    {
        return false;
    }
    const QFileInfoList entries =
        directory.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);
    return !entries.isEmpty();
}

} // namespace

ExportController::ExportController(CaseDocumentController* document, QObject* parent)
    : QObject(parent)
    , m_document(document)
{
}

CaseDocumentController* ExportController::document() const noexcept
{
    return m_document;
}

io::CaseExportOptions ExportController::makeOptions(
    const QString& targetDirectory,
    bool allowExistingTarget) const
{
    io::CaseExportOptions options;
    options.targetDirectory = toPath(targetDirectory);
    options.startDirectory = "0";
    options.sourceCaseRoot = m_document != nullptr ? m_document->caseDirectory().toStdString()
                                                   : std::string{};
    options.allowExistingTarget = allowExistingTarget;
    options.writePlaceholderDictionaries = true;
    options.mirrorSourceFiles = !options.sourceCaseRoot.empty();
    options.keepRunScripts = false;
    options.mirrorOtherTimeDirectories = false;

    // D-037: the solver editor's transport object is the one the exporter writes.
    if (m_document != nullptr)
    {
        options.transport = m_document->transportSettings();
        options.fvSchemesText = m_document->sourceDictionaryText(QStringLiteral("system/fvSchemes")).toStdString();
        options.fvSolutionText = m_document->sourceDictionaryText(QStringLiteral("system/fvSolution")).toStdString();
    }
    return options;
}

ExportPreview ExportController::preview(const QString& targetDirectory) const
{
    ExportPreview preview;
    if (m_document == nullptr)
    {
        preview.errorText = tr("尚未打开工程，无法导出。");
        return preview;
    }

    preview.targetHasContent = directoryHasContent(targetDirectory);

    std::vector<core::ValidationIssue> issues;
    // Model validation is part of the preview: a blocking model problem must be
    // visible (and disable the export button) before the user presses 导出.
    const core::ValidationResult& validation = m_document->validation();
    issues.insert(issues.end(), validation.issues().begin(), validation.issues().end());

    preview.plan = io::ExportPlanBuilder::build(
        m_document->project(),
        makeOptions(targetDirectory, false),
        issues);

    const std::vector<core::ValidationIssue> dictionaryIssues =
        checkExportPlan(preview.plan, m_document->project());
    issues.insert(issues.end(), dictionaryIssues.begin(), dictionaryIssues.end());
    preview.issues = std::move(issues);

    const bool hasBlocking = std::any_of(
        preview.issues.begin(), preview.issues.end(), [](const core::ValidationIssue& issue) {
            return issue.severity() == core::Severity::Error
                   || issue.severity() == core::Severity::BlockingError;
        });
    preview.ok = !preview.plan.empty() && !hasBlocking;
    if (!preview.ok)
    {
        preview.errorText = failureText(preview.issues);
    }
    return preview;
}

ExportOutcome ExportController::exportTo(
    const QString& targetDirectory,
    bool allowExistingTarget)
{
    ExportOutcome outcome;
    if (m_document == nullptr)
    {
        outcome.message = tr("尚未打开工程，无法导出。");
        return outcome;
    }

    const QString target = targetDirectory.trimmed();
    if (target.isEmpty())
    {
        outcome.message = tr("请选择导出目标目录。");
        return outcome;
    }
    if (!m_document->caseDirectory().isEmpty())
    {
        const QString sourceRoot = QDir::cleanPath(QDir(m_document->caseDirectory()).absolutePath());
        const QString absoluteTarget = QDir::cleanPath(QDir(target).absolutePath());
        if (absoluteTarget == sourceRoot
            || absoluteTarget.startsWith(sourceRoot + QLatin1Char('/')))
        {
            // Same guard as io::CaseExporter, but reported before any planning so
            // the message is actionable in the dialog.
            outcome.message = tr("导出目标不能位于源算例目录内：%1")
                                  .arg(QDir::toNativeSeparators(target));
            return outcome;
        }
    }

    emit progressMessage(tr("正在校验工程模型…"));
    const core::ValidationResult& validation = m_document->validation();
    if (validation.hasErrors() || validation.hasBlockingErrors())
    {
        outcome.issues = validation.issues();
        outcome.message = tr("导出已阻止：工程存在错误或阻断问题（%1），请先在校验问题面板修复。")
                              .arg(QString::fromStdString(validation.summary()));
        return outcome;
    }

    emit progressMessage(tr("正在生成导出计划并校验字典…"));
    const ExportPreview planned = preview(target);
    if (!planned.ok)
    {
        outcome.issues = planned.issues;
        outcome.message = planned.errorText;
        return outcome;
    }

    emit progressMessage(tr("正在写入目标目录…"));
    io::CaseExporter exporter;
    outcome.result = exporter.exportProject(
        m_document->project(),
        makeOptions(target, allowExistingTarget));
    outcome.issues = outcome.result.issues;
    outcome.backupDirectory = QString::fromStdString(outcome.result.backupDirectory.string());

    if (!outcome.result.ok)
    {
        outcome.message = failureText(outcome.issues);
        return outcome;
    }

    // Read the manifest the exporter just wrote: the user sees exactly what the
    // file system received, not what the plan predicted.
    const QString manifestPath = QDir(target).filePath(
        QString::fromLatin1(io::exportmanifest::kFileName));
    QFile manifestFile(manifestPath);
    if (manifestFile.open(QIODevice::ReadOnly))
    {
        const QJsonDocument document = QJsonDocument::fromJson(manifestFile.readAll());
        if (document.isObject())
        {
            outcome.manifestFiles = document.object()
                                        .value(QLatin1String{io::exportmanifest::kFilesKey})
                                        .toArray()
                                        .size();
            outcome.manifestVerified = true;
        }
    }

    outcome.ok = true;
    outcome.message = tr("导出完成：新写 %1、复制 %2、镜像 %3、跳过 %4，共 %5 个文件。")
                          .arg(static_cast<qulonglong>(outcome.result.writtenCount))
                          .arg(static_cast<qulonglong>(outcome.result.copiedCount))
                          .arg(static_cast<qulonglong>(outcome.result.mirroredCount))
                          .arg(static_cast<qulonglong>(outcome.result.skippedCount))
                          .arg(static_cast<qulonglong>(outcome.result.fileCount));
    emit progressMessage(outcome.message);
    return outcome;
}

QString ExportController::failureText(const std::vector<core::ValidationIssue>& issues) const
{
    QStringList lines;
    for (const core::ValidationIssue& issue : issues)
    {
        if (issue.severity() == core::Severity::Info)
        {
            continue;
        }
        if (lines.size() >= 6)
        {
            lines.append(tr("…（其余问题见校验问题面板）"));
            break;
        }
        lines.append(tr("[%1] %2：%3")
                         .arg(severityText(issue.severity()),
                              QString::fromStdString(issue.code()),
                              QString::fromStdString(issue.message())));
    }
    if (lines.isEmpty())
    {
        return tr("导出失败，未获得具体错误信息。");
    }
    return lines.join(QLatin1Char('\n'));
}

} // namespace ofgui
