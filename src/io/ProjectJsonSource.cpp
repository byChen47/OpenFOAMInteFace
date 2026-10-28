#include "io/ProjectJsonSource.h"

#include "io/ProjectJsonSchema.h"
#include "io/ProjectJsonValueReader.h"

#include <QJsonArray>
#include <QJsonValue>
#include <QString>

#include <cstddef>
#include <filesystem>
#include <set>
#include <string>
#include <utility>

namespace ofgui::io {
namespace {

using projectjson::key;

QString toQString(std::string_view text)
{
    return QString::fromUtf8(text.data(), static_cast<qsizetype>(text.size()));
}

void appendWarning(
    std::vector<ProjectJsonDiagnostic>& diagnostics,
    const char* code,
    std::string path,
    std::string message)
{
    if (diagnostics.size() >= projectjson::kMaxDiagnostics)
    {
        return;
    }
    diagnostics.push_back(
        ProjectJsonDiagnostic{ProjectJsonSeverity::Warning, code, std::move(path), std::move(message), {}});
}

/// Shared by the reader and the writer: one warning/error text for a rooted path.
std::string absolutePathMessage(std::string_view field, std::string_view path)
{
    return "'" + std::string(field) + "' must be relative to the project file, but '" + std::string(path)
           + "' is absolute (see D-025)";
}

} // namespace

bool projectJsonPathIsAbsolute(std::string_view path)
{
    if (path.empty())
    {
        return false;
    }
    const std::filesystem::path candidate{std::string(path)};
    return candidate.is_absolute() || candidate.has_root_directory() || candidate.has_root_name();
}

bool projectJsonIsLowerHexSha256(std::string_view text)
{
    if (text.size() != core::kSha256HexLength)
    {
        return false;
    }
    for (const char character : text)
    {
        const bool digit = character >= '0' && character <= '9';
        const bool lowerHexLetter = character >= 'a' && character <= 'f';
        if (!digit && !lowerHexLetter)
        {
            return false;
        }
    }
    return true;
}

QJsonObject projectJsonSourceToJson(
    const core::CaseSourceInfo& source,
    const ProjectJsonExtras& extras,
    std::vector<ProjectJsonDiagnostic>& diagnostics)
{
    QJsonObject object = extras.source;

    object[key(projectjson::kCaseRelativePath)] = toQString(source.sourceRelativePath);
    object[key(projectjson::kStartDirectory)] = toQString(source.startDirectory);
    object[key(projectjson::kImportedAt)] = toQString(source.importedAt);

    if (projectJsonPathIsAbsolute(source.sourceRelativePath))
    {
        appendWarning(
            diagnostics,
            projectjson::kAbsoluteSourcePath,
            projectjson::kSource + std::string(".") + projectjson::kCaseRelativePath,
            absolutePathMessage(projectjson::kCaseRelativePath, source.sourceRelativePath));
    }

    QJsonArray files;
    std::set<std::string> written;
    for (const core::SourceFileHash& file : source.files)
    {
        if (!written.insert(file.relativePath).second)
        {
            appendWarning(
                diagnostics,
                projectjson::kDuplicateSourceFile,
                projectjson::kSource + std::string(".") + projectjson::kFiles,
                "duplicate source file '" + file.relativePath + "'; the first entry was kept");
            continue;
        }
        if (projectJsonPathIsAbsolute(file.relativePath))
        {
            appendWarning(
                diagnostics,
                projectjson::kAbsoluteSourcePath,
                projectjson::kSource + std::string(".") + projectjson::kFiles,
                absolutePathMessage(projectjson::kRelativePath, file.relativePath));
        }

        QJsonObject entry = extras.sourceFiles.value(toQString(file.relativePath));
        entry[key(projectjson::kRelativePath)] = toQString(file.relativePath);
        entry[key(projectjson::kSha256)] = toQString(file.sha256Hex());
        files.append(entry);
    }
    object[key(projectjson::kFiles)] = files;

    return object;
}

void projectJsonSourceFromJson(
    const QJsonObject& root,
    core::CaseSourceInfo& source,
    ProjectJsonValueReader& reader)
{
    if (!root.contains(key(projectjson::kSource)))
    {
        return;
    }

    const std::string path = projectjson::kSource;
    QJsonObject object;
    if (!reader.readObject(root.value(key(projectjson::kSource)), path, object))
    {
        return;
    }

    if (object.contains(key(projectjson::kCaseRelativePath)))
    {
        const std::string fieldPath = projectJsonChildPath(path, projectjson::kCaseRelativePath);
        std::string relativePath;
        if (reader.readString(object.value(key(projectjson::kCaseRelativePath)), fieldPath, relativePath))
        {
            if (relativePath.empty())
            {
                // An empty case path means "no source"; this is the documented
                // spelling for a project that was not imported from a case. The
                // remaining keys of the section then carry no meaning, so the
                // whole section is ignored instead of resurrecting a source the
                // document says it does not have.
                source = core::CaseSourceInfo{};
                const std::string filesPath = projectJsonChildPath(path, projectjson::kFiles);
                if (object.contains(key(projectjson::kFiles))
                    && !object.value(key(projectjson::kFiles)).toArray().isEmpty())
                {
                    reader.warning(
                        projectjson::kIgnoredSourceKeys,
                        filesPath,
                        "the section has an empty 'caseRelativePath', so its file list is ignored");
                }
                return;
            }
            if (projectJsonPathIsAbsolute(relativePath))
            {
                reader.error(
                    projectjson::kAbsoluteSourcePath,
                    fieldPath,
                    absolutePathMessage(projectjson::kCaseRelativePath, relativePath));
            }
            else
            {
                source.sourceRelativePath = std::move(relativePath);
            }
        }
    }

    const auto readOptionalString = [&reader, &object, &path](const char* name, std::string& target)
    {
        if (!object.contains(key(name)))
        {
            return;
        }
        std::string text;
        if (reader.readString(object.value(key(name)), projectJsonChildPath(path, name), text))
        {
            target = std::move(text);
        }
    };
    readOptionalString(projectjson::kStartDirectory, source.startDirectory);
    readOptionalString(projectjson::kImportedAt, source.importedAt);

    if (!object.contains(key(projectjson::kFiles)))
    {
        return;
    }

    const std::string filesPath = projectJsonChildPath(path, projectjson::kFiles);
    QJsonArray files;
    if (!reader.readArray(object.value(key(projectjson::kFiles)), filesPath, files))
    {
        return;
    }

    std::set<std::string> seen;
    for (std::size_t index = 0; index < static_cast<std::size_t>(files.size()); ++index)
    {
        const std::string entryPath = projectJsonIndexPath(filesPath, index);
        QJsonObject entry;
        if (!reader.readObject(files.at(static_cast<qsizetype>(index)), entryPath, entry))
        {
            continue;
        }
        if (!reader.requireKey(entry, projectjson::kRelativePath, entryPath))
        {
            continue;
        }

        const std::string relativePathField = projectJsonChildPath(entryPath, projectjson::kRelativePath);
        std::string relativePath;
        if (!reader.readString(entry.value(key(projectjson::kRelativePath)), relativePathField, relativePath))
        {
            continue;
        }
        if (relativePath.empty())
        {
            reader.error(
                projectjson::kInvalidValue,
                relativePathField,
                "'relativePath' must not be empty");
            continue;
        }
        if (projectJsonPathIsAbsolute(relativePath))
        {
            reader.error(
                projectjson::kAbsoluteSourcePath,
                relativePathField,
                absolutePathMessage(projectjson::kRelativePath, relativePath));
            continue;
        }
        if (!seen.insert(relativePath).second)
        {
            reader.warning(
                projectjson::kDuplicateSourceFile,
                relativePathField,
                "duplicate source file '" + relativePath + "'; the first entry was kept");
            continue;
        }

        if (!reader.requireKey(entry, projectjson::kSha256, entryPath))
        {
            continue;
        }
        const std::string hashPath = projectJsonChildPath(entryPath, projectjson::kSha256);
        std::string hex;
        if (!reader.readString(entry.value(key(projectjson::kSha256)), hashPath, hex))
        {
            continue;
        }
        if (!projectJsonIsLowerHexSha256(hex))
        {
            reader.error(
                projectjson::kInvalidSourceHash,
                hashPath,
                "'sha256' must be 64 lower-case hex digits, found '" + hex + "'");
            continue;
        }

        core::SourceFileHash record;
        record.relativePath = std::move(relativePath);
        if (!core::sha256FromHex(hex, record.sha256))
        {
            // Unreachable: the predicate above accepts exactly what fromHex parses.
            reader.error(
                projectjson::kInvalidSourceHash,
                hashPath,
                "'sha256' could not be decoded: '" + hex + "'");
            continue;
        }
        source.files.push_back(std::move(record));
    }
}

} // namespace ofgui::io
