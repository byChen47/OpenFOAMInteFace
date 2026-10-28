#pragma once

#include <string_view>

namespace ofgui::io {

/// What the exporter does with one file of the exported case.
///
/// `Written` files are generated from the model by a dictionary writer,
/// `Copied` files are recognized but preserved byte for byte (read only
/// dictionaries and fields the model does not own) and `Mirrored` files are
/// opaque source files this tool never interprets (`constant/polyMesh/**`).
enum class ExportAction
{
    Written = 0,
    Copied = 1,
    Mirrored = 2,
    Skipped = 3
};

/// `written`, `copied`, `mirrored` or `skipped`.
///
/// These are the spellings used by the `action` field of `.ofgport-export.json`
/// (`docs/CASE_IMPORT_SPEC.md` section 9.5).
[[nodiscard]] std::string_view exportActionName(ExportAction action) noexcept;

/// Parses the spelling produced by `exportActionName`; `Written` when unknown.
[[nodiscard]] ExportAction exportActionFromName(std::string_view name) noexcept;

/// True for the actions that put a file into the exported case.
[[nodiscard]] bool exportActionWritesFile(ExportAction action) noexcept;

} // namespace ofgui::io
