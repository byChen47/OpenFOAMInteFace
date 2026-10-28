#include "core/CaseValidator.h"

#include "core/CaseValidatorMessages.h"

#include <map>
#include <string>
#include <vector>

// CASE-FIELD-*: field names, internalField completeness and per-patch BCs.

namespace ofgui::core {

void CaseValidator::validateFields(const CaseProject& project, ValidationResult& result) const
{
    const std::vector<ObjectId> allPatchIds = project.patchIds();
    std::map<std::string, std::vector<ObjectId>> fieldsByName;

    for (const FieldDefinition& field : project.fields())
    {
        if (field.name().empty())
        {
            ValidationIssue issue(Severity::Error, "CASE-FIELD-001", "场名称不能为空。");
            issue.setTargetObjectId(field.id());
            issue.setTargetField("fields[].name");
            result.add(std::move(issue));
        }
        else
        {
            fieldsByName[field.name()].push_back(field.id());
        }

        if (!field.internalField().isValid())
        {
            ValidationIssue issue(Severity::Warning, "CASE-FIELD-004",
                "场 \"" + field.name() + "\" 还没有设置 internalField。");
            issue.setTargetObjectId(field.id());
            issue.setTargetField("fields[].internalField");
            result.add(std::move(issue));
        }

        const std::vector<ObjectId> missing = field.patchesWithoutCondition(allPatchIds);
        if (!missing.empty())
        {
            std::vector<std::string> names;
            names.reserve(missing.size());
            for (const ObjectId& patchId : missing)
            {
                const BoundaryPatch* patch = project.findPatch(patchId);
                names.push_back(patch != nullptr ? patch->name() : std::string("<unknown>"));
            }

            ValidationIssue issue(Severity::Error, "CASE-FIELD-003",
                "场 \"" + field.name() + "\" 缺少 " + std::to_string(missing.size())
                    + " 个 patch 的边界条件：" + detail::joinNames("、", names) + "。");
            issue.setTargetObjectId(field.id());
            issue.setTargetField("fields[].boundaryConditions");
            issue.setSuggestedFix("为每个 patch 设置边界条件类型。");
            result.add(std::move(issue));
        }
    }

    for (const auto& [name, ids] : fieldsByName)
    {
        if (ids.size() > 1)
        {
            ValidationIssue issue(Severity::Error, "CASE-FIELD-002",
                "场名称 \"" + name + "\" 重复（共 " + std::to_string(ids.size()) + " 个）。");
            issue.setTargetObjectId(ids.at(1));
            issue.setTargetField("fields[].name");
            issue.setSuggestedFix("场名称必须唯一（对应 0/ 目录下的文件名）。");
            result.add(std::move(issue));
        }
    }
}

} // namespace ofgui::core
