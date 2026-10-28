#include "io/ProjectJsonValueReader.h"

#include "io/ProjectJsonSchema.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace ofgui::io {

std::string projectJsonChildPath(const std::string& base, const char* key)
{
    if (base.empty())
    {
        return key;
    }
    return base + "." + key;
}

std::string projectJsonIndexPath(const std::string& base, std::size_t index)
{
    return base + "[" + std::to_string(index) + "]";
}

ProjectJsonValueReader::ProjectJsonValueReader(
    std::string source,
    std::vector<ProjectJsonDiagnostic>& diagnostics)
    : source_(std::move(source))
    , diagnostics_(diagnostics)
{
}

void ProjectJsonValueReader::info(const char* code, std::string path, std::string message)
{
    add(ProjectJsonSeverity::Info, code, std::move(path), std::move(message));
}

void ProjectJsonValueReader::warning(const char* code, std::string path, std::string message)
{
    add(ProjectJsonSeverity::Warning, code, std::move(path), std::move(message));
}

void ProjectJsonValueReader::error(const char* code, std::string path, std::string message)
{
    add(ProjectJsonSeverity::Error, code, std::move(path), std::move(message));
}

void ProjectJsonValueReader::add(
    ProjectJsonSeverity severity,
    const char* code,
    std::string path,
    std::string message)
{
    if (severity == ProjectJsonSeverity::Error)
    {
        hasError_ = true;
    }

    if (diagnostics_.size() >= projectjson::kMaxDiagnostics)
    {
        if (!truncated_)
        {
            truncated_ = true;
            diagnostics_.push_back(ProjectJsonDiagnostic{
                ProjectJsonSeverity::Info,
                projectjson::kDiagnosticsTruncated,
                {},
                "further problems were truncated; fix the first errors and reopen the file",
                source_});
        }
        return;
    }

    diagnostics_.push_back(
        ProjectJsonDiagnostic{severity, code, std::move(path), std::move(message), source_});
}

bool ProjectJsonValueReader::readString(const QJsonValue& value, const std::string& path, std::string& out)
{
    if (!value.isString())
    {
        error(projectjson::kUnexpectedType, path, "expected a string");
        return false;
    }
    out = value.toString().toStdString();
    return true;
}

bool ProjectJsonValueReader::readReal(const QJsonValue& value, const std::string& path, core::Real& out)
{
    if (!value.isDouble())
    {
        error(projectjson::kUnexpectedType, path, "expected a number");
        return false;
    }

    const double number = value.toDouble();
    if (!std::isfinite(number))
    {
        error(projectjson::kInvalidValue, path, "number is not finite");
        return false;
    }

    out = number;
    return true;
}

bool ProjectJsonValueReader::readInt(const QJsonValue& value, const std::string& path, int& out)
{
    if (!value.isDouble())
    {
        error(projectjson::kUnexpectedType, path, "expected an integer");
        return false;
    }

    const double number = value.toDouble();
    if (!std::isfinite(number) || number != std::trunc(number)
        || number < static_cast<double>(std::numeric_limits<int>::min())
        || number > static_cast<double>(std::numeric_limits<int>::max()))
    {
        error(projectjson::kInvalidValue, path, "expected an integer within the 32 bit range");
        return false;
    }

    out = static_cast<int>(number);
    return true;
}

bool ProjectJsonValueReader::readBool(const QJsonValue& value, const std::string& path, bool& out)
{
    if (!value.isBool())
    {
        error(projectjson::kUnexpectedType, path, "expected true or false");
        return false;
    }
    out = value.toBool();
    return true;
}

bool ProjectJsonValueReader::readObject(const QJsonValue& value, const std::string& path, QJsonObject& out)
{
    if (!value.isObject())
    {
        error(projectjson::kUnexpectedType, path, "expected an object");
        return false;
    }
    out = value.toObject();
    return true;
}

bool ProjectJsonValueReader::readArray(const QJsonValue& value, const std::string& path, QJsonArray& out)
{
    if (!value.isArray())
    {
        error(projectjson::kUnexpectedType, path, "expected an array");
        return false;
    }
    out = value.toArray();
    return true;
}

bool ProjectJsonValueReader::readVec3(const QJsonValue& value, const std::string& path, core::Vec3d& out)
{
    std::vector<core::Real> components;
    if (!readRealArray(value, path, 3, components))
    {
        return false;
    }

    out = core::Vec3d{components[0], components[1], components[2]};
    return true;
}

bool ProjectJsonValueReader::readRealArray(
    const QJsonValue& value,
    const std::string& path,
    std::size_t expectedSize,
    std::vector<core::Real>& out)
{
    if (!value.isArray())
    {
        error(projectjson::kUnexpectedType, path, "expected an array of numbers");
        return false;
    }

    const QJsonArray array = value.toArray();
    if (static_cast<std::size_t>(array.size()) != expectedSize)
    {
        error(
            projectjson::kInvalidValue,
            path,
            "expected exactly " + std::to_string(expectedSize) + " numbers, found "
                + std::to_string(array.size()));
        return false;
    }

    std::vector<core::Real> values;
    values.reserve(expectedSize);
    bool converted = true;
    for (int index = 0; index < array.size(); ++index)
    {
        core::Real number = 0.0;
        if (!readReal(array.at(index), projectJsonIndexPath(path, static_cast<std::size_t>(index)), number))
        {
            converted = false;
            continue;
        }
        values.push_back(number);
    }

    if (!converted)
    {
        return false;
    }

    out = std::move(values);
    return true;
}

bool ProjectJsonValueReader::readIntArray(
    const QJsonValue& value,
    const std::string& path,
    std::size_t expectedSize,
    std::vector<int>& out)
{
    if (!value.isArray())
    {
        error(projectjson::kUnexpectedType, path, "expected an array of integers");
        return false;
    }

    const QJsonArray array = value.toArray();
    if (static_cast<std::size_t>(array.size()) != expectedSize)
    {
        error(
            projectjson::kInvalidValue,
            path,
            "expected exactly " + std::to_string(expectedSize) + " integers, found "
                + std::to_string(array.size()));
        return false;
    }

    std::vector<int> values;
    values.reserve(expectedSize);
    bool converted = true;
    for (int index = 0; index < array.size(); ++index)
    {
        int number = 0;
        if (!readInt(array.at(index), projectJsonIndexPath(path, static_cast<std::size_t>(index)), number))
        {
            converted = false;
            continue;
        }
        values.push_back(number);
    }

    if (!converted)
    {
        return false;
    }

    out = std::move(values);
    return true;
}

bool ProjectJsonValueReader::requireKey(const QJsonObject& object, const char* key, const std::string& path)
{
    if (object.contains(key))
    {
        return true;
    }

    error(projectjson::kMissingField, projectJsonChildPath(path, key), "missing required field");
    return false;
}

} // namespace ofgui::io
