#pragma once

#include "core/BoundaryPatch.h"
#include "core/CaseSourceInfo.h"
#include "core/DomainBox.h"
#include "core/FieldDefinition.h"
#include "core/MeshSettings.h"
#include "core/ObjectId.h"
#include "core/ProjectSchemaVersion.h"
#include "core/SolverSettings.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ofgui::core {

/// Root business object of the GUI: the single source of truth the renderer,
/// the panels and the exporter talk to.
///
/// Invariants are enforced by `CaseValidator`, not by the setters: the editor
/// must be able to hold a temporarily inconsistent state (for example while the
/// user types a new min point) and show the problems inline.
class CaseProject
{
public:
    CaseProject() = default;

    explicit CaseProject(std::string name);

    [[nodiscard]] int schemaVersion() const noexcept;

    [[nodiscard]] const std::string& name() const noexcept;
    void setName(std::string name);

    /// Directory of the OpenFOAM case this project was created from or writes to.
    [[nodiscard]] const std::string& caseDirectory() const noexcept;
    void setCaseDirectory(std::string caseDirectory);

    /// Provenance of an imported case (docs/DECISIONS.md D-025).
    ///
    /// An all-empty value means "new project, no source case"; the importer
    /// fills it before handing the project to the UI.
    [[nodiscard]] const CaseSourceInfo& source() const noexcept;
    [[nodiscard]] CaseSourceInfo& source() noexcept;
    void setSource(CaseSourceInfo source);

    /// True when the project was imported from an existing case directory.
    [[nodiscard]] bool hasSource() const noexcept;

    [[nodiscard]] const DomainBox& domain() const noexcept;
    [[nodiscard]] DomainBox& domain() noexcept;
    void setDomain(DomainBox domain);

    [[nodiscard]] const MeshSettings& mesh() const noexcept;
    [[nodiscard]] MeshSettings& mesh() noexcept;
    void setMesh(MeshSettings mesh);

    [[nodiscard]] const SolverSettings& solver() const noexcept;
    [[nodiscard]] SolverSettings& solver() noexcept;
    void setSolver(SolverSettings solver);

    [[nodiscard]] const std::vector<BoundaryPatch>& patches() const noexcept;
    [[nodiscard]] const std::vector<FieldDefinition>& fields() const noexcept;

    /// Creates a patch with a fresh id. @return the new patch id.
    ObjectId addPatch(std::string name, PatchType type = PatchType::Patch);

    /// Removes the patch and every boundary condition that referenced it.
    /// @return false when no patch with that id exists.
    bool removePatch(const ObjectId& id);

    [[nodiscard]] const BoundaryPatch* findPatch(const ObjectId& id) const;
    [[nodiscard]] BoundaryPatch* findPatch(const ObjectId& id);
    [[nodiscard]] const BoundaryPatch* findPatchByName(std::string_view name) const;

    [[nodiscard]] std::optional<ObjectId> patchIdByName(std::string_view name) const;

    /// Moves one face to `patchId`, removing it from every other patch.
    ///
    /// This is the intentional "assign" operation of the GUI; the model still
    /// allows inconsistent lists via `BoundaryPatch::addFace`, which the
    /// validator reports.
    /// @return false when no patch with that id exists.
    bool moveFaceToPatch(FaceId face, const ObjectId& patchId);

    /// Creates a field with a fresh id. @return the new field id.
    ObjectId addField(
        std::string name,
        FieldClass fieldClass,
        const FieldDefinition::Dimensions& dimensions = {});

    /// @return false when no field with that id exists.
    bool removeField(const ObjectId& id);

    [[nodiscard]] const FieldDefinition* findField(const ObjectId& id) const;
    [[nodiscard]] FieldDefinition* findField(const ObjectId& id);
    [[nodiscard]] const FieldDefinition* findFieldByName(std::string_view name) const;

    /// Patch ids in list order; the validator uses it for coverage checks.
    [[nodiscard]] std::vector<ObjectId> patchIds() const;

    /// True when neither a patch nor a field exists yet (empty new project).
    [[nodiscard]] bool isEmpty() const noexcept;

private:
    std::string name_;
    std::string caseDirectory_;
    CaseSourceInfo source_;
    DomainBox domain_;
    MeshSettings mesh_;
    SolverSettings solver_;
    std::vector<BoundaryPatch> patches_;
    std::vector<FieldDefinition> fields_;
};

} // namespace ofgui::core
