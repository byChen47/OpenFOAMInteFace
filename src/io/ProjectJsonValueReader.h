#pragma once

#include "core/Basics.h"
#include "core/Vec3d.h"
#include "io/ProjectJsonDiagnostic.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace ofgui::io {

/// Appends `.key` to a JSON path (`"fields[0]"` + `"id"` -> `"fields[0].id"`).
[[nodiscard]] std::string projectJsonChildPath(const std::string& base, const char* key);

/// Appends `[index]` to a JSON path (`"patches"` + `2` -> `"patches[2]"`).
[[nodiscard]] std::string projectJsonIndexPath(const std::string& base, std::size_t index);

/// Typed access to the values of one `.ofgproj` document.
///
/// The reader owns the "wrong type / missing key" diagnostics: every accessor
/// either converts and returns `true`, or reports one `PROJ-MODEL-*` error with
/// the JSON path and returns `false`. Callers therefore never need to inspect
/// `QJsonValue` kinds themselves, and every message points at a concrete path.
class ProjectJsonValueReader
{
public:
    ProjectJsonValueReader(std::string source, std::vector<ProjectJsonDiagnostic>& diagnostics);

    void info(const char* code, std::string path, std::string message);
    void warning(const char* code, std::string path, std::string message);
    void error(const char* code, std::string path, std::string message);

    [[nodiscard]] bool hasError() const noexcept
    {
        return hasError_;
    }

    [[nodiscard]] bool diagnosticsTruncated() const noexcept
    {
        return truncated_;
    }

    [[nodiscard]] bool readString(const QJsonValue& value, const std::string& path, std::string& out);
    [[nodiscard]] bool readReal(const QJsonValue& value, const std::string& path, core::Real& out);
    [[nodiscard]] bool readInt(const QJsonValue& value, const std::string& path, int& out);
    [[nodiscard]] bool readBool(const QJsonValue& value, const std::string& path, bool& out);
    [[nodiscard]] bool readObject(const QJsonValue& value, const std::string& path, QJsonObject& out);
    [[nodiscard]] bool readArray(const QJsonValue& value, const std::string& path, QJsonArray& out);

    [[nodiscard]] bool readVec3(const QJsonValue& value, const std::string& path, core::Vec3d& out);

    [[nodiscard]] bool readRealArray(
        const QJsonValue& value,
        const std::string& path,
        std::size_t expectedSize,
        std::vector<core::Real>& out);

    [[nodiscard]] bool readIntArray(
        const QJsonValue& value,
        const std::string& path,
        std::size_t expectedSize,
        std::vector<int>& out);

    /// @return true when `object` contains `key`; otherwise reports
    /// `PROJ-MODEL-001` ("missing required field") and returns false.
    [[nodiscard]] bool requireKey(const QJsonObject& object, const char* key, const std::string& path);

private:
    void add(ProjectJsonSeverity severity, const char* code, std::string path, std::string message);

    std::string source_;
    std::vector<ProjectJsonDiagnostic>& diagnostics_;
    bool hasError_{false};
    bool truncated_{false};
};

} // namespace ofgui::io
