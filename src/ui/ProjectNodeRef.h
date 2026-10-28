#pragma once

#include "core/FaceId.h"
#include "core/ObjectId.h"

#include <QMetaType>

namespace ofgui {

/// Kind of one node of the project tree.
///
/// The tree is a projection of `core::CaseProject`; a node never owns business
/// data. Together with `ProjectNodeRef::face` / `fieldId` / `patchId` the kind
/// is the *only* identity the UI may use for selection: display labels are
/// translatable and editable, so they must never be compared (AGENTS.md §5).
enum class NodeKind
{
    Project,
    Domain,
    Mesh,
    Boundaries,
    BoundaryFace,
    Fields,
    Field,
    FieldBoundary,
    Solver,
    ExportPreview
};

/// Stable reference to one tree node.
///
/// `face` is meaningful for `BoundaryFace`, `fieldId` for `Field` and
/// `FieldBoundary`, and both ids for `FieldBoundary` (one boundary condition is
/// addressed by the pair field + patch). All other kinds address a singleton
/// section of the document and only use `kind`.
struct ProjectNodeRef
{
    NodeKind kind = NodeKind::Project;
    core::FaceId face = core::FaceId::XMin;
    core::ObjectId fieldId;
    core::ObjectId patchId;

    [[nodiscard]] static ProjectNodeRef section(NodeKind kind) noexcept
    {
        ProjectNodeRef ref;
        ref.kind = kind;
        return ref;
    }

    [[nodiscard]] static ProjectNodeRef forFace(core::FaceId face) noexcept
    {
        ProjectNodeRef ref;
        ref.kind = NodeKind::BoundaryFace;
        ref.face = face;
        return ref;
    }

    [[nodiscard]] static ProjectNodeRef forField(const core::ObjectId& fieldId) noexcept
    {
        ProjectNodeRef ref;
        ref.kind = NodeKind::Field;
        ref.fieldId = fieldId;
        return ref;
    }

    [[nodiscard]] static ProjectNodeRef forFieldBoundary(
        const core::ObjectId& fieldId,
        const core::ObjectId& patchId) noexcept
    {
        ProjectNodeRef ref;
        ref.kind = NodeKind::FieldBoundary;
        ref.fieldId = fieldId;
        ref.patchId = patchId;
        return ref;
    }

    /// True when every id the kind requires is set.
    [[nodiscard]] bool isValid() const noexcept
    {
        switch (kind)
        {
            case NodeKind::Field:
                return fieldId.isValid();
            case NodeKind::FieldBoundary:
                return fieldId.isValid() && patchId.isValid();
            default:
                return true;
        }
    }

    friend bool operator==(const ProjectNodeRef& left, const ProjectNodeRef& right) noexcept
    {
        return left.kind == right.kind && left.face == right.face
               && left.fieldId == right.fieldId && left.patchId == right.patchId;
    }

    friend bool operator!=(const ProjectNodeRef& left, const ProjectNodeRef& right) noexcept
    {
        return !(left == right);
    }
};

} // namespace ofgui

Q_DECLARE_METATYPE(ofgui::ProjectNodeRef)
