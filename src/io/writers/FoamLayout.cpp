#include "io/writers/FoamLayout.h"

#include <cstddef>

namespace ofgui::io {
namespace {

constexpr std::string_view kBannerLines[] = {
    "/*--------------------------------*- C++ -*----------------------------------*\\\n",
    "| =========                 |                                                 |\n",
    "| \\\\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox           |\n",
    "|  \\\\    /   O peration     | Version:  v2206                                 |\n",
    "|   \\\\  /    A nd           | Website:  www.openfoam.com                      |\n",
    "|    \\\\/     M anipulation  |                                                 |\n",
    "\\*---------------------------------------------------------------------------*/\n",
};

constexpr std::string_view kSeparatorLine =
    "// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //\n";

constexpr std::string_view kTrailerLine =
    "// ************************************************************************* //\n";

constexpr int kAlignment = 16;

} // namespace

std::string foamIndent(int level)
{
    if (level <= 0)
    {
        return std::string{};
    }
    return std::string(static_cast<std::size_t>(level) * 4, ' ');
}

std::string foamEntryLine(int indent, std::string_view keyword, std::string_view value, int valueColumn)
{
    const std::string prefix = foamIndent(indent);
    const std::size_t target = valueColumn >= 0
        ? static_cast<std::size_t>(valueColumn)
        : prefix.size() + static_cast<std::size_t>(kAlignment);

    std::string line = prefix;
    line += keyword;
    if (line.size() + 1 <= target)
    {
        line.append(target - line.size(), ' ');
    }
    else
    {
        line += ' ';
    }
    line += value;
    line += ';';
    return line;
}

const std::string& foamBanner()
{
    static const std::string banner = []
    {
        std::string text;
        for (const std::string_view line : kBannerLines)
        {
            text += line;
        }
        return text;
    }();
    return banner;
}

const std::string& foamSeparator()
{
    static const std::string separator{kSeparatorLine};
    return separator;
}

const std::string& foamTrailer()
{
    static const std::string trailer{kTrailerLine};
    return trailer;
}

std::string foamFileBlock(std::string_view className, std::string_view objectName)
{
    std::string text = "FoamFile\n{\n";
    text += foamEntryLine(1, "version", "2.0", 16);
    text += '\n';
    text += foamEntryLine(1, "format", "ascii", 16);
    text += '\n';
    text += foamEntryLine(1, "class", className, 16);
    text += '\n';
    text += foamEntryLine(1, "object", objectName, 16);
    text += "\n}\n";
    return text;
}

std::string foamFileHeader(std::string_view className, std::string_view objectName)
{
    std::string text = foamBanner();
    text += foamFileBlock(className, objectName);
    text += foamSeparator();
    text += '\n';
    return text;
}

} // namespace ofgui::io
