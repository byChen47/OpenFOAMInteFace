#pragma once

#include "io/CaseProjectionContext.h"
#include "io/ImportedSetting.h"

#include <string>
#include <vector>

namespace ofgui::io {

class FoamDictionary;

/// Result of projecting `constant/transportProperties`.
struct TransportPropertiesProjection
{
    /// `transportModel` verbatim (`Newtonian`, `BinghamPlastic`, ...); empty when
    /// the keyword is missing.
    std::string transportModel;
    bool hasTransportModel = false;
    /// `nu` as a number when it is written directly (`nu 0.01;`).
    bool hasKinematicViscosity = false;
    double kinematicViscosity = 0.0;
    /// True when `nu` carried a `[dimensions]` prefix; the value itself is kept in
    /// `settings` and stays read only.
    bool kinematicViscosityHasDimensions = false;
    /// `nu` as raw text when it could not be interpreted as one number.
    std::string kinematicViscosityText;
    std::vector<ImportedSetting> settings;
};

/// Projects `constant/transportProperties`: `transportModel` and `nu` are the
/// only editable values today (spec section 4.3), everything else is read only.
class TransportPropertiesProjector
{
public:
    [[nodiscard]] static TransportPropertiesProjection project(
        const FoamDictionary& dictionary,
        const CaseProjectionContext& context);
};

} // namespace ofgui::io
