#include "core/FieldDefinition.h"

#include <utility>

namespace ofgui::core {

std::string_view fieldClassName(FieldClass fieldClass) noexcept
{
    switch (fieldClass)
    {
    case FieldClass::VolScalarField:
        return "volScalarField";
    case FieldClass::VolVectorField:
        return "volVectorField";
    }

    return "volScalarField";
}

std::optional<FieldClass> fieldClassFromName(std::string_view name) noexcept
{
    if (name == "volScalarField")
    {
        return FieldClass::VolScalarField;
    }
    if (name == "volVectorField")
    {
        return FieldClass::VolVectorField;
    }

    return std::nullopt;
}

FieldDefinition::FieldDefinition(
    std::string name,
    FieldClass fieldClass,
    const Dimensions& dimensions)
    : name_(std::move(name))
    , fieldClass_(fieldClass)
    , dimensions_(dimensions)
{
}

const ObjectId& FieldDefinition::id() const noexcept
{
    return id_;
}

void FieldDefinition::setId(const ObjectId& id) noexcept
{
    id_ = id;
}

const std::string& FieldDefinition::name() const noexcept
{
    return name_;
}

void FieldDefinition::setName(std::string name)
{
    name_ = std::move(name);
}

FieldClass FieldDefinition::fieldClass() const noexcept
{
    return fieldClass_;
}

void FieldDefinition::setFieldClass(FieldClass fieldClass) noexcept
{
    fieldClass_ = fieldClass;
}

const FieldDefinition::Dimensions& FieldDefinition::dimensions() const noexcept
{
    return dimensions_;
}

void FieldDefinition::setDimensions(const Dimensions& dimensions) noexcept
{
    dimensions_ = dimensions;
}

const FieldValue& FieldDefinition::internalField() const noexcept
{
    return internalField_;
}

void FieldDefinition::setInternalField(FieldValue value)
{
    internalField_ = std::move(value);
}

const std::map<ObjectId, BoundaryCondition>& FieldDefinition::boundaryConditions() const noexcept
{
    return boundaryConditions_;
}

void FieldDefinition::setBoundaryCondition(const ObjectId& patchId, BoundaryCondition condition)
{
    boundaryConditions_.insert_or_assign(patchId, std::move(condition));
}

bool FieldDefinition::removeBoundaryCondition(const ObjectId& patchId)
{
    return boundaryConditions_.erase(patchId) > 0;
}

const BoundaryCondition* FieldDefinition::findBoundaryCondition(const ObjectId& patchId) const
{
    const auto found = boundaryConditions_.find(patchId);
    return (found == boundaryConditions_.end()) ? nullptr : &found->second;
}

BoundaryCondition* FieldDefinition::findBoundaryCondition(const ObjectId& patchId)
{
    const auto found = boundaryConditions_.find(patchId);
    return (found == boundaryConditions_.end()) ? nullptr : &found->second;
}

bool FieldDefinition::hasBoundaryCondition(const ObjectId& patchId) const
{
    return findBoundaryCondition(patchId) != nullptr;
}

std::vector<ObjectId> FieldDefinition::patchesWithoutCondition(
    const std::vector<ObjectId>& allPatchIds) const
{
    std::vector<ObjectId> missing;
    missing.reserve(allPatchIds.size());

    for (const ObjectId& patchId : allPatchIds)
    {
        if (!hasBoundaryCondition(patchId))
        {
            missing.push_back(patchId);
        }
    }

    return missing;
}

} // namespace ofgui::core
