#include "io/CaseNameRules.h"

#include <charconv>
#include <system_error>

namespace ofgui::io::detail {

bool isNumericName(std::string_view name) noexcept
{
    if (name.empty())
    {
        return false;
    }
    double value = 0.0;
    const char* const begin = name.data();
    const char* const end = begin + name.size();
    const auto result = std::from_chars(begin, end, value);
    return result.ec == std::errc{} && result.ptr == end;
}

CaseEntryKind kindForSystemFile(std::string_view name) noexcept
{
    if (name == "PDRblockMeshDict")
    {
        return CaseEntryKind::Other;
    }
    return CaseEntryKind::Dictionary;
}

} // namespace ofgui::io::detail
