#include "core/CaseValidator.h"

#include "core/CaseValidatorMessages.h"

#include <cmath>
#include <string>

// CASE-MESH-*: cell counts, grading, blockMesh scale and the 2D thickness rule.

namespace ofgui::core {

void CaseValidator::validateMesh(const CaseProject& project, ValidationResult& result) const
{
    const MeshSettings& mesh = project.mesh();

    for (std::size_t axis = 0; axis < detail::kAxisNames.size(); ++axis)
    {
        if (mesh.cellCount(axis) <= 0)
        {
            ValidationIssue issue(Severity::BlockingError, "CASE-MESH-001",
                std::string("网格数 n") + detail::kAxisNames[axis] + " 必须大于 0（当前为 "
                    + std::to_string(mesh.cellCount(axis)) + "）。");
            issue.setTargetField(std::string("mesh.cells.") + detail::kAxisNames[axis]);
            issue.setSuggestedFix("把网格数设为不小于 1 的整数。");
            result.add(std::move(issue));
        }

        const Real grading = mesh.grading().at(axis);
        if (!std::isfinite(grading) || !(grading > 0.0))
        {
            ValidationIssue issue(Severity::Error, "CASE-MESH-002",
                std::string("网格渐变因子 grading ") + detail::kAxisNames[axis]
                    + " 必须是正的有限数（当前为 " + detail::formatReal(grading) + "）。");
            issue.setTargetField(std::string("mesh.grading.") + detail::kAxisNames[axis]);
            issue.setSuggestedFix("把 grading 设为 1 或其它正数。");
            result.add(std::move(issue));
        }
    }

    if (!std::isfinite(mesh.scale()) || !(mesh.scale() > 0.0))
    {
        ValidationIssue issue(Severity::Error, "CASE-MESH-003",
            "blockMesh 的 scale 必须是正的有限数（当前为 " + detail::formatReal(mesh.scale()) + "）。");
        issue.setTargetField("mesh.scale");
        issue.setSuggestedFix("把 scale 设为 1 或其它正数。");
        result.add(std::move(issue));
    }

    const bool twoDimensional = project.domain().mode() == DomainMode::TwoD;
    const int thicknessCells = mesh.cellCount(static_cast<std::size_t>(kTwoDThicknessAxis));

    if (twoDimensional && thicknessCells != 1)
    {
        ValidationIssue issue(Severity::BlockingError, "CASE-MESH-004",
            "2D 模式下厚度方向（z）的网格数必须为 1（当前为 "
                + std::to_string(thicknessCells) + "）。");
        issue.setTargetField("mesh.cells.z");
        issue.setSuggestedFix("把 nz 设为 1，或把计算域切换为 3D。");
        result.add(std::move(issue));
    }

    if (!twoDimensional && thicknessCells == 1)
    {
        ValidationIssue issue(Severity::Warning, "CASE-MESH-006",
            "3D 模式下 z 方向只有 1 层网格，通常意味着应该使用 2D 模式。");
        issue.setTargetField("mesh.cells.z");
        result.add(std::move(issue));
    }

    if (mesh.exceedsCellBudget())
    {
        ValidationIssue issue(Severity::Warning, "CASE-MESH-007",
            "总网格数 " + std::to_string(mesh.totalCells()) + " 超过建议上限 "
                + std::to_string(MeshSettings::kCellBudgetWarning) + "，导出后求解会很慢。");
        issue.setTargetField("mesh.cells");
        result.add(std::move(issue));
    }
}

} // namespace ofgui::core
