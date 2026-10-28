#pragma once

#include <string_view>

namespace ofgui::io {

/// How much of a case file the importer puts into the editable model.
///
/// The four levels follow `docs/CASE_IMPORT_SPEC.md` section 3: `Editable`
/// files are projected into the model, `ReadOnly` files are parsed but only
/// shown/exported verbatim, `Recognized` files are listed without being read and
/// `Unsupported` files cannot be parsed and are mirrored on export.
enum class SupportLevel
{
    Editable = 0,
    ReadOnly = 1,
    Recognized = 2,
    Unsupported = 3
};

/// `editable`, `readOnly`, `recognized` or `unsupported`.
[[nodiscard]] std::string_view supportLevelName(SupportLevel level) noexcept;

/// Parses the spelling produced by `supportLevelName`.
[[nodiscard]] SupportLevel supportLevelFromName(std::string_view name) noexcept;

/// True for `Editable` (the only level the GUI may write back).
[[nodiscard]] bool supportLevelIsEditable(SupportLevel level) noexcept;

} // namespace ofgui::io
