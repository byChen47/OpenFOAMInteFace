#pragma once

#include "core/CaseSourceInfo.h"
#include "io/ProjectJsonDiagnostic.h"
#include "io/ProjectJsonExtras.h"

#include <QJsonObject>

#include <string_view>
#include <vector>

namespace ofgui::io {

class ProjectJsonValueReader;

/// True when `path` is rooted on any platform (`<drive>:/case`, `/case`,
/// `\\server\case`).
///
/// A `.ofgproj` must only store case-relative paths (D-025), so both the reader
/// and the writer consult this predicate instead of inspecting the string.
[[nodiscard]] bool projectJsonPathIsAbsolute(std::string_view path);

/// True when `text` is exactly 64 characters of lower-case hexadecimal.
///
/// Upper case is rejected on purpose: `core::sha256ToHex` writes lower case, so a
/// file that differs can only have been edited by hand and is worth reporting.
[[nodiscard]] bool projectJsonIsLowerHexSha256(std::string_view text);

/// Builds the `source` section of a schemaVersion 2 document.
///
/// The section is always written, even for a brand new project (empty strings and
/// an empty `files` array): a reader then never has to distinguish "missing" from
/// "empty", and the byte layout of a saved document does not depend on whether the
/// project happens to have a source (docs/DECISIONS.md D-034).
///
/// Absolute paths are reported as `PROJ-PATH-002` warnings and still written
/// verbatim, so a broken value is visible instead of silently disappearing.
[[nodiscard]] QJsonObject projectJsonSourceToJson(
    const core::CaseSourceInfo& source,
    const ProjectJsonExtras& extras,
    std::vector<ProjectJsonDiagnostic>& diagnostics);

/// Reads the `source` section into `source`, which is left empty when the
/// document has no such section (a schemaVersion 1 file, or a new project).
///
/// Problems are reported through `reader` with the full JSON path
/// (`source.files[3].sha256`), so a hand-edited file is diagnosable.
void projectJsonSourceFromJson(
    const QJsonObject& root,
    core::CaseSourceInfo& source,
    ProjectJsonValueReader& reader);

} // namespace ofgui::io
