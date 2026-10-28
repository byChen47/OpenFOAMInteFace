#include "core/CaseProject.h"

#include <algorithm>
#include <utility>

namespace ofgui::core {

CaseProject::CaseProject(std::string name)
    : name_(std::move(name))
{
}

int CaseProject::schemaVersion() const noexcept
{
    return kCurrentSchemaVersion;
}

const std::string& CaseProject::name() const noexcept
{
    return name_;
}

void CaseProject::setName(std::string name)
{
    name_ = std::move(name);
}

const std::string& CaseProject::caseDirectory() const noexcept
{
    return caseDirectory_;
}

void CaseProject::setCaseDirectory(std::string caseDirectory)
{
    caseDirectory_ = std::move(caseDirectory);
}

const CaseSourceInfo& CaseProject::source() const noexcept
{
    return source_;
}

CaseSourceInfo& CaseProject::source() noexcept
{
    return source_;
}

void CaseProject::setSource(CaseSourceInfo source)
{
    source_ = std::move(source);
}

bool CaseProject::hasSource() const noexcept
{
    return !source_.isEmpty();
}

const DomainBox& CaseProject::domain() const noexcept
{
    return domain_;
}

DomainBox& CaseProject::domain() noexcept
{
    return domain_;
}

void CaseProject::setDomain(DomainBox domain)
{
    domain_ = std::move(domain);
}

const MeshSettings& CaseProject::mesh() const noexcept
{
    return mesh_;
}

MeshSettings& CaseProject::mesh() noexcept
{
    return mesh_;
}

void CaseProject::setMesh(MeshSettings mesh)
{
    mesh_ = std::move(mesh);
}

const SolverSettings& CaseProject::solver() const noexcept
{
    return solver_;
}

SolverSettings& CaseProject::solver() noexcept
{
    return solver_;
}

void CaseProject::setSolver(SolverSettings solver)
{
    solver_ = std::move(solver);
}

const std::vector<BoundaryPatch>& CaseProject::patches() const noexcept
{
    return patches_;
}

const std::vector<FieldDefinition>& CaseProject::fields() const noexcept
{
    return fields_;
}

ObjectId CaseProject::addPatch(std::string name, PatchType type)
{
    BoundaryPatch patch = BoundaryPatch::create(std::move(name), type);
    const ObjectId id = patch.id();
    patches_.push_back(std::move(patch));
    return id;
}

bool CaseProject::removePatch(const ObjectId& id)
{
    const auto found = std::find_if(patches_.begin(), patches_.end(),
        [&id](const BoundaryPatch& patch) { return patch.id() == id; });
    if (found == patches_.end())
    {
        return false;
    }

    patches_.erase(found);
    for (FieldDefinition& field : fields_)
    {
        field.removeBoundaryCondition(id);
    }

    return true;
}

const BoundaryPatch* CaseProject::findPatch(const ObjectId& id) const
{
    const auto found = std::find_if(patches_.begin(), patches_.end(),
        [&id](const BoundaryPatch& patch) { return patch.id() == id; });
    return (found == patches_.end()) ? nullptr : &*found;
}

BoundaryPatch* CaseProject::findPatch(const ObjectId& id)
{
    const auto found = std::find_if(patches_.begin(), patches_.end(),
        [&id](const BoundaryPatch& patch) { return patch.id() == id; });
    return (found == patches_.end()) ? nullptr : &*found;
}

const BoundaryPatch* CaseProject::findPatchByName(std::string_view name) const
{
    const auto found = std::find_if(patches_.begin(), patches_.end(),
        [name](const BoundaryPatch& patch) { return patch.name() == name; });
    return (found == patches_.end()) ? nullptr : &*found;
}

std::optional<ObjectId> CaseProject::patchIdByName(std::string_view name) const
{
    const BoundaryPatch* patch = findPatchByName(name);
    if (patch == nullptr)
    {
        return std::nullopt;
    }

    return patch->id();
}

bool CaseProject::moveFaceToPatch(FaceId face, const ObjectId& patchId)
{
    BoundaryPatch* target = findPatch(patchId);
    if (target == nullptr)
    {
        return false;
    }

    for (BoundaryPatch& patch : patches_)
    {
        if (patch.id() != patchId)
        {
            patch.removeFace(face);
        }
    }

    target->addFace(face);
    return true;
}

ObjectId CaseProject::addField(
    std::string name,
    FieldClass fieldClass,
    const FieldDefinition::Dimensions& dimensions)
{
    FieldDefinition field(std::move(name), fieldClass, dimensions);
    const ObjectId id = ObjectId::create();
    field.setId(id);
    fields_.push_back(std::move(field));
    return id;
}

bool CaseProject::removeField(const ObjectId& id)
{
    const auto found = std::find_if(fields_.begin(), fields_.end(),
        [&id](const FieldDefinition& field) { return field.id() == id; });
    if (found == fields_.end())
    {
        return false;
    }

    fields_.erase(found);
    return true;
}

const FieldDefinition* CaseProject::findField(const ObjectId& id) const
{
    const auto found = std::find_if(fields_.begin(), fields_.end(),
        [&id](const FieldDefinition& field) { return field.id() == id; });
    return (found == fields_.end()) ? nullptr : &*found;
}

FieldDefinition* CaseProject::findField(const ObjectId& id)
{
    const auto found = std::find_if(fields_.begin(), fields_.end(),
        [&id](const FieldDefinition& field) { return field.id() == id; });
    return (found == fields_.end()) ? nullptr : &*found;
}

const FieldDefinition* CaseProject::findFieldByName(std::string_view name) const
{
    const auto found = std::find_if(fields_.begin(), fields_.end(),
        [name](const FieldDefinition& field) { return field.name() == name; });
    return (found == fields_.end()) ? nullptr : &*found;
}

std::vector<ObjectId> CaseProject::patchIds() const
{
    std::vector<ObjectId> ids;
    ids.reserve(patches_.size());
    for (const BoundaryPatch& patch : patches_)
    {
        ids.push_back(patch.id());
    }

    return ids;
}

bool CaseProject::isEmpty() const noexcept
{
    return patches_.empty() && fields_.empty();
}

} // namespace ofgui::core
