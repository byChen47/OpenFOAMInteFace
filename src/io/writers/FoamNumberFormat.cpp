#include "io/writers/FoamNumberFormat.h"

#include <charconv>
#include <cstddef>

namespace ofgui::io {

std::string formatFoamReal(core::Real value)
{
    char buffer[64];
    const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
    if (result.ec != std::errc{})
    {
        return "0";
    }
    return std::string(buffer, result.ptr);
}

std::string formatFoamInt(long long value)
{
    char buffer[32];
    const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
    if (result.ec != std::errc{})
    {
        return "0";
    }
    return std::string(buffer, result.ptr);
}

std::string formatFoamCoordinate(core::Real value)
{
    char buffer[64];
    const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value, std::chars_format::general, 15);
    if (result.ec != std::errc{})
    {
        return formatFoamReal(value);
    }
    return std::string(buffer, result.ptr);
}

std::string formatFoamVector(const core::Vec3d& value)
{
    return "(" + formatFoamReal(value.x) + " " + formatFoamReal(value.y) + " " + formatFoamReal(value.z) + ")";
}

std::string formatFoamPoint(const core::Vec3d& value)
{
    return "(" + formatFoamCoordinate(value.x) + " " + formatFoamCoordinate(value.y) + " "
        + formatFoamCoordinate(value.z) + ")";
}

std::string formatFoamDimensions(const std::array<int, 7>& dimensions)
{
    std::string text = "[";
    for (std::size_t index = 0; index < dimensions.size(); ++index)
    {
        if (index != 0)
        {
            text += ' ';
        }
        text += formatFoamInt(dimensions[index]);
    }
    text += ']';
    return text;
}

} // namespace ofgui::io
