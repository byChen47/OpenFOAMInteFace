#include "ui/ExportDictionaryCheck.h"

#include "io/FoamDictionary.h"
#include "io/FoamDiagnostic.h"

#include <QCoreApplication>

#include <array>
#include <string>
#include <string_view>

namespace ofgui {
namespace {

QString tr(const char* text)
{
    return QCoreApplication::translate("ExportDictionaryCheck", text);
}

void addIssue(
    std::vector<core::ValidationIssue>& issues,
    core::Severity severity,
    const char* code,
    const QString& message,
    const QString& path)
{
    core::ValidationIssue issue(severity, code, message.toStdString());
    issue.setTargetField(path.toStdString());
    issue.setSuggestedFix(tr("修正模型中的对应值后重新导出。").toStdString());
    issues.push_back(std::move(issue));
}

/// Files every exported case needs; a case without them cannot run.
constexpr std::array<const char*, 4> kRequiredFiles{
    "system/blockMeshDict",
    "system/controlDict",
    "system/fvSchemes",
    "system/fvSolution",
};

} // namespace

std::vector<core::ValidationIssue> checkExportPlan(
    const io::CaseExportPlan& plan,
    const core::CaseProject& project)
{
    std::vector<core::ValidationIssue> issues;

    for (const char* required : kRequiredFiles)
    {
        const io::ExportFile* file = plan.find(required);
        if (file == nullptr || (!file->isCopy() && file->text.empty()))
        {
            addIssue(
                issues,
                core::Severity::BlockingError,
                kExportFileMissing,
                tr("导出计划缺少必需文件：%1。").arg(QString::fromLatin1(required)),
                QString::fromLatin1(required));
        }
    }

    for (const core::FieldDefinition& field : project.fields())
    {
        const std::string relativePath = std::string{"0/"} + field.name();
        const io::ExportFile* file = plan.find(relativePath);
        if (file == nullptr || (!file->isCopy() && file->text.empty()))
        {
            addIssue(
                issues,
                core::Severity::BlockingError,
                kExportFileMissing,
                tr("导出计划缺少场文件：%1。").arg(QString::fromStdString(relativePath)),
                QString::fromStdString(relativePath));
        }
    }

    for (const io::ExportFile& file : plan.files)
    {
        if (file.isCopy() || file.text.empty())
        {
            continue;
        }
        const io::FoamDictionary dictionary =
            io::FoamDictionary::parse(file.text, file.relativePath);
        if (dictionary.ok())
        {
            continue;
        }

        QString detail;
        for (const io::FoamDiagnostic& diagnostic : dictionary.diagnostics())
        {
            if (diagnostic.severity != io::FoamSeverity::Error)
            {
                continue;
            }
            detail = QString::fromStdString(diagnostic.message);
            break;
        }
        if (detail.isEmpty())
        {
            detail = tr("无法解析生成的字典文本。");
        }
        addIssue(
            issues,
            core::Severity::BlockingError,
            kExportDictionaryInvalid,
            tr("生成的 %1 不是有效的 OpenFOAM 字典：%2")
                .arg(QString::fromStdString(file.relativePath), detail),
            QString::fromStdString(file.relativePath));
    }

    return issues;
}

} // namespace ofgui
