#pragma once

#include "core/CaseProject.h"
#include "io/ProjectJsonDiagnostic.h"
#include "io/ProjectJsonExtras.h"

#include <QString>

#include <filesystem>
#include <string>
#include <vector>

namespace ofgui::io {

/// Result of loading a `.ofgproj` file. On failure `ok` is false, the model and
/// the extras are left empty, and `diagnostics` explains why the file was
/// rejected. A rejected file is never rewritten by this class.
struct ProjectLoadResult
{
    bool ok{false};
    core::CaseProject project;
    ProjectJsonExtras extras;
    std::vector<ProjectJsonDiagnostic> diagnostics;

    /// `schemaVersion` read from the file (0 when the file was rejected before
    /// the version could be read).
    int schemaVersion{0};

    /// True when the file was older than the version this build writes and was
    /// upgraded in memory (a `PROJ-SCHEMA-004` info diagnostic is present).
    /// Saving the loaded project writes the current version.
    bool migrated{false};

    std::string diagnosticsText() const;
};

/// Result of saving a `.ofgproj` file.
struct ProjectSaveResult
{
    bool ok{false};
    std::vector<ProjectJsonDiagnostic> diagnostics;

    std::string diagnosticsText() const;
};

/// Reads and writes the project file format (`.ofgproj`, schema version 2).
///
/// Loading accepts versions @ref projectjson::kOldestSupportedProjectSchemaVersion
/// through @ref projectjson::kCurrentProjectSchemaVersion. An older document is
/// migrated in memory (v1 has no `source` section, so the loaded project has no
/// source) and reported as `PROJ-SCHEMA-004`; the file itself is never rewritten
/// by a load. A document from a newer build is rejected as `PROJ-SCHEMA-002`
/// instead of being guessed at, so an older build cannot destroy data written by
/// a newer one. Saving always writes the current version, into a temporary file
/// that is renamed over the target, therefore a failed save leaves the previous
/// file intact.
class ProjectJsonRepository
{
public:
    static ProjectLoadResult load(const std::filesystem::path& path);

    static ProjectSaveResult save(
        const std::filesystem::path& path,
        const core::CaseProject& project,
        const ProjectJsonExtras& extras = {});

    /// Serialises the project into the exact bytes `save()` would write. Exposed
    /// for the export preview and for golden-file tests.
    static std::string toJsonDocument(
        const core::CaseProject& project,
        const ProjectJsonExtras& extras,
        std::vector<ProjectJsonDiagnostic>& diagnostics);
};

} // namespace ofgui::io
