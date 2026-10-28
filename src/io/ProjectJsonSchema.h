#pragma once

#include "core/BoundaryPatch.h"
#include "core/DomainBox.h"
#include "core/FaceId.h"
#include "core/FieldDefinition.h"
#include "core/FieldValue.h"
#include "core/ProjectSchemaVersion.h"

#include <QLatin1String>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace ofgui::io {

/// Key names and enumerated spellings of the `.ofgproj` document.
///
/// Every key the reader and the writer share is named here exactly once, so a
/// typo cannot silently create a second spelling of the same field. Keys that are
/// unknown to this build are preserved (see `ProjectJsonExtras`) instead of being
/// dropped.
namespace projectjson {

/// Version of the `.ofgproj` document written by this build.
///
/// `io` owns the project-file format (docs/DECISIONS.md D-034): the version that
/// is written and the range that is accepted are decided here, not by
/// `core::kCurrentSchemaVersion`, which only names the model revision.
///
/// - 1: model sections only (domain, mesh, solver, patches, fields).
/// - 2: adds the `source` section (`core::CaseSourceInfo`, D-025/D-031).
inline constexpr int kCurrentProjectSchemaVersion = 2;

/// Oldest document version this build still reads. Older documents are migrated
/// in memory to @ref kCurrentProjectSchemaVersion; saving always writes the
/// current version.
inline constexpr int kOldestSupportedProjectSchemaVersion = 1;

inline constexpr const char* kSchemaVersion = "schemaVersion";
inline constexpr const char* kProject = "project";
inline constexpr const char* kName = "name";
inline constexpr const char* kCaseDirectory = "caseDirectory";
inline constexpr const char* kDomain = "domain";
inline constexpr const char* kMin = "min";
inline constexpr const char* kMax = "max";
inline constexpr const char* kMode = "mode";
inline constexpr const char* kUnit = "unit";
inline constexpr const char* kMesh = "mesh";
inline constexpr const char* kCells = "cells";
inline constexpr const char* kGrading = "grading";
inline constexpr const char* kScale = "scale";
inline constexpr const char* kSolver = "solver";
inline constexpr const char* kApplication = "application";
inline constexpr const char* kStartTime = "startTime";
inline constexpr const char* kEndTime = "endTime";
inline constexpr const char* kDeltaT = "deltaT";
inline constexpr const char* kWriteInterval = "writeInterval";
inline constexpr const char* kPatches = "patches";
inline constexpr const char* kFields = "fields";
inline constexpr const char* kId = "id";
inline constexpr const char* kType = "type";
inline constexpr const char* kFaces = "faces";
inline constexpr const char* kColor = "color";
inline constexpr const char* kVisible = "visible";
inline constexpr const char* kClass = "class";
inline constexpr const char* kDimensions = "dimensions";
inline constexpr const char* kInternalField = "internalField";
inline constexpr const char* kBoundaryField = "boundaryField";
inline constexpr const char* kKind = "kind";
inline constexpr const char* kValue = "value";
inline constexpr const char* kRaw = "raw";
inline constexpr const char* kPatchId = "patchId";
inline constexpr const char* kParameters = "parameters";

/// `source` section (schemaVersion 2). The key names follow
/// docs/CASE_IMPORT_SPEC.md §6.7 / D-025; the C++ member is
/// `core::CaseSourceInfo::sourceRelativePath` while the JSON key is
/// `caseRelativePath`, matching the specification.
inline constexpr const char* kSource = "source";
inline constexpr const char* kCaseRelativePath = "caseRelativePath";
inline constexpr const char* kStartDirectory = "startDirectory";
inline constexpr const char* kImportedAt = "importedAt";
inline constexpr const char* kFiles = "files";
inline constexpr const char* kRelativePath = "relativePath";
inline constexpr const char* kSha256 = "sha256";

/// Upper bound on diagnostics produced by one load; a corrupt document must not
/// turn into an unbounded list in the import/report UI.
inline constexpr std::size_t kMaxDiagnostics = 200;

/// `internalField.kind` written for a field whose interior value is not set yet
/// (`core::FieldValue::isValid() == false`). Not a `FieldValueKind`, because the
/// model has no "unset" enumerator: the reader leaves the value default.
inline constexpr const char* kUnsetValueKind = "unset";

/// `QLatin1String` view of a key constant, so `QJsonObject::operator[]` /
/// `contains` / `value` never see an ambiguous `const char*` argument.
[[nodiscard]] inline QLatin1String key(const char* name) noexcept
{
    return QLatin1String(name);
}

/// `DomainMode` spelling: `twoD` / `threeD`.
[[nodiscard]] inline std::string_view domainModeName(core::DomainMode mode) noexcept
{
    return mode == core::DomainMode::TwoD ? std::string_view{"twoD"} : std::string_view{"threeD"};
}

[[nodiscard]] inline std::optional<core::DomainMode> domainModeFromName(std::string_view name) noexcept
{
    if (name == "twoD")
    {
        return core::DomainMode::TwoD;
    }
    if (name == "threeD")
    {
        return core::DomainMode::ThreeD;
    }
    return std::nullopt;
}

/// `FieldValueKind` spelling: `uniformScalar` / `uniformVector` / `nonUniformRaw`.
[[nodiscard]] inline std::string_view fieldValueKindName(core::FieldValueKind kind) noexcept
{
    switch (kind)
    {
    case core::FieldValueKind::UniformScalar:
        return "uniformScalar";
    case core::FieldValueKind::UniformVector:
        return "uniformVector";
    case core::FieldValueKind::NonUniformRaw:
        return "nonUniformRaw";
    }
    return "uniformScalar";
}

[[nodiscard]] inline std::optional<core::FieldValueKind> fieldValueKindFromName(std::string_view name) noexcept
{
    if (name == "uniformScalar")
    {
        return core::FieldValueKind::UniformScalar;
    }
    if (name == "uniformVector")
    {
        return core::FieldValueKind::UniformVector;
    }
    if (name == "nonUniformRaw")
    {
        return core::FieldValueKind::NonUniformRaw;
    }
    return std::nullopt;
}

/// Reverse of `core::faceName`: `"xMin"` -> `FaceId::XMin`.
[[nodiscard]] inline std::optional<core::FaceId> faceIdFromName(std::string_view name) noexcept
{
    for (const core::FaceId face : core::kAllFaceIds)
    {
        if (name == core::faceName(face))
        {
            return face;
        }
    }
    return std::nullopt;
}

} // namespace projectjson

} // namespace ofgui::io
