#pragma once

#include "io/ProjectJsonDiagnostic.h"

#include <string>
#include <vector>

class QJsonObject;

namespace ofgui::core {
class CaseProject;
}

namespace ofgui::io {

class ProjectJsonValueReader;
struct ProjectJsonExtras;

/// Internal cross-file hooks of the `.ofgproj` reader.
///
/// `ProjectJsonRead.cpp`, `ProjectJsonReadPatches.cpp`,
/// `ProjectJsonReadFields.cpp` and `ProjectJsonReadExtras.cpp` implement one
/// section each (D-021: one file per sub-function, no file above 400 lines).
/// These declarations are not part of the public API; only
/// `ProjectJsonRepository` is.
void projectJsonReadPatches(
    ProjectJsonValueReader& reader,
    const QJsonObject& root,
    core::CaseProject& project);

void projectJsonReadFields(
    ProjectJsonValueReader& reader,
    const QJsonObject& root,
    core::CaseProject& project);

/// Collects the keys this build does not model so a save writes them back.
void projectJsonCollectExtras(const QJsonObject& root, ProjectJsonExtras& extras);

} // namespace ofgui::io
