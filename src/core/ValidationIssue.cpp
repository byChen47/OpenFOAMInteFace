#include "core/ValidationIssue.h"

#include <utility>

namespace ofgui::core {

std::string_view severityName(Severity severity) noexcept
{
    switch (severity)
    {
    case Severity::Info:
        return "Info";
    case Severity::Warning:
        return "Warning";
    case Severity::Error:
        return "Error";
    case Severity::BlockingError:
        return "BlockingError";
    }

    return "Info";
}

ValidationIssue::ValidationIssue(Severity severity, std::string code, std::string message)
    : severity_(severity)
    , code_(std::move(code))
    , message_(std::move(message))
{
}

Severity ValidationIssue::severity() const noexcept
{
    return severity_;
}

void ValidationIssue::setSeverity(Severity severity) noexcept
{
    severity_ = severity;
}

const std::string& ValidationIssue::code() const noexcept
{
    return code_;
}

void ValidationIssue::setCode(std::string code)
{
    code_ = std::move(code);
}

const std::string& ValidationIssue::message() const noexcept
{
    return message_;
}

void ValidationIssue::setMessage(std::string message)
{
    message_ = std::move(message);
}

const std::optional<ObjectId>& ValidationIssue::targetObjectId() const noexcept
{
    return targetObjectId_;
}

void ValidationIssue::setTargetObjectId(ObjectId objectId)
{
    targetObjectId_ = objectId;
}

void ValidationIssue::clearTargetObjectId() noexcept
{
    targetObjectId_.reset();
}

const std::string& ValidationIssue::targetField() const noexcept
{
    return targetField_;
}

void ValidationIssue::setTargetField(std::string targetField)
{
    targetField_ = std::move(targetField);
}

const std::string& ValidationIssue::suggestedFix() const noexcept
{
    return suggestedFix_;
}

void ValidationIssue::setSuggestedFix(std::string suggestedFix)
{
    suggestedFix_ = std::move(suggestedFix);
}

const std::optional<SourceLocation>& ValidationIssue::location() const noexcept
{
    return location_;
}

void ValidationIssue::setLocation(SourceLocation location)
{
    location_ = std::move(location);
}

void ValidationIssue::clearLocation() noexcept
{
    location_.reset();
}

const std::string& ValidationIssue::nodePath() const noexcept
{
    return nodePath_;
}

void ValidationIssue::setNodePath(std::string nodePath)
{
    nodePath_ = std::move(nodePath);
}

const std::string& ValidationIssue::snippet() const noexcept
{
    return snippet_;
}

void ValidationIssue::setSnippet(std::string snippet)
{
    snippet_ = std::move(snippet);
}

const std::string& ValidationIssue::origin() const noexcept
{
    return origin_;
}

void ValidationIssue::setOrigin(std::string origin)
{
    origin_ = std::move(origin);
}

bool ValidationIssue::isBlocking() const noexcept
{
    return severity_ == Severity::BlockingError;
}

bool ValidationIssue::isAtLeast(Severity other) const noexcept
{
    return static_cast<int>(severity_) >= static_cast<int>(other);
}

std::string ValidationIssue::format() const
{
    std::string text;
    text.reserve(code_.size() + message_.size() + 64);
    text += severityName(severity_);
    text += ' ';
    text += code_;
    text += ": ";
    text += message_;

    if (!targetField_.empty())
    {
        text += " [field: ";
        text += targetField_;
        text += ']';
    }

    if (location_ && location_->isValid())
    {
        text += " [at: ";
        text += location_->toString();
        text += ']';
    }

    if (!nodePath_.empty())
    {
        text += " [node: ";
        text += nodePath_;
        text += ']';
    }

    if (!suggestedFix_.empty())
    {
        text += " [fix: ";
        text += suggestedFix_;
        text += ']';
    }

    return text;
}

} // namespace ofgui::core
