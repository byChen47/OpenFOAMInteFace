#pragma once

#include "io/writers/TransportModelSettings.h"

#include <string>

namespace ofgui::io {

/// Writes `constant/transportProperties`.
class TransportPropertiesWriter
{
public:
    static std::string write(const TransportModelSettings& settings);
};

} // namespace ofgui::io
