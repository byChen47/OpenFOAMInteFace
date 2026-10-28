#pragma once

#include <string>

namespace ofgui::core {

/// Position of a problem inside a source file (dictionary / project file).
///
/// Import and export issues carry a location so the GUI can jump to the file,
/// line and column instead of showing a context free error text (case import
/// specification section 8). Line and column are 1 based; `0` means unknown.
class SourceLocation
{
public:
    SourceLocation() = default;

    SourceLocation(std::string file, int line = 0, int column = 0);

    [[nodiscard]] const std::string& file() const noexcept;
    void setFile(std::string file);

    [[nodiscard]] int line() const noexcept;
    void setLine(int line) noexcept;

    [[nodiscard]] int column() const noexcept;
    void setColumn(int column) noexcept;

    [[nodiscard]] bool isValid() const noexcept;

    /// `file`, `file:line` or `file:line:column`, depending on what is known.
    [[nodiscard]] std::string toString() const;

private:
    std::string file_;
    int line_ = 0;
    int column_ = 0;
};

} // namespace ofgui::core
