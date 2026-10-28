#include "io/CaseFieldApplier.h"

#include "io/ImportCodes.h"
#include "io/ImportIssueFactory.h"

#include <algorithm>
#include <map>
#include <set>
#include <string>

namespace ofgui::io {
namespace {

bool isEditableFieldName(const std::string& name)
{
    return name == "p" || name == "U";
}

} // namespace

void CaseFieldApplier::apply(
    const std::vector<FieldFile>& fields,
    const BlockMeshProjection* blockMesh,
    core::CaseProject& project,
    ImportReport& report)
{
    // 1. Patch order: blockMeshDict first, then field-only names in file order.
    std::vector<std::string> names;
    std::set<std::string> seen;
    if (blockMesh != nullptr)
    {
        for (const ProjectedPatch& patch : blockMesh->patches)
        {
            if (patch.regularExpression || patch.name.empty() || !seen.insert(patch.name).second)
            {
                continue;
            }
            names.push_back(patch.name);
        }
    }
    for (const FieldFile& file : fields)
    {
        for (const ProjectedBoundaryCondition& condition : file.projection.boundaryField)
        {
            if (condition.regularExpression || condition.patchName.empty())
            {
                continue;
            }
            if (seen.insert(condition.patchName).second)
            {
                names.push_back(condition.patchName);
            }
        }
    }

    // 2. Create patches with types and faces from blockMeshDict.
    std::map<std::string, ProjectedPatch> blockMeshPatches;
    if (blockMesh != nullptr)
    {
        for (const ProjectedPatch& patch : blockMesh->patches)
        {
            if (!patch.regularExpression && !patch.name.empty())
            {
                blockMeshPatches.emplace(patch.name, patch);
            }
        }
    }
    for (const std::string& name : names)
    {
        const auto found = blockMeshPatches.find(name);
        const core::PatchType type =
            found != blockMeshPatches.end() && found->second.typeKnown ? found->second.type : core::PatchType::Patch;
        if (!core::BoundaryPatch::isValidName(name))
        {
            report.issues.push_back(makeImportIssue(importcodes::kPatchEmptyName, {}, 0, 0, name, name));
            continue;
        }
        const core::ObjectId id = project.addPatch(name, type);
        if (core::BoundaryPatch* patch = project.findPatch(id))
        {
            if (found != blockMeshPatches.end() && !found->second.faces.empty())
            {
                std::set<core::FaceId> faces(found->second.faces.begin(), found->second.faces.end());
                patch->setFaces(std::move(faces));
            }
        }
    }

    // 3. Fields plus their boundary conditions.
    for (const FieldFile& file : fields)
    {
        const ProjectedField& projected = file.projection;
        if (!file.parsed || !projected.fieldClass.has_value() || projected.name.empty())
        {
            continue;
        }
        if (project.findFieldByName(projected.name) != nullptr)
        {
            continue;
        }

        const core::ObjectId fieldId =
            project.addField(projected.name, *projected.fieldClass, projected.dimensions);
        core::FieldDefinition* field = project.findField(fieldId);
        if (field == nullptr)
        {
            continue;
        }
        if (projected.internalField.isValid())
        {
            field->setInternalField(projected.internalField);
        }

        for (const ProjectedBoundaryCondition& condition : projected.boundaryField)
        {
            if (condition.regularExpression)
            {
                continue;
            }
            const std::optional<core::ObjectId> patchId = project.patchIdByName(condition.patchName);
            if (!patchId.has_value() || !patchId->isValid())
            {
                continue;
            }
            field->setBoundaryCondition(*patchId, condition.condition);
        }

        // 4. p/U must cover every patch; other fields stay read only.
        if (isEditableFieldName(projected.name))
        {
            for (const std::string& patchName : names)
            {
                const std::optional<core::ObjectId> patchId = project.patchIdByName(patchName);
                if (!patchId.has_value() || !patchId->isValid()
                    || field->hasBoundaryCondition(*patchId))
                {
                    continue;
                }
                report.issues.push_back(makeImportIssue(importcodes::kPatchMissingBoundaryCondition,
                                                        file.relativePath,
                                                        projected.boundaryField.empty() ? 0 : projected.boundaryField.front().line,
                                                        0,
                                                        "boundaryField." + patchName,
                                                        patchName));
            }
        }
    }

    // 5. blockMeshDict vs field patch names.
    if (blockMesh != nullptr && !blockMesh->patches.empty())
    {
        for (const FieldFile& file : fields)
        {
            if (!isEditableFieldName(file.projection.name) || !file.parsed)
            {
                continue;
            }
            for (const std::string& name : names)
            {
                if (!blockMeshPatches.count(name))
                {
                    report.issues.push_back(makeImportIssue(importcodes::kPatchBoundaryMismatch,
                                                            file.relativePath,
                                                            0,
                                                            0,
                                                            "boundaryField." + name,
                                                            name));
                }
            }
        }
    }
}

} // namespace ofgui::io
