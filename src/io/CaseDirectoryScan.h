#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace ofgui::io {

/// What a scanned file is, decided from its location inside the case only.
///
/// The kind is a cheap classification by path (`0/p` is a field, `system/*` is
/// a dictionary); the importer refines it with the `FoamFile.class` it parses.
enum class CaseEntryKind
{
    Field = 0,
    Dictionary = 1,
    MeshDirectory = 2,
    Script = 3,
    Other = 4
};

/// `field`, `dictionary`, `mesh-dir`, `script` or `other`.
[[nodiscard]] std::string_view caseEntryKindName(CaseEntryKind kind) noexcept;

/// One file or directory found while walking a case root.
///
/// Paths are case relative and use `/` on every platform so that issue messages,
/// the import report and the project file agree.
struct CaseScannedEntry
{
    std::string relativePath;
    CaseEntryKind kind = CaseEntryKind::Other;
    /// Size in bytes; `0` for directories and unreadable entries.
    std::uintmax_t byteSize = 0;
    /// True when the entry itself ends in `.gz`.
    bool gzip = false;
    /// True when the entry is a symbolic link (never followed, only listed).
    bool symlink = false;
    /// True for directories.
    bool directory = false;
    /// True when the file lives in a time directory other than the start one.
    bool otherTimeDirectory = false;
};

/// Result of `CaseScanner::scan`.
struct CaseDirectoryScan
{
    /// Absolute or user supplied root, exactly as passed in (for UI display only).
    std::string caseRoot;
    /// `0`, `0.orig` or empty when neither directory exists.
    std::string startDirectory;
    bool hasCaseRoot = false;
    bool hasStartDirectory = false;
    bool hasConstantDirectory = false;
    bool hasSystemDirectory = false;
    bool hasControlDict = false;
    bool hasBlockMeshDict = false;
    bool hasFvSchemes = false;
    bool hasFvSolution = false;
    bool hasDecomposeParDict = false;
    bool hasTransportProperties = false;
    bool hasTurbulenceProperties = false;
    bool hasPolyMesh = false;
    /// `<root>/constant/polyMesh` exists.
    bool polyMeshComplete = false;
    /// Fewer files were listed because the entry cap was hit.
    bool truncated = false;
    std::vector<CaseScannedEntry> entries;
};

} // namespace ofgui::io
