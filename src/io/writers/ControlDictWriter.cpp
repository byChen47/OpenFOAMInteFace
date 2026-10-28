#include "io/writers/ControlDictWriter.h"

#include "io/writers/FoamLayout.h"
#include "io/writers/FoamNumberFormat.h"

#include <string>
#include <utility>
#include <vector>

namespace ofgui::io {
namespace {

constexpr int kTopLevel = 0;

} // namespace

std::string ControlDictWriter::write(const core::CaseProject& project)
{
    const core::SolverSettings& solver = project.solver();

    std::vector<std::pair<std::string, std::string>> entries;
    entries.emplace_back("application", solver.application().empty() ? "icoFoam" : solver.application());
    entries.emplace_back("startFrom", "startTime");
    entries.emplace_back("startTime", formatFoamReal(solver.startTime()));
    entries.emplace_back("stopAt", "endTime");
    entries.emplace_back("endTime", formatFoamReal(solver.endTime()));
    entries.emplace_back("deltaT", formatFoamReal(solver.deltaT()));
    entries.emplace_back("writeControl", "timeStep");
    entries.emplace_back("writeInterval", formatFoamInt(solver.writeInterval()));
    entries.emplace_back("purgeWrite", "0");
    entries.emplace_back("writeFormat", "ascii");
    entries.emplace_back("writePrecision", "6");
    entries.emplace_back("writeCompression", "off");
    entries.emplace_back("timeFormat", "general");
    entries.emplace_back("timePrecision", "6");
    entries.emplace_back("runTimeModifiable", "true");

    std::string text = foamFileHeader("dictionary", "controlDict");
    for (std::size_t index = 0; index < entries.size(); ++index)
    {
        text += foamEntryLine(kTopLevel, entries[index].first, entries[index].second);
        text += '\n';
        text += '\n';
    }
    text += '\n';
    text += foamTrailer();
    return text;
}

} // namespace ofgui::io
