#pragma once

#include <string>
#include <string_view>

namespace ofgui::io {

/// 4 spaces per nesting level.
std::string foamIndent(int level);

/// `key    value;` with the value column at `indent + 16` unless
/// `valueColumn` is given (blockMeshDict patch bodies use a single space).
std::string foamEntryLine(int indent, std::string_view keyword, std::string_view value, int valueColumn = -1);

/// The 7 line banner shared by every generated dictionary.
const std::string& foamBanner();

/// The `// * * * * ... * * //` separator that follows the FoamFile block.
const std::string& foamSeparator();

/// The `// ***** ... ***** //` trailer written at the end of a dictionary.
const std::string& foamTrailer();

/// `FoamFile { version 2.0; format ascii; class <class>; object <object>; }`
std::string foamFileBlock(std::string_view className, std::string_view objectName);

/// Banner + FoamFile block + separator + blank line.
std::string foamFileHeader(std::string_view className, std::string_view objectName);

} // namespace ofgui::io
