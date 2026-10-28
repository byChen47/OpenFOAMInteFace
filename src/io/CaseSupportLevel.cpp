#include "io/CaseSupportLevel.h"

namespace ofgui::io {

std::string_view supportLevelName(SupportLevel level) noexcept
{
    switch (level)
    {
    case SupportLevel::Editable:
        return "editable";
    case SupportLevel::ReadOnly:
        return "readOnly";
    case SupportLevel::Recognized:
        return "recognized";
    case SupportLevel::Unsupported:
        return "unsupported";
    }
    return "unsupported";
}

SupportLevel supportLevelFromName(std::string_view name) noexcept
{
    if (name == "editable")
    {
        return SupportLevel::Editable;
    }
    if (name == "readOnly")
    {
        return SupportLevel::ReadOnly;
    }
    if (name == "recognized")
    {
        return SupportLevel::Recognized;
    }
    return SupportLevel::Unsupported;
}

bool supportLevelIsEditable(SupportLevel level) noexcept
{
    return level == SupportLevel::Editable;
}

} // namespace ofgui::io
