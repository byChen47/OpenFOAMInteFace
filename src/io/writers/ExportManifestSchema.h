#pragma once

namespace ofgui::io::exportmanifest {

/// Version of the `.ofgport-export.json` layout written by this build.
///
/// The manifest is a side product of one export run: it is informational for
/// the user and for later diffing, so a newer build may add keys without
/// breaking readers (`docs/CASE_IMPORT_SPEC.md` section 9.5).
inline constexpr int kSchemaVersion = 1;

/// File name of the export manifest at the root of the exported case.
inline constexpr const char* kFileName = ".ofgport-export.json";

inline constexpr const char* kSchemaVersionKey = "schemaVersion";
inline constexpr const char* kGeneratedAtKey = "generatedAt";
inline constexpr const char* kCountsKey = "counts";
inline constexpr const char* kFilesKey = "files";
inline constexpr const char* kSkippedKey = "skipped";
inline constexpr const char* kRelativePathKey = "relativePath";
inline constexpr const char* kActionKey = "action";
inline constexpr const char* kBytesKey = "bytes";
inline constexpr const char* kReasonKey = "reason";

} // namespace ofgui::io::exportmanifest
