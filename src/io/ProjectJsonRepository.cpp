#include "io/ProjectJsonRepository.h"

#include "io/AtomicFileWriter.h"
#include "io/ProjectJsonMapping.h"
#include "io/ProjectJsonSchema.h"
#include "io/ProjectJsonSource.h"

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace ofgui::io {
namespace {

/// Hard limit for a project file; the format only holds descriptors, so any
/// larger document is a mistake or a corrupted file.
constexpr std::size_t kMaxProjectFileBytes = 32u * 1024u * 1024u;

std::string joinDiagnostics(const std::vector<ProjectJsonDiagnostic>& diagnostics)
{
    std::string text;
    for (const ProjectJsonDiagnostic& diagnostic : diagnostics)
    {
        if (!text.empty())
        {
            text.push_back('\n');
        }
        text += diagnostic.toString();
    }
    return text;
}

void addError(
    std::vector<ProjectJsonDiagnostic>& diagnostics,
    const char* code,
    std::string path,
    std::string message,
    std::string source)
{
    ProjectJsonDiagnostic diagnostic;
    diagnostic.severity = ProjectJsonSeverity::Error;
    diagnostic.code = code;
    diagnostic.path = std::move(path);
    diagnostic.message = std::move(message);
    diagnostic.source = std::move(source);
    diagnostics.push_back(std::move(diagnostic));
}

bool readAllBytes(
    const std::filesystem::path& path,
    std::string& content,
    std::vector<ProjectJsonDiagnostic>& diagnostics)
{
    const std::string source = path.string();

    std::error_code error;
    if (std::filesystem::is_directory(path, error) && !error)
    {
        addError(
            diagnostics,
            projectjson::kCannotOpen,
            {},
            "the project path is a directory",
            source);
        return false;
    }

    std::ifstream stream(path, std::ios::binary);
    if (!stream)
    {
        addError(
            diagnostics,
            projectjson::kCannotOpen,
            {},
            "cannot open the project file for reading",
            source);
        return false;
    }

    content.assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
    if (stream.bad())
    {
        addError(
            diagnostics,
            projectjson::kReadFailed,
            {},
            "failed while reading the project file",
            source);
        return false;
    }

    if (content.size() > kMaxProjectFileBytes)
    {
        addError(
            diagnostics,
            projectjson::kReadFailed,
            {},
            "the project file exceeds the 32 MiB size limit",
            source);
        return false;
    }
    return true;
}

void stripByteOrderMark(std::string& content, const std::string& source,
                        std::vector<ProjectJsonDiagnostic>& diagnostics)
{
    static constexpr std::string_view kUtf8Bom = "\xEF\xBB\xBF";
    if (content.size() < kUtf8Bom.size()
        || std::string_view(content.data(), kUtf8Bom.size()) != kUtf8Bom)
    {
        return;
    }

    content.erase(0, kUtf8Bom.size());
    ProjectJsonDiagnostic diagnostic;
    diagnostic.severity = ProjectJsonSeverity::Info;
    diagnostic.code = projectjson::kBomRemoved;
    diagnostic.message = "the UTF-8 byte order mark was removed";
    diagnostic.source = source;
    diagnostics.push_back(std::move(diagnostic));
}

} // namespace

std::string ProjectLoadResult::diagnosticsText() const
{
    return joinDiagnostics(diagnostics);
}

std::string ProjectSaveResult::diagnosticsText() const
{
    return joinDiagnostics(diagnostics);
}

ProjectLoadResult ProjectJsonRepository::load(const std::filesystem::path& path)
{
    ProjectLoadResult result;
    if (path.empty())
    {
        addError(
            result.diagnostics,
            projectjson::kCannotOpen,
            {},
            "no project file path was given",
            {});
        return result;
    }

    const std::string source = path.string();
    std::string content;
    if (!readAllBytes(path, content, result.diagnostics))
    {
        return result;
    }
    stripByteOrderMark(content, source, result.diagnostics);

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(
        QByteArray(content.data(), static_cast<qsizetype>(content.size())),
        &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        addError(
            result.diagnostics,
            projectjson::kMalformedJson,
            {},
            "cannot parse the project JSON: " + parseError.errorString().toStdString()
                + " at byte offset " + std::to_string(parseError.offset),
            source);
        return result;
    }
    if (!document.isObject())
    {
        addError(
            result.diagnostics,
            projectjson::kRootNotObject,
            {},
            "the project file root must be a JSON object",
            source);
        return result;
    }

    const QJsonObject root = document.object();
    if (!root.contains(projectjson::key(projectjson::kSchemaVersion)))
    {
        addError(
            result.diagnostics,
            projectjson::kMissingSchemaVersion,
            projectjson::kSchemaVersion,
            "the project file has no 'schemaVersion' field",
            source);
        return result;
    }

    const QJsonValue versionValue = root.value(projectjson::key(projectjson::kSchemaVersion));
    const double versionNumber = versionValue.toDouble(-1.0);
    if (!versionValue.isDouble() || versionNumber != static_cast<double>(static_cast<int>(versionNumber)))
    {
        addError(
            result.diagnostics,
            projectjson::kInvalidSchemaVersion,
            projectjson::kSchemaVersion,
            "'schemaVersion' must be an integer number",
            source);
        return result;
    }
    const int version = static_cast<int>(versionNumber);
    result.schemaVersion = version;
    if (version < projectjson::kOldestSupportedProjectSchemaVersion
        || version > projectjson::kCurrentProjectSchemaVersion)
    {
        addError(
            result.diagnostics,
            projectjson::kUnsupportedSchemaVersion,
            projectjson::kSchemaVersion,
            "schema version " + std::to_string(version) + " is not supported by this build (supported: "
                + std::to_string(projectjson::kOldestSupportedProjectSchemaVersion) + " to "
                + std::to_string(projectjson::kCurrentProjectSchemaVersion)
                + "); the file was left unchanged",
            source);
        return result;
    }
    result.migrated = version < projectjson::kCurrentProjectSchemaVersion;

    const bool converted = projectJsonToModel(
        root,
        result.project,
        result.extras,
        result.diagnostics,
        source,
        version);
    result.ok = converted && !projectJsonHasError(result.diagnostics);
    if (!result.ok)
    {
        // A partially populated model must not be used; the caller shows the
        // diagnostics and keeps the previous session instead.
        result.project = core::CaseProject{};
        result.extras = ProjectJsonExtras{};
        result.migrated = false;
        return result;
    }

    if (result.migrated)
    {
        ProjectJsonDiagnostic diagnostic;
        diagnostic.severity = ProjectJsonSeverity::Info;
        diagnostic.code = projectjson::kMigratedSchemaVersion;
        diagnostic.path = projectjson::kSchemaVersion;
        diagnostic.message = "the project was written with schema version " + std::to_string(version)
                             + " and was migrated in memory; saving writes schema version "
                             + std::to_string(projectjson::kCurrentProjectSchemaVersion);
        diagnostic.source = source;
        result.diagnostics.push_back(std::move(diagnostic));
    }
    return result;
}

std::string ProjectJsonRepository::toJsonDocument(
    const core::CaseProject& project,
    const ProjectJsonExtras& extras,
    std::vector<ProjectJsonDiagnostic>& diagnostics)
{
    const QJsonObject root = projectJsonFromModel(project, extras, diagnostics);
    const QByteArray bytes = QJsonDocument(root).toJson(QJsonDocument::Indented);
    return std::string(bytes.constData(), static_cast<std::size_t>(bytes.size()));
}

ProjectSaveResult ProjectJsonRepository::save(
    const std::filesystem::path& path,
    const core::CaseProject& project,
    const ProjectJsonExtras& extras)
{
    ProjectSaveResult result;
    if (path.empty())
    {
        addError(
            result.diagnostics,
            projectjson::kTargetNotWritable,
            {},
            "no project file path was given",
            {});
        return result;
    }

    const std::string source = path.string();
    const std::string text = toJsonDocument(project, extras, result.diagnostics);

    const AtomicWriteResult written = AtomicFileWriter(path).write(text);
    switch (written.status)
    {
    case AtomicWriteStatus::Success:
        break;
    case AtomicWriteStatus::TargetNotWritable:
        addError(
            result.diagnostics,
            projectjson::kTargetNotWritable,
            {},
            written.message.toStdString(),
            source);
        break;
    case AtomicWriteStatus::WriteFailed:
        addError(
            result.diagnostics,
            projectjson::kWriteFailed,
            {},
            written.message.toStdString(),
            source);
        break;
    }

    result.ok = !projectJsonHasError(result.diagnostics);
    return result;
}

} // namespace ofgui::io
