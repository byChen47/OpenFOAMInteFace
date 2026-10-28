#include "io/CaseSourceRecorder.h"

#include "io/Sha256Hasher.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>
#include <vector>

namespace ofgui::io {

std::string currentUtcTimestamp()
{
    const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm utc{};
#if defined(_WIN32)
    gmtime_s(&utc, &now);
#else
    gmtime_r(&now, &utc);
#endif
    char buffer[32]{};
    std::snprintf(buffer,
                  sizeof(buffer),
                  "%04d-%02d-%02dT%02d:%02d:%02dZ",
                  utc.tm_year + 1900,
                  utc.tm_mon + 1,
                  utc.tm_mday,
                  utc.tm_hour,
                  utc.tm_min,
                  utc.tm_sec);
    return buffer;
}

SourceRecording CaseSourceRecorder::record(
    const std::filesystem::path& caseRoot,
    const CaseDirectoryScan& scan,
    std::string sourceRelativePath,
    std::size_t maxFileBytes)
{
    SourceRecording recording;
    recording.info.sourceRelativePath = std::move(sourceRelativePath);
    recording.info.startDirectory = scan.startDirectory;
    recording.info.importedAt = currentUtcTimestamp();

    // Hash every regular file below the case root, not just the entries the
    // scan lists: polyMesh payload files and included dictionaries are part of
    // the provenance record even though the listing does not descend into them.
    namespace fs = std::filesystem;
    std::error_code walkError;
    fs::recursive_directory_iterator iterator(
        caseRoot, fs::directory_options::skip_permission_denied, walkError);
    if (walkError)
    {
        return recording;
    }

    for (const fs::recursive_directory_iterator end; iterator != end; iterator.increment(walkError))
    {
        if (walkError)
        {
            break;
        }
        const fs::directory_entry& entry = *iterator;
        std::error_code statusError;
        const fs::file_status status = entry.symlink_status(statusError);
        if (statusError)
        {
            continue;
        }
        if (fs::is_symlink(status))
        {
            iterator.disable_recursion_pending();
            continue;
        }
        if (!fs::is_regular_file(status))
        {
            continue;
        }

        std::error_code relativeError;
        const std::string relative =
            fs::relative(entry.path(), caseRoot, relativeError).generic_string();
        if (relativeError || relative.empty())
        {
            continue;
        }

        std::error_code sizeError;
        const std::uintmax_t byteSize = entry.file_size(sizeError);
        if (maxFileBytes != 0 && !sizeError && byteSize > maxFileBytes)
        {
            ++recording.skippedFiles;
            continue;
        }

        const std::filesystem::path path = caseRoot / fs::path(relative);
        std::ifstream stream(path, std::ios::binary);
        if (!stream)
        {
            ++recording.skippedFiles;
            continue;
        }
        const std::string bytes{std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
        if (!stream.good() && !stream.eof())
        {
            ++recording.skippedFiles;
            continue;
        }

        core::SourceFileHash fileHash;
        fileHash.relativePath = relative;
        fileHash.sha256 = Sha256Hasher::hash(bytes);
        recording.info.files.push_back(std::move(fileHash));
        ++recording.hashedFiles;
    }

    std::sort(recording.info.files.begin(),
              recording.info.files.end(),
              [](const core::SourceFileHash& left, const core::SourceFileHash& right) {
                  return left.relativePath < right.relativePath;
              });

    return recording;
}

} // namespace ofgui::io
