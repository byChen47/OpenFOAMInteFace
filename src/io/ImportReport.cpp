#include "io/ImportReport.h"

#include <algorithm>

namespace ofgui::io {

bool ImportReport::blocking() const
{
    return std::any_of(issues.begin(), issues.end(), [](const core::ValidationIssue& issue) {
        return issue.severity() == core::Severity::BlockingError;
    });
}

bool ImportReport::hasCode(std::string_view code) const
{
    return std::any_of(issues.begin(), issues.end(), [code](const core::ValidationIssue& issue) {
        return issue.code() == code;
    });
}

std::size_t ImportReport::countCode(std::string_view code) const
{
    return static_cast<std::size_t>(std::count_if(
        issues.begin(), issues.end(), [code](const core::ValidationIssue& issue) {
            return issue.code() == code;
        }));
}

std::size_t ImportReport::countFiles(SupportLevel level) const
{
    return static_cast<std::size_t>(std::count_if(
        files.begin(), files.end(), [level](const ImportedFile& file) { return file.level == level; }));
}

std::size_t ImportReport::partialCount() const
{
    return static_cast<std::size_t>(
        std::count_if(files.begin(), files.end(), [](const ImportedFile& file) { return file.partial; }));
}

std::vector<std::string> ImportReport::pathsWithLevel(SupportLevel level) const
{
    std::vector<std::string> paths;
    for (const ImportedFile& file : files)
    {
        if (file.level == level)
        {
            paths.push_back(file.relativePath);
        }
    }
    return paths;
}

const ImportedFile* ImportReport::findFile(std::string_view relativePath) const
{
    const auto found = std::find_if(files.begin(), files.end(), [relativePath](const ImportedFile& file) {
        return file.relativePath == relativePath;
    });
    return found == files.end() ? nullptr : &*found;
}

} // namespace ofgui::io
