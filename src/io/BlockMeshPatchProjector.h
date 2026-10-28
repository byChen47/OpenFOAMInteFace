#pragma once

#include "io/BlockMeshProjector.h"

#include <string>

namespace ofgui::io {

class FoamDictionary;

/// Projects the `boundary` list of `blockMeshDict` into `projection.patches`:
/// patch names, patch types and the faces each patch owns (spec section 6.3).
///
/// Quoted regular expression keys never enter the model; they are reported with
/// `IMPORT-PATCH-001` and kept in the patch list with `regularExpression` set so
/// later stages can still show them as read-only.
void projectBlockMeshPatches(const FoamDictionary& dictionary,
                             const std::string& file,
                             const CaseProjectionContext& context,
                             BlockMeshProjection& projection);

} // namespace ofgui::io
