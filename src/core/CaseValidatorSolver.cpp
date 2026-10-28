#include "core/CaseValidator.h"

#include "core/CaseValidatorMessages.h"

#include <cmath>
#include <string>

// CASE-SOLV-*: time controls and the solver application.

namespace ofgui::core {

void CaseValidator::validateSolver(const CaseProject& project, ValidationResult& result) const
{
    const SolverSettings& solver = project.solver();

    if (!solver.hasValidTimeRange())
    {
        ValidationIssue issue(Severity::Error, "CASE-SOLV-001",
            "时间控制非法：必须满足 endTime > startTime（当前 startTime = "
                + detail::formatReal(solver.startTime()) + "，endTime = "
                + detail::formatReal(solver.endTime()) + "）。");
        issue.setTargetField("solver.endTime");
        issue.setSuggestedFix("增大 endTime 或减小 startTime。");
        result.add(std::move(issue));
    }

    if (!std::isfinite(solver.deltaT()) || !(solver.deltaT() > 0.0))
    {
        ValidationIssue issue(Severity::Error, "CASE-SOLV-002",
            "时间步长 deltaT 必须是正的有限数（当前为 " + detail::formatReal(solver.deltaT())
                + "）。");
        issue.setTargetField("solver.deltaT");
        issue.setSuggestedFix("设置正的 deltaT，例如 0.001。");
        result.add(std::move(issue));
    }

    if (solver.writeInterval() < 1)
    {
        ValidationIssue issue(Severity::Error, "CASE-SOLV-003",
            "writeInterval 必须大于等于 1（当前为 "
                + std::to_string(solver.writeInterval()) + "）。");
        issue.setTargetField("solver.writeInterval");
        result.add(std::move(issue));
    }

    if (solver.application().empty())
    {
        ValidationIssue issue(Severity::Warning, "CASE-SOLV-004",
            "还没有选择求解器 application（controlDict.application）。");
        issue.setTargetField("solver.application");
        issue.setSuggestedFix("选择 icoFoam、simpleFoam 等求解器。");
        result.add(std::move(issue));
    }
}

} // namespace ofgui::core
