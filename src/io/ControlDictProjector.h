#pragma once

#include "core/SolverSettings.h"
#include "io/CaseProjectionContext.h"
#include "io/ImportedSetting.h"

#include <string>
#include <vector>

namespace ofgui::io {

class FoamDictionary;

/// Result of projecting `system/controlDict`.
struct ControlDictProjection
{
    core::SolverSettings solver;
    bool hasApplication = false;
    /// True when at least one of startTime/endTime/deltaT was readable.
    bool hasTiming = false;
    /// Every `controlDict` entry (including the ones copied into `solver`), so the
    /// exporter can mirror keywords the GUI does not model yet.
    std::vector<ImportedSetting> settings;
};

/// Projects `system/controlDict` into `SolverSettings` plus read-only entries.
///
/// `functions{}` is never interpreted: it stays in `settings` as raw text and is
/// reported by the importer as unsupported (spec section 4.4).
class ControlDictProjector
{
public:
    [[nodiscard]] static ControlDictProjection project(
        const FoamDictionary& dictionary,
        const CaseProjectionContext& context);
};

} // namespace ofgui::io
