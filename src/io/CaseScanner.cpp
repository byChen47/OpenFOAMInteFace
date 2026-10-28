#include "io/CaseScanner.h"

#include "io/CaseDirectoryProbe.h"
#include "io/CaseNameRules.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace ofgui::io {
namespace {

namespace fs = std::filesystem;

} // namespace

using detail::fileSize;
using detail::isDirectory;
using detail::isNumericName;
using detail::kindForSystemFile;
using detail::polyMeshComplete;
using detail::relativePath;

bool isTimeDirectoryName(std::string_view name) noexcept
{
    if (name == "latest")
    {
        return true;
    }
    return isNumericName(name);
}

CaseScanner::CaseScanner(CaseImportOptions options) noexcept
    : options_(options)
{
}

CaseDirectoryScan CaseScanner::scan(const fs::path& caseRoot) const
{
    CaseDirectoryScan scan;
    scan.caseRoot = caseRoot.generic_string();

    std::error_code ec;
    if (!fs::is_directory(caseRoot, ec) || ec)
    {
        return scan;
    }
    scan.hasCaseRoot = true;

    // Start time directory: `0` wins over `0.orig` (spec section 4.1).
    for (const std::string candidate : {"0", "0.orig"})
    {
        const fs::path directory = caseRoot / candidate;
        if (isDirectory(directory))
        {
            scan.startDirectory = candidate;
            break;
        }
    }
    scan.hasStartDirectory = !scan.startDirectory.empty();

    const fs::path constantDirectory = caseRoot / "constant";
    const fs::path systemDirectory = caseRoot / "system";
    scan.hasConstantDirectory = isDirectory(constantDirectory);
    scan.hasSystemDirectory = isDirectory(systemDirectory);

    const std::size_t maxFiles = options_.maxFiles;
    const auto pushEntry = [&scan, maxFiles](CaseScannedEntry entry) {
        if (scan.entries.size() >= maxFiles)
        {
            scan.truncated = true;
            return false;
        }
        scan.entries.push_back(std::move(entry));
        return true;
    };

    // Root level: time directories, scripts, loose dictionaries.
    {
        fs::directory_iterator iterator(caseRoot, ec);
        if (!ec)
        {
            for (const fs::directory_entry& entry : iterator)
            {
                std::error_code statusError;
                const fs::file_status status = entry.symlink_status(statusError);
                if (statusError)
                {
                    continue;
                }
                const std::string name = entry.path().filename().generic_string();
                CaseScannedEntry scanned;
                scanned.relativePath = name;
                if (fs::is_symlink(status))
                {
                    scanned.symlink = true;
                    scanned.kind = CaseEntryKind::Other;
                    if (!pushEntry(std::move(scanned)))
                    {
                        break;
                    }
                    continue;
                }
                if (fs::is_directory(status))
                {
                    if (name == scan.startDirectory || name == "0" || name == "0.orig")
                    {
                        continue; // expanded below as the field directory
                    }
                    if (!isTimeDirectoryName(name) || name == "constant" || name == "system")
                    {
                        continue;
                    }
                    scanned.otherTimeDirectory = true;
                    scanned.directory = true;
                    scanned.kind = CaseEntryKind::Other;
                    if (!pushEntry(std::move(scanned)))
                    {
                        break;
                    }
                    continue;
                }
                if (!fs::is_regular_file(status))
                {
                    continue;
                }
                scanned.byteSize = fileSize(entry.path());
                scanned.gzip = name.size() > 3 && name.compare(name.size() - 3, 3, ".gz") == 0;
                scanned.kind = name.rfind("All", 0) == 0 ? CaseEntryKind::Script : CaseEntryKind::Other;
                if (!pushEntry(std::move(scanned)))
                {
                    break;
                }
            }
        }
    }

    // Start time directory: every regular file is a field candidate.
    if (scan.hasStartDirectory)
    {
        fs::directory_iterator iterator(caseRoot / scan.startDirectory, ec);
        if (!ec)
        {
            for (const fs::directory_entry& entry : iterator)
            {
                std::error_code statusError;
                const fs::file_status status = entry.symlink_status(statusError);
                if (statusError)
                {
                    continue;
                }
                CaseScannedEntry scanned;
                scanned.relativePath = relativePath(caseRoot, entry.path());
                if (fs::is_symlink(status))
                {
                    scanned.symlink = true;
                    scanned.kind = CaseEntryKind::Field;
                    if (!pushEntry(std::move(scanned)))
                    {
                        break;
                    }
                    continue;
                }
                if (!fs::is_regular_file(status))
                {
                    continue;
                }
                const std::string name = entry.path().filename().generic_string();
                scanned.kind = CaseEntryKind::Field;
                scanned.byteSize = fileSize(entry.path());
                scanned.gzip = name.size() > 3 && name.compare(name.size() - 3, 3, ".gz") == 0;
                if (!pushEntry(std::move(scanned)))
                {
                    break;
                }
            }
        }
    }

    // constant/: dictionaries plus the polyMesh probe.
    if (scan.hasConstantDirectory)
    {
        fs::directory_iterator iterator(constantDirectory, ec);
        if (!ec)
        {
            for (const fs::directory_entry& entry : iterator)
            {
                std::error_code statusError;
                const fs::file_status status = entry.symlink_status(statusError);
                if (statusError)
                {
                    continue;
                }
                CaseScannedEntry scanned;
                scanned.relativePath = relativePath(caseRoot, entry.path());
                if (fs::is_symlink(status))
                {
                    scanned.symlink = true;
                    if (!pushEntry(std::move(scanned)))
                    {
                        break;
                    }
                    continue;
                }
                if (fs::is_directory(status))
                {
                    scanned.directory = true;
                    scanned.kind = CaseEntryKind::MeshDirectory;
                    const std::string name = entry.path().filename().generic_string();
                    if (name == "polyMesh")
                    {
                        scan.hasPolyMesh = true;
                        scan.polyMeshComplete = polyMeshComplete(entry.path());
                    }
                    if (!pushEntry(std::move(scanned)))
                    {
                        break;
                    }
                    continue;
                }
                if (!fs::is_regular_file(status))
                {
                    continue;
                }
                const std::string name = entry.path().filename().generic_string();
                scanned.kind = CaseEntryKind::Dictionary;
                scanned.byteSize = fileSize(entry.path());
                scanned.gzip = name.size() > 3 && name.compare(name.size() - 3, 3, ".gz") == 0;
                scan.hasTransportProperties = scan.hasTransportProperties || name == "transportProperties";
                scan.hasTurbulenceProperties = scan.hasTurbulenceProperties || name == "turbulenceProperties";
                if (!pushEntry(std::move(scanned)))
                {
                    break;
                }
            }
        }
    }

    // system/: the dictionaries the importer reads.
    if (scan.hasSystemDirectory)
    {
        fs::directory_iterator iterator(systemDirectory, ec);
        if (!ec)
        {
            for (const fs::directory_entry& entry : iterator)
            {
                std::error_code statusError;
                const fs::file_status status = entry.symlink_status(statusError);
                if (statusError)
                {
                    continue;
                }
                CaseScannedEntry scanned;
                scanned.relativePath = relativePath(caseRoot, entry.path());
                if (fs::is_symlink(status))
                {
                    scanned.symlink = true;
                    if (!pushEntry(std::move(scanned)))
                    {
                        break;
                    }
                    continue;
                }
                if (fs::is_directory(status))
                {
                    scanned.directory = true;
                    scanned.kind = CaseEntryKind::Other;
                    if (!pushEntry(std::move(scanned)))
                    {
                        break;
                    }
                    continue;
                }
                if (!fs::is_regular_file(status))
                {
                    continue;
                }
                const std::string name = entry.path().filename().generic_string();
                scanned.kind = kindForSystemFile(name);
                scanned.byteSize = fileSize(entry.path());
                scanned.gzip = name.size() > 3 && name.compare(name.size() - 3, 3, ".gz") == 0;
                scan.hasControlDict = scan.hasControlDict || name == "controlDict";
                scan.hasBlockMeshDict = scan.hasBlockMeshDict || name == "blockMeshDict";
                scan.hasFvSchemes = scan.hasFvSchemes || name == "fvSchemes";
                scan.hasFvSolution = scan.hasFvSolution || name == "fvSolution";
                scan.hasDecomposeParDict = scan.hasDecomposeParDict || name == "decomposeParDict";
                if (!pushEntry(std::move(scanned)))
                {
                    break;
                }
            }
        }
    }

    // The listing above stops at maxFiles, but the presence flags steer the
    // importer, so probe the canonical files directly instead of trusting a
    // possibly truncated listing.
    std::error_code probeError;
    const auto probeFile = [&](std::string_view relativePath) {
        probeError.clear();
        const fs::path plain = caseRoot / fs::path(std::string(relativePath));
        if (fs::is_regular_file(plain, probeError) && !probeError)
        {
            return true;
        }
        probeError.clear();
        const fs::path compressed = caseRoot / fs::path(std::string(relativePath) + ".gz");
        return fs::is_regular_file(compressed, probeError) && !probeError;
    };
    const auto probeDirectory = [&](std::string_view relativePath) {
        probeError.clear();
        return fs::is_directory(caseRoot / fs::path(std::string(relativePath)), probeError)
               && !probeError;
    };

    scan.hasControlDict = scan.hasControlDict || probeFile("system/controlDict");
    scan.hasBlockMeshDict = scan.hasBlockMeshDict || probeFile("system/blockMeshDict");
    scan.hasFvSchemes = scan.hasFvSchemes || probeFile("system/fvSchemes");
    scan.hasFvSolution = scan.hasFvSolution || probeFile("system/fvSolution");
    scan.hasDecomposeParDict = scan.hasDecomposeParDict || probeFile("system/decomposeParDict");
    scan.hasTransportProperties =
        scan.hasTransportProperties || probeFile("constant/transportProperties");
    scan.hasTurbulenceProperties =
        scan.hasTurbulenceProperties || probeFile("constant/turbulenceProperties");
    scan.hasPolyMesh = scan.hasPolyMesh || probeDirectory("constant/polyMesh");
    if (scan.hasPolyMesh && !scan.polyMeshComplete)
    {
        probeError.clear();
        scan.polyMeshComplete = polyMeshComplete(caseRoot / "constant" / "polyMesh");
    }

    std::stable_sort(
        scan.entries.begin(),
        scan.entries.end(),
        [](const CaseScannedEntry& left, const CaseScannedEntry& right) {
            return left.relativePath < right.relativePath;
        });
    return scan;
}

} // namespace ofgui::io