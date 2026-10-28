#include "core/ValidationResult.h"

#include <algorithm>
#include <utility>

namespace ofgui::core {

void ValidationResult::add(ValidationIssue issue)
{
    issues_.push_back(std::move(issue));
}

void ValidationResult::addAll(const ValidationResult& other)
{
    issues_.insert(issues_.end(), other.issues_.begin(), other.issues_.end());
}

void ValidationResult::clear() noexcept
{
    issues_.clear();
}

const std::vector<ValidationIssue>& ValidationResult::issues() const noexcept
{
    return issues_;
}

bool ValidationResult::empty() const noexcept
{
    return issues_.empty();
}

std::size_t ValidationResult::size() const noexcept
{
    return issues_.size();
}

std::size_t ValidationResult::count(Severity severity) const noexcept
{
    return static_cast<std::size_t>(std::count_if(issues_.begin(), issues_.end(),
        [severity](const ValidationIssue& issue) { return issue.severity() == severity; }));
}

bool ValidationResult::hasBlockingErrors() const noexcept
{
    return firstBlockingError() != nullptr;
}

bool ValidationResult::hasErrors() const noexcept
{
    return std::any_of(issues_.begin(), issues_.end(),
        [](const ValidationIssue& issue) { return issue.isAtLeast(Severity::Error); });
}

const ValidationIssue* ValidationResult::firstBlockingError() const noexcept
{
    const auto found = std::find_if(issues_.begin(), issues_.end(),
        [](const ValidationIssue& issue) { return issue.isBlocking(); });
    return (found == issues_.end()) ? nullptr : &*found;
}

std::vector<ValidationIssue> ValidationResult::issuesWithSeverityAtLeast(Severity severity) const
{
    std::vector<ValidationIssue> filtered;
    for (const ValidationIssue& issue : issues_)
    {
        if (issue.isAtLeast(severity))
        {
            filtered.push_back(issue);
        }
    }

    return filtered;
}

std::string ValidationResult::summary() const
{
    const auto blocking = count(Severity::BlockingError);
    const auto errors = count(Severity::Error);
    const auto warnings = count(Severity::Warning);
    const auto infos = count(Severity::Info);

    return "阻断 " + std::to_string(blocking) + "、错误 " + std::to_string(errors)
           + "、警告 " + std::to_string(warnings) + "、提示 " + std::to_string(infos);
}

} // namespace ofgui::core
