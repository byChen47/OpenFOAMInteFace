#pragma once

#include "core/CaseProject.h"
#include "core/ValidationIssue.h"
#include "ui/ProjectNodeRef.h"

namespace ofgui {

/// Maps a validation issue onto the tree node that owns the offending value.
///
/// The mapping is by `targetField()` prefix and stable object id, never by
/// display text (`docs/ARCHITECTURE.md` §5.7). Issues without a known target
/// resolve to the project node, so a double-click always lands somewhere.
[[nodiscard]] ProjectNodeRef nodeRefForIssue(
    const core::CaseProject& project,
    const core::ValidationIssue& issue);

} // namespace ofgui
