#pragma once

#include "core/ValidationIssue.h"

#include <cstddef>
#include <string>
#include <vector>

namespace ofgui::core {

/// Ordered bag of `ValidationIssue`s produced by a validator pass.
///
/// Issues are kept in insertion order because the panels show them as reported
/// (file order, then dictionary order). `hasBlockingErrors()` is the single
/// switch the export button listens to.
class ValidationResult
{
public:
    ValidationResult() = default;

    void add(ValidationIssue issue);
    void addAll(const ValidationResult& other);
    void clear() noexcept;

    [[nodiscard]] const std::vector<ValidationIssue>& issues() const noexcept;

    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;

    [[nodiscard]] std::size_t count(Severity severity) const noexcept;

    /// True when at least one `BlockingError` exists.
    [[nodiscard]] bool hasBlockingErrors() const noexcept;

    /// True when at least one `Error` or `BlockingError` exists.
    [[nodiscard]] bool hasErrors() const noexcept;

    [[nodiscard]] const ValidationIssue* firstBlockingError() const noexcept;

    [[nodiscard]] std::vector<ValidationIssue> issuesWithSeverityAtLeast(Severity severity) const;

    /// `阻断 1、错误 2、警告 0、提示 3` - used by the status bar and tests.
    [[nodiscard]] std::string summary() const;

private:
    std::vector<ValidationIssue> issues_;
};

} // namespace ofgui::core
