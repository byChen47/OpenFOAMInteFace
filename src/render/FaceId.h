#pragma once

// Compatibility header.
//
// `FaceId` is a core domain type now: the single definition lives in
// `src/core/FaceId.h` (`ofgui::core::FaceId`) because `BoundaryPatch` owns face
// assignments and `render` must not be the business data source.
//
// This header only re-exports the core type into the historical `ofgui`
// namespace so existing render/ui code keeps compiling. New code should
// `#include "core/FaceId.h"` and use `core::FaceId` directly.

#include "core/FaceId.h"

namespace ofgui {

using core::faceAxis;
using core::faceIndex;
using core::faceIsMax;
using core::faceName;
using core::FaceId;
using core::kAllFaceIds;

} // namespace ofgui
