#pragma once

#include "core/CaseProject.h"
#include "io/ImportReport.h"
#include "io/ImportedSetting.h"
#include "io/TransportPropertiesProjector.h"

#include <vector>

namespace ofgui::io {

/// Everything one import produced.
///
/// `project` is filled in memory only; the case directory is never modified.
/// `readOnlySettings` carries the keyword/value pairs the GUI does not edit yet,
/// so the exporter can mirror them (schemaVersion 2 / t17 persists
/// `project.source()` as well).
struct CaseImportResult
{
    /// False when a `BlockingError` was reported (import must not continue).
    bool ok = false;
    core::CaseProject project;
    ImportReport report;
    TransportPropertiesProjection transport;
    std::vector<ImportedSetting> readOnlySettings;
    /// True when at least one file was read as `Partial` (unexpanded include).
    bool partial = false;
};

} // namespace ofgui::io
