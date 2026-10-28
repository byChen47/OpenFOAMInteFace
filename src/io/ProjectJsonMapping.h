#pragma once

#include "io/ProjectJsonDiagnostic.h"
#include "io/ProjectJsonExtras.h"
#include "io/ProjectJsonSchema.h"

#include <QJsonObject>

#include <string>
#include <vector>

namespace ofgui::core {
class CaseProject;
}

namespace ofgui::io {

/// Serialises the model into the `.ofgproj` document (`schemaVersion` 2).
///
/// Unknown keys from `extras` are written back first and then overwritten with
/// the model values, so a document written by a newer build keeps its extra
/// data when an older build saves it. Problems that do not block writing (for
/// example an absolute `caseDirectory`) are appended as warnings.
[[nodiscard]] QJsonObject projectJsonFromModel(
    const core::CaseProject& project,
    const ProjectJsonExtras& extras,
    std::vector<ProjectJsonDiagnostic>& diagnostics);

/// Rebuilds the model from a `.ofgproj` document.
///
/// @param root parsed document; the caller has already checked
///        `schemaVersion`.
/// @param schemaVersion version read from the document. Sections introduced
///        after that version are not read: a schemaVersion 1 document has no
///        `source` section and therefore loads as a project without a source.
///        Upgrading the document in memory is the caller's decision; the next
///        save always writes @ref projectjson::kCurrentProjectSchemaVersion.
/// @param project receives the model; untouched when `false` is returned.
/// @param extras receives the unknown fields found in the document.
/// @return false when an `Error` diagnostic was produced; the caller must then
///         discard `project` (it may hold a partially populated model).
[[nodiscard]] bool projectJsonToModel(
    const QJsonObject& root,
    core::CaseProject& project,
    ProjectJsonExtras& extras,
    std::vector<ProjectJsonDiagnostic>& diagnostics,
    const std::string& source,
    int schemaVersion = projectjson::kCurrentProjectSchemaVersion);

} // namespace ofgui::io
