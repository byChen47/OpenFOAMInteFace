#include "core/CaseValidator.h"

#include "core/CaseValidatorMessages.h"

#include <string>

// CASE-DOM-*: geometry of the computational domain.

namespace ofgui::core {

void CaseValidator::validateDomain(const CaseProject& project, ValidationResult& result) const
{
    const DomainBox& domain = project.domain();

    if (!domain.isFinite())
    {
        result.add(detail::makeIssue(Severity::BlockingError, "CASE-DOM-002",
            "计算域包含非有限数值（NaN 或 infinity）。", "domain"));
        return;
    }

    const Vec3d minimum = domain.min();
    const Vec3d maximum = domain.max();
    for (std::size_t axis = 0; axis < detail::kAxisNames.size(); ++axis)
    {
        if (!(maximum.component(axis) > minimum.component(axis)))
        {
            ValidationIssue issue(Severity::BlockingError, "CASE-DOM-001",
                std::string("计算域 ") + detail::kAxisNames[axis] + " 轴必须满足 max > min（当前 min = "
                    + detail::formatReal(minimum.component(axis)) + "，max = "
                    + detail::formatReal(maximum.component(axis)) + "）。");
            issue.setTargetField("domain.max");
            issue.setSuggestedFix(std::string("增大 ") + detail::kAxisNames[axis]
                + " 方向的最大点，或减小最小点。");
            result.add(std::move(issue));
        }
    }
}

} // namespace ofgui::core
