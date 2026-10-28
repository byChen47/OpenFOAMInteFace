#pragma once

#include "ui/ProjectNodeRef.h"

#include <QString>

namespace ofgui {

class CaseDocumentController;

/// Display text of one tree node, resolved from the document on every call.
///
/// Kept separate from `ProjectTreeModel` so the identity logic (structure,
/// indexes, stable refs) and the presentation logic stay independently
/// reviewable and testable.
[[nodiscard]] QString treeNodeLabel(const CaseDocumentController& document,
                                   const ProjectNodeRef& ref);

/// Multi-line tool tip explaining the node in terms of the model.
[[nodiscard]] QString treeNodeToolTip(const CaseDocumentController& document,
                                      const ProjectNodeRef& ref);

} // namespace ofgui
