#pragma once

#include "io/FoamDiagnostic.h"
#include "io/FoamNode.h"
#include "io/FoamParseOptions.h"

#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace ofgui::io {

class FoamParser;

/// A parsed OpenFOAM dictionary file: ordered AST + diagnostics + original text.
///
/// The document is the parser's only public entry point. It keeps the byte-exact
/// source text alongside the tree so a loaded case can be written back unchanged
/// (`sourceText()`), while `toText()` renders the canonical form from the AST.
/// Neither method touches the file system: callers decide where bytes go.
class FoamDictionary
{
public:
    FoamDictionary();
    ~FoamDictionary();

    FoamDictionary(FoamDictionary&&) noexcept;
    FoamDictionary& operator=(FoamDictionary&&) noexcept;
    FoamDictionary(const FoamDictionary&) = delete;
    FoamDictionary& operator=(const FoamDictionary&) = delete;

    /// Parse an in-memory dictionary. `fileLabel` is used in diagnostics only.
    static FoamDictionary parse(
        std::string_view source,
        std::string fileLabel = std::string{},
        const FoamParseOptions& options = FoamParseOptions{});

    /// Read and parse a dictionary file.
    ///
    /// Rejects UTF-16/binary content and files above `maxFileBytes`, strips a
    /// UTF-8 BOM (reporting `IMPORT-ENC-003`), and keeps the first 200 issues.
    /// This is the temporary byte reader for the parser: once `TextFileReader`
    /// exists it becomes the single place that touches the file system.
    static FoamDictionary parseFile(
        const std::filesystem::path& path,
        const FoamParseOptions& options = FoamParseOptions{});

    /// Parsed without any `Error` diagnostic.
    bool ok() const;
    bool isEmpty() const;

    const std::string& fileLabel() const noexcept;
    /// Byte-exact source text handed to the parser (UTF-8 BOM already stripped).
    const std::string& sourceText() const noexcept;

    const FoamNode& root() const noexcept;
    const FoamNode* header() const noexcept;
    bool hasFoamFileHeader() const noexcept;

    const std::vector<FoamDiagnostic>& diagnostics() const noexcept;
    /// One line per diagnostic, suitable for a log/console view.
    std::string diagnosticsText() const;

    /// Look up `boundaryField/movingWall/type` (both `/` and `.` separate keys).
    /// Returns the value node of the entry, or nullptr.
    const FoamNode* find(std::string_view path) const;

    /// All direct children of `dictionaryPath` whose keyword equals `keyword`.
    /// Duplicate keywords are legal in OpenFOAM, so this returns every match.
    std::vector<const FoamNode*> findAll(
        std::string_view dictionaryPath, std::string_view keyword) const;

    /// Depth-first pre-order walk over dictionary entries.
    /// `path` is dotted (`boundaryField.movingWall.type`).
    void visit(const std::function<void(const FoamNode& node, std::string_view path)>& visitor) const;

    /// Canonical text emitted from the AST: LF line endings, four-space indent,
    /// `key    value;`, dictionaries as `key\\n{\\n...}\\n`. Comments and the
    /// original banner are not part of the AST and are therefore not emitted.
    std::string toText() const;

private:
    friend class FoamParser;

    FoamDictionary(
        std::string sourceText,
        std::string fileLabel,
        std::unique_ptr<FoamNode> root,
        std::vector<FoamDiagnostic> diagnostics);

    std::string sourceText_;
    std::string fileLabel_;
    std::unique_ptr<FoamNode> root_;
    std::vector<FoamDiagnostic> diagnostics_;
};

} // namespace ofgui::io
