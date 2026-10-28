#include "io/writers/TransportPropertiesWriter.h"

#include "io/writers/FoamLayout.h"
#include "io/writers/FoamNumberFormat.h"

#include <vector>

namespace ofgui::io {
namespace {

constexpr int kTopLevel = 0;

} // namespace

std::string TransportPropertiesWriter::write(const TransportModelSettings& settings)
{
    std::vector<std::pair<std::string, std::string>> entries;
    if (!settings.transportModel.empty())
    {
        entries.emplace_back("transportModel", settings.transportModel);
    }

    std::string viscosity = settings.kinematicViscosityText;
    if (viscosity.empty())
    {
        viscosity = formatFoamReal(settings.kinematicViscosity);
    }
    entries.emplace_back("nu", viscosity);

    std::string text = foamFileHeader("dictionary", "transportProperties");
    for (const auto& entry : entries)
    {
        text += foamEntryLine(kTopLevel, entry.first, entry.second);
        text += '\n';
        text += '\n';
    }
    text += '\n';
    text += foamTrailer();
    return text;
}

} // namespace ofgui::io
