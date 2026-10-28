#pragma once

#include <string>

namespace ofgui::io {

/// One keyword/value pair the importer keeps as read-only text.
///
/// `path` uses `<case relative file>.<dictionary path>`, for example
/// `system/fvSchemes.ddtSchemes.default`, so the read-only panel can show where
/// the value came from and the exporter can mirror it unchanged.
struct ImportedSetting
{
    std::string path;
    std::string text;
};

} // namespace ofgui::io
