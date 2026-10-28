#include "io/writers/ExportSkipRules.h"

#include "io/CaseNameRules.h"

#include <array>
#include <string_view>
#include <utility>

namespace ofgui::io {
namespace {

constexpr std::array<std::pair<ExportSkipReason, const char*>, 5> kReasonNames{{
    {ExportSkipReason::None, "none"},
    {ExportSkipReason::RuntimeArtifact, "runtime-artifact"},
    {ExportSkipReason::OtherTimeDirectory, "other-time-directory"},
    {ExportSkipReason::SymbolicLink, "symbolic-link"},
    {ExportSkipReason::Unreadable, "unreadable"},
}};

std::string_view firstSegment(std::string_view path) noexcept
{
    const std::size_t slash = path.find('/');
    return slash == std::string_view::npos ? path : path.substr(0, slash);
}

std::string_view lastSegment(std::string_view path) noexcept
{
    const std::size_t slash = path.rfind('/');
    return slash == std::string_view::npos ? path : path.substr(slash + 1);
}

bool isProcessorDirectory(std::string_view name) noexcept
{
    constexpr std::string_view kPrefix{"processor"};
    if (name.size() <= kPrefix.size() || name.substr(0, kPrefix.size()) != kPrefix)
    {
        return false;
    }
    return detail::isNumericName(name.substr(kPrefix.size()));
}

bool isRunScript(std::string_view relativePath) noexcept
{
    return relativePath.find('/') == std::string_view::npos && relativePath.starts_with("All");
}

} // namespace

std::string_view exportSkipReasonName(ExportSkipReason reason) noexcept
{
    for (const auto& [value, name] : kReasonNames)
    {
        if (value == reason)
        {
            return name;
        }
    }
    return "none";
}

ExportSkipRules::ExportSkipRules(CaseExportSkipPolicy policy) noexcept
    : policy_(policy)
{
}

bool ExportSkipRules::isRuntimeArtifact(std::string_view relativePath) noexcept
{
    if (relativePath.empty())
    {
        return false;
    }

    const std::string_view first = firstSegment(relativePath);
    if (first == "postProcessing" || isProcessorDirectory(first))
    {
        return true;
    }
    if (isRunScript(relativePath))
    {
        return true;
    }

    const std::string_view name = lastSegment(relativePath);
    return name.ends_with(".log") || name.ends_with(".foam") || name.starts_with("log.");
}

bool ExportSkipRules::isOtherTimeDirectory(
    std::string_view relativePath,
    std::string_view startDirectory) noexcept
{
    if (relativePath.empty())
    {
        return false;
    }

    const std::string_view first = firstSegment(relativePath);
    if (first == startDirectory)
    {
        return false;
    }
    if (!detail::isNumericName(first) && first != "latest")
    {
        return false;
    }
    // `0/p` with an empty start directory still belongs to the start data.
    return !(startDirectory.empty() && first == "0");
}

ExportSkipReason ExportSkipRules::classify(
    std::string_view relativePath,
    std::string_view startDirectory) const noexcept
{
    if (relativePath.empty())
    {
        return ExportSkipReason::None;
    }
    if (isOtherTimeDirectory(relativePath, startDirectory) && !policy_.mirrorOtherTimeDirectories)
    {
        return ExportSkipReason::OtherTimeDirectory;
    }
    if (isRuntimeArtifact(relativePath) && !(policy_.keepRunScripts && isRunScript(relativePath)))
    {
        return ExportSkipReason::RuntimeArtifact;
    }
    return ExportSkipReason::None;
}

} // namespace ofgui::io
