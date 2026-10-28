#pragma once

#include <string_view>

namespace ofgui::io {

/// Why a source file of the case is not part of the export.
enum class ExportSkipReason
{
    None = 0,
    RuntimeArtifact = 1,
    OtherTimeDirectory = 2,
    SymbolicLink = 3,
    Unreadable = 4
};

/// `runtime-artifact`, `other-time-directory`, `symbolic-link`, `unreadable` or
/// `none`; these are the `reason` spellings of `.ofgport-export.json`.
[[nodiscard]] std::string_view exportSkipReasonName(ExportSkipReason reason) noexcept;

/// User visible switches that turn a default skip into a mirror.
struct CaseExportSkipPolicy
{
    /// Keep the root level `All*` scripts (`Allrun`, `Allclean`, ...).
    bool keepRunScripts{false};
    /// Mirror time directories other than the start directory.
    bool mirrorOtherTimeDirectories{false};
};

/// Decides which source files the exporter leaves out
/// (`docs/CASE_IMPORT_SPEC.md` section 9.2).
class ExportSkipRules
{
public:
    ExportSkipRules() = default;
    explicit ExportSkipRules(CaseExportSkipPolicy policy) noexcept;

    [[nodiscard]] const CaseExportSkipPolicy& policy() const noexcept { return policy_; }

    /// `relativePath` is a case relative POSIX path; `startDirectory` is `0`,
    /// `0.orig` or empty when the source has no start directory.
    [[nodiscard]] ExportSkipReason classify(
        std::string_view relativePath,
        std::string_view startDirectory) const noexcept;

    /// True for `postProcessing/**`, `processor*/**`, `*.log`, `*.foam` and the
    /// root level `All*` run scripts.
    [[nodiscard]] static bool isRuntimeArtifact(std::string_view relativePath) noexcept;

    /// True when the first path segment is a time directory other than
    /// `startDirectory` (`1`, `0.5`, `latest`, ...).
    [[nodiscard]] static bool isOtherTimeDirectory(
        std::string_view relativePath,
        std::string_view startDirectory) noexcept;

private:
    CaseExportSkipPolicy policy_{};
};

} // namespace ofgui::io
