#pragma once

#include "ui/NodeDetailFormatter.h"

namespace ofgui {

class CaseDocumentController;

namespace detail {

/// Detail of the boundary oriented nodes: `Boundaries`, `BoundaryFace`,
/// `Fields`, `Field` and `FieldBoundary`.
///
/// Split from `NodeDetailFormatter.cpp` (D-021) because boundary conditions are
/// a self-contained topic that grows with the field editor (t10).
[[nodiscard]] NodeDetail describeBoundaryNode(
    const CaseDocumentController& document,
    const ProjectNodeRef& ref);

} // namespace detail
} // namespace ofgui
