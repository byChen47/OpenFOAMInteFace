#include "io/ControlDictProjector.h"

#include "io/FoamDictionary.h"
#include "io/FoamNode.h"
#include "io/FoamToken.h"
#include "io/FoamValue.h"
#include "io/ImportCodes.h"
#include "io/ImportIssueFactory.h"
#include "io/ReadOnlyDictionaryProjector.h"

#include <string>

namespace ofgui::io {
namespace {

std::string textOf(const FoamNode* node)
{
    if (node == nullptr)
    {
        return {};
    }
    return node->isLeaf() ? node->rawText() : FoamValue(*node).text();
}

bool readReal(const FoamNode* node, double& value)
{
    return node != nullptr && node->isLeaf() && parseFoamReal(node->rawText(), value);
}

bool readInt(const FoamNode* node, int& value)
{
    long long parsed = 0;
    if (node == nullptr || !node->isLeaf() || !parseFoamInt(node->rawText(), parsed))
    {
        return false;
    }
    if (parsed < -2147483647LL - 1LL || parsed > 2147483647LL)
    {
        return false;
    }
    value = static_cast<int>(parsed);
    return true;
}

} // namespace

ControlDictProjection ControlDictProjector::project(
    const FoamDictionary& dictionary, const CaseProjectionContext& context)
{
    ControlDictProjection projection;
    const std::string& file = context.relativePath;

    if (const FoamNode* application = dictionary.find("application"))
    {
        projection.solver.setApplication(textOf(application));
        projection.hasApplication = true;
    }

    double startTime = 0.0;
    if (readReal(dictionary.find("startTime"), startTime))
    {
        projection.solver.setStartTime(startTime);
        projection.hasTiming = true;
    }
    double endTime = 0.0;
    if (readReal(dictionary.find("endTime"), endTime) && endTime > 0.0)
    {
        projection.solver.setEndTime(endTime);
        projection.hasTiming = true;
    }
    double deltaT = 0.0;
    if (readReal(dictionary.find("deltaT"), deltaT) && deltaT > 0.0)
    {
        projection.solver.setDeltaT(deltaT);
        projection.hasTiming = true;
    }
    int writeInterval = 0;
    if (readInt(dictionary.find("writeInterval"), writeInterval) && writeInterval >= 0)
    {
        projection.solver.setWriteInterval(writeInterval);
    }

    if (const FoamNode* functions = dictionary.find("functions"); functions != nullptr)
    {
        context.issues.push_back(makeImportIssue(importcodes::kUnsupportedSubtree,
                                                file,
                                                functions->location().line,
                                                functions->location().column,
                                                "functions",
                                                textOf(functions)));
    }

    projection.settings = ReadOnlyDictionaryProjector::flatten(dictionary, file);
    return projection;
}

} // namespace ofgui::io
