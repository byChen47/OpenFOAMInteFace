#pragma once

#include <QHash>
#include <QJsonObject>
#include <QString>

namespace ofgui::io {

/// Parts of an existing `.ofgproj` document that this build does not model.
///
/// Policy (docs/DECISIONS.md D-030): unknown keys are preserved, not dropped.
/// A newer build may add `mesh.boundaryLayers` or a per-patch `notes` field;
/// saving the project from an older build must not destroy that data. Only the
/// keys the reader/writer pair owns are stripped; everything else travels back
/// unchanged. Keys inside `internalField` and inside a boundary condition's
/// `parameters` map are owned by the model and are intentionally not preserved.
struct ProjectJsonExtras
{
    /// Unknown keys of the document root.
    QJsonObject root;
    /// Unknown keys of `project`, `domain`, `mesh` and `solver`.
    QJsonObject project;
    QJsonObject domain;
    QJsonObject mesh;
    QJsonObject solver;
    /// Unknown keys per patch, keyed by `ObjectId::toString()`.
    QHash<QString, QJsonObject> patches;
    /// Unknown keys per field, keyed by `ObjectId::toString()`.
    QHash<QString, QJsonObject> fields;
    /// Unknown keys per boundary condition, keyed by `"<fieldId>/<patchId>"`.
    QHash<QString, QJsonObject> boundaryConditions;
    /// Unknown keys of the `source` section (schemaVersion 2).
    QJsonObject source;
    /// Unknown keys per `source.files[]` entry, keyed by its `relativePath`.
    QHash<QString, QJsonObject> sourceFiles;

    [[nodiscard]] bool isEmpty() const noexcept
    {
        return root.isEmpty() && project.isEmpty() && domain.isEmpty() && mesh.isEmpty()
               && solver.isEmpty() && patches.isEmpty() && fields.isEmpty()
               && boundaryConditions.isEmpty() && source.isEmpty() && sourceFiles.isEmpty();
    }
};

} // namespace ofgui::io
