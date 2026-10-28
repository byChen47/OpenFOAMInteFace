#include "ui/ValidationNavigator.h"

#include "core/BoundaryPatch.h"
#include "core/FieldDefinition.h"

#include <string>
#include <string_view>

namespace ofgui {
namespace {

bool startsWith(std::string_view text, std::string_view prefix)
{
    return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
}

ProjectNodeRef refForPatch(const core::CaseProject& project, const core::ObjectId& patchId)
{
    const core::BoundaryPatch* patch = project.findPatch(patchId);
    if (patch == nullptr)
    {
        return ProjectNodeRef::section(NodeKind::Boundaries);
    }
    if (!patch->faces().empty())
    {
        return ProjectNodeRef::forFace(*patch->faces().begin());
    }
    return ProjectNodeRef::section(NodeKind::Boundaries);
}

} // namespace

ProjectNodeRef nodeRefForIssue(
    const core::CaseProject& project,
    const core::ValidationIssue& issue)
{
    const std::string& target = issue.targetField();

    if (startsWith(target, "domain"))
    {
        return ProjectNodeRef::section(NodeKind::Domain);
    }
    if (startsWith(target, "mesh"))
    {
        return ProjectNodeRef::section(NodeKind::Mesh);
    }
    if (startsWith(target, "solver"))
    {
        return ProjectNodeRef::section(NodeKind::Solver);
    }
    if (startsWith(target, "patches"))
    {
        if (issue.targetObjectId().has_value())
        {
            return refForPatch(project, *issue.targetObjectId());
        }
        return ProjectNodeRef::section(NodeKind::Boundaries);
    }
    if (startsWith(target, "fields"))
    {
        if (issue.targetObjectId().has_value())
        {
            if (const core::FieldDefinition* field = project.findField(*issue.targetObjectId());
                field != nullptr)
            {
                return ProjectNodeRef::forField(field->id());
            }
        }
        return ProjectNodeRef::section(NodeKind::Fields);
    }

    // Issues without a target field still carry an object id when they are about
    // one patch or one field.
    if (issue.targetObjectId().has_value())
    {
        const core::ObjectId& id = *issue.targetObjectId();
        if (project.findField(id) != nullptr)
        {
            return ProjectNodeRef::forField(id);
        }
        if (project.findPatch(id) != nullptr)
        {
            return refForPatch(project, id);
        }
    }

    return ProjectNodeRef::section(NodeKind::Project);
}

} // namespace ofgui
