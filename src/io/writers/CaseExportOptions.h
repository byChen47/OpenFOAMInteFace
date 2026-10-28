#pragma once

#include "io/writers/TransportModelSettings.h"

#include <core/Basics.h>

#include <filesystem>
#include <string>

namespace ofgui::io {

/// Inputs for one export run.
struct CaseExportOptions
{
    /// Directory that receives `0/`, `constant/` and `system/`.
    std::filesystem::path targetDirectory;

    /// `0` or `0.orig`; also the directory name in the export.
    std::string startDirectory{"0"};

    /// Source case root used by the "target must not be inside the source"
    /// guard. Empty skips the check (new projects have no source).
    std::string sourceCaseRoot;

    /// Must be set to true by the UI once the user confirmed that an existing
    /// non-empty target directory may be replaced.
    bool allowExistingTarget{false};

    /// Write `constant/transportProperties` from `transport`.
    bool writePlaceholderDictionaries{true};

    /// Copy the source files this tool does not own (`constant/polyMesh/**`,
    /// other dictionaries, fields that are not in the model). Mirrored copies
    /// are byte exact and default on (`docs/DECISIONS.md` D-023).
    bool mirrorSourceFiles{true};

    /// Keep the root level `All*` scripts instead of skipping them (spec 9.2).
    bool keepRunScripts{false};

    /// Mirror time directories other than the start directory instead of
    /// skipping them (spec 9.2).
    bool mirrorOtherTimeDirectories{false};

    TransportModelSettings transport;

    /// Text of `system/fvSchemes`; empty means the default cavity schemes.
    std::string fvSchemesText;

    /// Text of `system/fvSolution`; empty means the default PISO settings.
    std::string fvSolutionText;
};

} // namespace ofgui::io
