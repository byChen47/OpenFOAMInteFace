#include "core/CaseValidator.h"

#include "core/CaseValidatorMessages.h"
#include "core/FaceAssignmentValidator.h"

#include <map>
#include <string>
#include <vector>

// CASE-PATCH-*: patch names, six-face coverage and the 2D empty boundary.

namespace ofgui::core {

void CaseValidator::validatePatches(const CaseProject& project, ValidationResult& result) const
{
    const std::vector<BoundaryPatch>& patches = project.patches();

    if (patches.empty())
    {
        ValidationIssue issue(Severity::BlockingError, "CASE-PATCH-001",
            "工程中没有任何 patch，至少需要一个 patch 覆盖六个边界面。");
        issue.setTargetField("patches");
        issue.setSuggestedFix("创建 patch 并分配六个边界面。");
        result.add(std::move(issue));
        return;
    }

    std::map<std::string, std::vector<ObjectId>> patchesByName;
    for (const BoundaryPatch& patch : patches)
    {
        if (patch.name().empty())
        {
            ValidationIssue issue(Severity::BlockingError, "CASE-PATCH-002",
                "patch 名称不能为空。");
            issue.setTargetObjectId(patch.id());
            issue.setTargetField("patches[].name");
            issue.setSuggestedFix("输入以字母开头的 patch 名称，例如 inlet。");
            result.add(std::move(issue));
        }
        else if (!BoundaryPatch::isValidName(patch.name()))
        {
            ValidationIssue issue(Severity::BlockingError, "CASE-PATCH-003",
                "patch 名称 \"" + patch.name()
                    + "\" 非法：必须以字母开头，且只包含字母、数字和下划线。");
            issue.setTargetObjectId(patch.id());
            issue.setTargetField("patches[].name");
            issue.setSuggestedFix("改为例如 inlet_1 的写法。");
            result.add(std::move(issue));
        }
        else
        {
            patchesByName[patch.name()].push_back(patch.id());
        }

        if (patch.faceCount() == 0)
        {
            ValidationIssue issue(Severity::Warning, "CASE-PATCH-007",
                "patch \"" + patch.name() + "\" 没有分配任何边界面。");
            issue.setTargetObjectId(patch.id());
            issue.setTargetField("patches[].faces");
            result.add(std::move(issue));
        }
    }

    for (const auto& [name, ids] : patchesByName)
    {
        if (ids.size() > 1)
        {
            ValidationIssue issue(Severity::BlockingError, "CASE-PATCH-004",
                "patch 名称 \"" + name + "\" 重复（共 " + std::to_string(ids.size()) + " 个）。");
            issue.setTargetObjectId(ids.at(1));
            issue.setTargetField("patches[].name");
            issue.setSuggestedFix("给重名的 patch 换一个唯一名称。");
            result.add(std::move(issue));
        }
    }

    const auto coverage = FaceAssignmentValidator::coverage(patches);
    const auto patchName = [&patches](const ObjectId& id) {
        for (const BoundaryPatch& patch : patches)
        {
            if (patch.id() == id)
            {
                return patch.name();
            }
        }
        return std::string("<unknown>");
    };

    for (const auto& face : coverage)
    {
        if (!face.isAssigned())
        {
            ValidationIssue issue(Severity::BlockingError, "CASE-PATCH-005",
                "边界面 " + detail::faceLabel(face.face) + " 没有归属任何 patch。");
            issue.setTargetField("patches");
            issue.setSuggestedFix("把该面分配给一个 patch。");
            result.add(std::move(issue));
        }
        else if (face.isAmbiguous())
        {
            std::vector<std::string> owners;
            owners.reserve(face.patchIds.size());
            for (const ObjectId& id : face.patchIds)
            {
                owners.push_back(patchName(id));
            }

            ValidationIssue issue(Severity::BlockingError, "CASE-PATCH-006",
                "边界面 " + detail::faceLabel(face.face) + " 被 "
                    + std::to_string(face.patchIds.size())
                    + " 个 patch 同时覆盖：" + detail::joinNames("、", owners) + "。");
            issue.setTargetField("patches");
            issue.setSuggestedFix("保证每个边界面只属于一个 patch。");
            result.add(std::move(issue));
        }
    }

    if (project.domain().mode() == DomainMode::TwoD)
    {
        for (const FaceId face : {FaceId::ZMin, FaceId::ZMax})
        {
            const auto& owners = coverage[faceIndex(face)].patchIds;
            if (owners.size() != 1)
            {
                continue;
            }

            const BoundaryPatch* owner = project.findPatch(owners.front());
            if (owner != nullptr && owner->type() != PatchType::Empty)
            {
                ValidationIssue issue(Severity::BlockingError, "CASE-PATCH-008",
                    "2D 模式下前/后面（" + detail::faceLabel(face) + "）所属 patch \""
                        + owner->name() + "\" 的类型必须是 empty，当前为 "
                        + std::string(patchTypeName(owner->type())) + "。");
                issue.setTargetObjectId(owner->id());
                issue.setTargetField("patches[].type");
                issue.setSuggestedFix("把该 patch 的类型改为 empty。");
                result.add(std::move(issue));
            }
        }
    }
}

} // namespace ofgui::core
