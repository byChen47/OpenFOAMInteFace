#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace ofgui::io {

/// Severity of one `.ofgproj` load/save message.
///
/// Mirrors `FoamSeverity` on purpose: the import report shows OpenFOAM parse
/// problems and project problems in one list, so the UI can map both enums to a
/// single icon set without special cases.
enum class ProjectJsonSeverity
{
    Info,
    Warning,
    Error
};

/// One problem found while reading or writing a project file.
///
/// `path` is the JSON location of the problem in dotted/bracketed form
/// (`fields[2].boundaryField[0].type`); it is empty for whole-document problems.
/// `code` is stable (`PROJ-*`) so tests and the UI can match on it.
struct ProjectJsonDiagnostic
{
    ProjectJsonSeverity severity{ProjectJsonSeverity::Error};
    std::string code;
    std::string path;
    std::string message;
    std::string source;

    /// `source: path: error [PROJ-..] message` with empty parts omitted.
    [[nodiscard]] std::string toString() const;
};

/// `info` / `warning` / `error`.
[[nodiscard]] std::string_view projectJsonSeverityName(ProjectJsonSeverity severity) noexcept;

/// True when any diagnostic is an `Error`.
[[nodiscard]] bool projectJsonHasError(const std::vector<ProjectJsonDiagnostic>& diagnostics) noexcept;

/// Number of diagnostics with the given severity.
[[nodiscard]] std::size_t projectJsonCount(
    const std::vector<ProjectJsonDiagnostic>& diagnostics,
    ProjectJsonSeverity severity) noexcept;

/// Stable diagnostic codes. Adding a code requires updating the docs table in
/// `docs/DECISIONS.md` (see D-030) and a test that produces it.
namespace projectjson {

inline constexpr const char* kCannotOpen = "PROJ-IO-001";
inline constexpr const char* kReadFailed = "PROJ-IO-002";
inline constexpr const char* kTargetNotWritable = "PROJ-IO-003";
inline constexpr const char* kWriteFailed = "PROJ-IO-004";
inline constexpr const char* kBomRemoved = "PROJ-ENC-001";
inline constexpr const char* kMalformedJson = "PROJ-JSON-001";
inline constexpr const char* kRootNotObject = "PROJ-JSON-002";
inline constexpr const char* kMissingSchemaVersion = "PROJ-SCHEMA-001";
inline constexpr const char* kUnsupportedSchemaVersion = "PROJ-SCHEMA-002";
inline constexpr const char* kInvalidSchemaVersion = "PROJ-SCHEMA-003";
inline constexpr const char* kMissingField = "PROJ-MODEL-001";
inline constexpr const char* kUnexpectedType = "PROJ-MODEL-002";
inline constexpr const char* kInvalidValue = "PROJ-MODEL-003";
inline constexpr const char* kDuplicateId = "PROJ-MODEL-004";
inline constexpr const char* kUnknownEnumWord = "PROJ-MODEL-005";
inline constexpr const char* kUnknownPatchReference = "PROJ-MODEL-006";
inline constexpr const char* kPreservedUnknownFields = "PROJ-MODEL-007";
/// `source.files[].sha256` is not exactly 64 lower-case hex digits.
inline constexpr const char* kInvalidSourceHash = "PROJ-MODEL-008";
/// `source.files` lists the same `relativePath` twice (first entry wins).
inline constexpr const char* kDuplicateSourceFile = "PROJ-MODEL-009";
inline constexpr const char* kAbsoluteCaseDirectory = "PROJ-PATH-001";
/// `source.caseRelativePath` / `source.files[].relativePath` is a rooted path.
///
/// Error while reading (D-025 forbids absolute paths in a project file),
/// warning while writing (the value is preserved verbatim, never dropped).
inline constexpr const char* kAbsoluteSourcePath = "PROJ-PATH-002";
/// `source.caseRelativePath` is empty but the section still lists files, so the
/// file list is ignored: an empty case path is the documented spelling of "this
/// project has no source" (CASE_IMPORT_SPEC §6.7).
inline constexpr const char* kIgnoredSourceKeys = "PROJ-MODEL-010";
/// The document was older than @ref kCurrentProjectSchemaVersion and was
/// migrated in memory; the next save writes the current version.
inline constexpr const char* kMigratedSchemaVersion = "PROJ-SCHEMA-004";
inline constexpr const char* kDiagnosticsTruncated = "PROJ-REPORT-001";

} // namespace projectjson

} // namespace ofgui::io
