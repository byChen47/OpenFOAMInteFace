#pragma once

#include "ui/ProjectNodeRef.h"

#include <QCoreApplication>
#include <QString>

namespace ofgui {

class CaseDocumentController;

/// Read-only description of one tree node, rendered by `InspectorPanel`.
struct NodeDetail
{
    QString title;
    QString subtitle;
    /// HTML fragment; every dynamic text is escaped.
    QString html;
};

/// Builds the read-only detail shown for a tree selection.
///
/// The formatter only reads the document; editing arrives with the dedicated
/// inspectors (domain, field, solver). Keeping it free of widgets makes the
/// content unit testable and keeps `InspectorPanel` a thin container.
class NodeDetailFormatter
{
    Q_DECLARE_TR_FUNCTIONS(NodeDetailFormatter)

public:
    [[nodiscard]] static NodeDetail describe(
        const CaseDocumentController& document,
        const ProjectNodeRef& ref);
};

} // namespace ofgui
