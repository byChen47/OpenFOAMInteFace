#include "io/ImportReportBookkeeping.h"

#include <algorithm>
#include <string>

namespace ofgui::io::detail {
namespace {

using core::Severity;
using core::ValidationIssue;

int severityRank(Severity severity) noexcept
{
    switch (severity)
    {
    case Severity::Info:
        return 0;
    case Severity::Warning:
        return 1;
    case Severity::Error:
        return 2;
    case Severity::BlockingError:
        return 3;
    }
    return 0;
}

} // namespace

void sortIssues(std::vector<ValidationIssue>& issues)
{
    std::stable_sort(issues.begin(), issues.end(), [](const ValidationIssue& left, const ValidationIssue& right) {
        const int leftRank = severityRank(left.severity());
        const int rightRank = severityRank(right.severity());
        if (leftRank != rightRank)
        {
            return leftRank > rightRank;
        }
        const std::string leftFile = left.location().has_value() ? left.location()->file() : std::string{};
        const std::string rightFile = right.location().has_value() ? right.location()->file() : std::string{};
        if (leftFile != rightFile)
        {
            return leftFile < rightFile;
        }
        const int leftLine = left.location().has_value() ? left.location()->line() : 0;
        const int rightLine = right.location().has_value() ? right.location()->line() : 0;
        if (leftLine != rightLine)
        {
            return leftLine < rightLine;
        }
        return left.code() < right.code();
    });
}

ImportedFile* findFile(ImportReport& report, std::string_view relativePath)
{
    for (ImportedFile& file : report.files)
    {
        if (file.relativePath == relativePath)
        {
            return &file;
        }
    }
    return nullptr;
}

SupportLevel defaultLevel(CaseEntryKind kind) noexcept
{
    switch (kind)
    {
    case CaseEntryKind::Field:
        return SupportLevel::ReadOnly;
    case CaseEntryKind::Dictionary:
    case CaseEntryKind::MeshDirectory:
    case CaseEntryKind::Script:
    case CaseEntryKind::Other:
        return SupportLevel::Recognized;
    }
    return SupportLevel::Recognized;
}

} // namespace ofgui::io::detail
