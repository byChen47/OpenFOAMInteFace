#pragma once

#include "core/ObjectId.h"
#include "core/SourceLocation.h"

#include <optional>
#include <string>
#include <string_view>

namespace ofgui::core {

/// Severity of a validation issue. Only `BlockingError` stops export.
enum class Severity
{
    Info = 0,
    Warning = 1,
    Error = 2,
    BlockingError = 3
};

/// `Info`, `Warning`, `Error`, `BlockingError`.
[[nodiscard]] std::string_view severityName(Severity severity) noexcept;

/// One validation finding.
///
/// The first six fields follow `docs/ARCHITECTURE.md` section 8.2. The last four
/// (`location`, `nodePath`, `snippet`, `origin`) were added by
/// `docs/DECISIONS.md` D-017 for import/export issues and always default to
/// empty, so model level findings and import findings share one type.
class ValidationIssue
{
public:
    ValidationIssue() = default;

    ValidationIssue(Severity severity, std::string code, std::string message);

    [[nodiscard]] Severity severity() const noexcept;
    void setSeverity(Severity severity) noexcept;

    [[nodiscard]] const std::string& code() const noexcept;
    void setCode(std::string code);

    [[nodiscard]] const std::string& message() const noexcept;
    void setMessage(std::string message);

    [[nodiscard]] const std::optional<ObjectId>& targetObjectId() const noexcept;
    void setTargetObjectId(ObjectId objectId);
    void clearTargetObjectId() noexcept;

    /// Model field the issue belongs to, e.g. `domain.max` or `patches[2].name`.
    [[nodiscard]] const std::string& targetField() const noexcept;
    void setTargetField(std::string targetField);

    [[nodiscard]] const std::string& suggestedFix() const noexcept;
    void setSuggestedFix(std::string suggestedFix);

    [[nodiscard]] const std::optional<SourceLocation>& location() const noexcept;
    void setLocation(SourceLocation location);
    void clearLocation() noexcept;

    [[nodiscard]] const std::string& nodePath() const noexcept;
    void setNodePath(std::string nodePath);

    [[nodiscard]] const std::string& snippet() const noexcept;
    void setSnippet(std::string snippet);

    /// `model`, `import` or `export`; lets the export panel filter by origin.
    [[nodiscard]] const std::string& origin() const noexcept;
    void setOrigin(std::string origin);

    /// True for `BlockingError` (the only severity that stops export).
    [[nodiscard]] bool isBlocking() const noexcept;

    /// True when this issue is at least as severe as `other`.
    [[nodiscard]] bool isAtLeast(Severity other) const noexcept;

    /// Single line text used by logs and tests:
    /// `BlockingError CASE-DOM-001: message [field: domain.max] [at: ...] [fix: ...]`.
    [[nodiscard]] std::string format() const;

private:
    Severity severity_ = Severity::Info;
    std::string code_;
    std::string message_;
    std::optional<ObjectId> targetObjectId_;
    std::string targetField_;
    std::string suggestedFix_;
    std::optional<SourceLocation> location_;
    std::string nodePath_;
    std::string snippet_;
    std::string origin_ = "model";
};

} // namespace ofgui::core
