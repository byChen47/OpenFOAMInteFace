#include "io/writers/ExportAction.h"

#include <array>
#include <utility>

namespace ofgui::io {
namespace {

constexpr std::array<std::pair<ExportAction, const char*>, 4> kActionNames{{
    {ExportAction::Written, "written"},
    {ExportAction::Copied, "copied"},
    {ExportAction::Mirrored, "mirrored"},
    {ExportAction::Skipped, "skipped"},
}};

} // namespace

std::string_view exportActionName(ExportAction action) noexcept
{
    for (const auto& [value, name] : kActionNames)
    {
        if (value == action)
        {
            return name;
        }
    }
    return "written";
}

ExportAction exportActionFromName(std::string_view name) noexcept
{
    for (const auto& [value, spelling] : kActionNames)
    {
        if (name == spelling)
        {
            return value;
        }
    }
    return ExportAction::Written;
}

bool exportActionWritesFile(ExportAction action) noexcept
{
    return action != ExportAction::Skipped;
}

} // namespace ofgui::io
