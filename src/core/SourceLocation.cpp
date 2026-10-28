#include "core/SourceLocation.h"

#include <utility>

namespace ofgui::core {

SourceLocation::SourceLocation(std::string file, int line, int column)
    : file_(std::move(file))
    , line_(line)
    , column_(column)
{
}

const std::string& SourceLocation::file() const noexcept
{
    return file_;
}

void SourceLocation::setFile(std::string file)
{
    file_ = std::move(file);
}

int SourceLocation::line() const noexcept
{
    return line_;
}

void SourceLocation::setLine(int line) noexcept
{
    line_ = line;
}

int SourceLocation::column() const noexcept
{
    return column_;
}

void SourceLocation::setColumn(int column) noexcept
{
    column_ = column;
}

bool SourceLocation::isValid() const noexcept
{
    return !file_.empty();
}

std::string SourceLocation::toString() const
{
    if (file_.empty())
    {
        return {};
    }

    std::string text = file_;
    if (line_ > 0)
    {
        text += ':' + std::to_string(line_);
        if (column_ > 0)
        {
            text += ':' + std::to_string(column_);
        }
    }

    return text;
}

} // namespace ofgui::core
