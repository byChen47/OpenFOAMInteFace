#include "io/TransportPropertiesProjector.h"

#include "io/FoamDictionary.h"
#include "io/FoamNode.h"
#include "io/FoamToken.h"
#include "io/FoamValue.h"
#include "io/ReadOnlyDictionaryProjector.h"

#include <string>

namespace ofgui::io {
namespace {

bool readReal(const FoamNode* node, double& value)
{
    return node != nullptr && node->isLeaf() && parseFoamReal(node->rawText(), value);
}

} // namespace

TransportPropertiesProjection TransportPropertiesProjector::project(
    const FoamDictionary& dictionary, const CaseProjectionContext& context)
{
    TransportPropertiesProjection projection;

    if (const FoamNode* model = dictionary.find("transportModel"))
    {
        projection.transportModel = model->rawText();
        projection.hasTransportModel = !projection.transportModel.empty();
    }

    const FoamNode* nu = dictionary.find("nu");
    if (nu != nullptr)
    {
        if (double value = 0.0; readReal(nu, value))
        {
            projection.hasKinematicViscosity = true;
            projection.kinematicViscosity = value;
        }
        else if (nu->isSequence() && nu->childCount() >= 2)
        {
            // `nu [0 2 -1 0 0 0 0] 0.01;` keeps the dimension set as read-only text
            // and exposes the trailing number so the GUI can still show it.
            projection.kinematicViscosityHasDimensions = true;
            projection.kinematicViscosityText = FoamValue(*nu).text();
            const FoamNode* last = nu->childAt(nu->childCount() - 1);
            if (double value = 0.0; readReal(last, value))
            {
                projection.hasKinematicViscosity = true;
                projection.kinematicViscosity = value;
            }
        }
        else
        {
            projection.kinematicViscosityText = FoamValue(*nu).text();
        }
    }

    projection.settings = ReadOnlyDictionaryProjector::flatten(dictionary, context.relativePath);
    return projection;
}

} // namespace ofgui::io
