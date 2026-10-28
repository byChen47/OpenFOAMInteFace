#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace ofgui::io::importcodes {

// Stable codes from `docs/CASE_IMPORT_SPEC.md` appendix C. Only the codes the
// importer currently emits are listed; the full table lives in the spec.

inline constexpr const char* kCaseMissingControlDict = "IMPORT-CASE-001";
inline constexpr const char* kCaseMissingStartDirectory = "IMPORT-CASE-002";
inline constexpr const char* kCaseConflictingTarget = "IMPORT-CASE-003";
inline constexpr const char* kCaseMissingConstant = "IMPORT-CASE-004";
inline constexpr const char* kCaseRootMissing = "IMPORT-CASE-005";
inline constexpr const char* kCaseUnreadableControlDict = "IMPORT-CASE-006";

inline constexpr const char* kLexUnterminatedString = "IMPORT-LEX-001";
inline constexpr const char* kLexUnterminatedComment = "IMPORT-LEX-002";
inline constexpr const char* kLexUnterminatedVerbatim = "IMPORT-LEX-003";
inline constexpr const char* kLexInvalidCharacter = "IMPORT-LEX-004";
inline constexpr const char* kLexUnbalancedParenthesis = "IMPORT-LEX-005";
inline constexpr const char* kLexMalformedNumber = "IMPORT-LEX-006";
inline constexpr const char* kLexVariable = "IMPORT-LEX-007";

inline constexpr const char* kParseMissingSemicolon = "IMPORT-PARSE-001";
inline constexpr const char* kParseMissingBracket = "IMPORT-PARSE-002";
inline constexpr const char* kParseMissingHeader = "IMPORT-PARSE-003";
inline constexpr const char* kParseHeaderIncomplete = "IMPORT-PARSE-004";
inline constexpr const char* kParseObjectMissing = "IMPORT-PARSE-005";
inline constexpr const char* kParseExtraBracket = "IMPORT-PARSE-006";
inline constexpr const char* kParseExpectedKeyword = "IMPORT-PARSE-007";
inline constexpr const char* kParseStraySemicolon = "IMPORT-PARSE-008";
inline constexpr const char* kParseNestingTooDeep = "IMPORT-PARSE-009";

inline constexpr const char* kSynExpression = "IMPORT-SYN-001";
inline constexpr const char* kSynSubstitution = "IMPORT-SYN-002";
inline constexpr const char* kSynDuplicateKeyword = "IMPORT-SYN-003";

inline constexpr const char* kVarUnsupportedScope = "IMPORT-VAR-002";

inline constexpr const char* kDirInclude = "IMPORT-DIR-001";
inline constexpr const char* kDirDirective = "IMPORT-DIR-002";
inline constexpr const char* kDirUnknown = "IMPORT-DIR-003";

inline constexpr const char* kFmtBinary = "IMPORT-FMT-001";
inline constexpr const char* kEncBinary = "IMPORT-ENC-001";
inline constexpr const char* kEncNonAscii = "IMPORT-ENC-002";
inline constexpr const char* kEncBomRemoved = "IMPORT-ENC-003";

inline constexpr const char* kMeshMultipleBlocks = "IMPORT-MESH-001";
inline constexpr const char* kMeshNonHexBlock = "IMPORT-MESH-002";
inline constexpr const char* kMeshAnisotropicScale = "IMPORT-MESH-003";
inline constexpr const char* kMeshVertexCount = "IMPORT-MESH-004";
inline constexpr const char* kMeshGradingShape = "IMPORT-MESH-005";
inline constexpr const char* kMeshEdgesPresent = "IMPORT-MESH-006";
inline constexpr const char* kMeshNotACuboid = "IMPORT-MESH-007";
inline constexpr const char* kMeshMissingScale = "IMPORT-MESH-008";
inline constexpr const char* kMeshInvalidCells = "IMPORT-MESH-009";

inline constexpr const char* kDomainSuggestedTwoD = "IMPORT-DOM-001";
inline constexpr const char* kDomainAssumedMetre = "IMPORT-DOM-002";

inline constexpr const char* kPatchRegexKey = "IMPORT-PATCH-001";
inline constexpr const char* kPatchMissingBoundaryCondition = "IMPORT-PATCH-002";
inline constexpr const char* kPatchEmptyName = "IMPORT-PATCH-004";
inline constexpr const char* kPatchInvalidType = "IMPORT-PATCH-005";
inline constexpr const char* kPatchFaceConflict = "IMPORT-PATCH-006";
inline constexpr const char* kPatchUnknownType = "IMPORT-PATCH-007";
inline constexpr const char* kPatchBoundaryMismatch = "IMPORT-PATCH-008";

inline constexpr const char* kFieldMissingDimensions = "IMPORT-FIELD-001";
inline constexpr const char* kBoundaryUnknownType = "IMPORT-BC-001";
inline constexpr const char* kBoundaryMissingType = "IMPORT-BC-002";
inline constexpr const char* kTurbulenceMissing = "IMPORT-TURB-001";
inline constexpr const char* kTransportModelDefaulted = "IMPORT-TRANS-001";
inline constexpr const char* kUnsupportedSubtree = "IMPORT-UNSUP-001";

inline constexpr const char* kLimitFileTooLarge = "IMPORT-LIM-001";
inline constexpr const char* kLimitNonUniform = "IMPORT-LIM-002";
inline constexpr const char* kReportTruncated = "IMPORT-REPORT-001";
inline constexpr const char* kReportTooManyFiles = "IMPORT-REPORT-002";
inline constexpr const char* kExportTargetInsideSource = "IMPORT-EXP-001";
inline constexpr const char* kExportTargetExists = "IMPORT-EXP-002";
inline constexpr const char* kExportWriteFailed = "IMPORT-EXP-003";
inline constexpr const char* kExportTargetNotWritable = "IMPORT-EXP-004";
inline constexpr const char* kExportPdrCoexists = "IMPORT-EXP-005";
inline constexpr const char* kExportRejectedByValidation = "IMPORT-EXP-006";
inline constexpr const char* kExportNonAsciiText = "IMPORT-EXP-007";
inline constexpr const char* kExportMirrorFailed = "IMPORT-EXP-008";
inline constexpr const char* kExportManifestFailed = "IMPORT-EXP-009";
inline constexpr const char* kExportMirrorIncomplete = "IMPORT-EXP-010";

/// Number of codes this build knows about. `ImportIssueFactory` sizes its code
/// table with this constant, so adding a code above without a row there is a
/// compile error (see the `static_assert`s in `ImportIssueFactory.cpp`).
inline constexpr std::size_t kCodeCount = 71;

/// Every code of appendix C, in table order.
///
/// The issue factory checks at compile time that `kAllCodes` and its own
/// `kCodes` table describe exactly the same set, and the tests iterate this
/// array so a new code cannot ship without severity and text.
inline constexpr std::array<const char*, kCodeCount> kAllCodes{
    kCaseMissingControlDict,
    kCaseMissingStartDirectory,
    kCaseConflictingTarget,
    kCaseMissingConstant,
    kCaseRootMissing,
    kCaseUnreadableControlDict,
    kLexUnterminatedString,
    kLexUnterminatedComment,
    kLexUnterminatedVerbatim,
    kLexInvalidCharacter,
    kLexUnbalancedParenthesis,
    kLexMalformedNumber,
    kLexVariable,
    kParseMissingSemicolon,
    kParseMissingBracket,
    kParseMissingHeader,
    kParseHeaderIncomplete,
    kParseObjectMissing,
    kParseExtraBracket,
    kParseExpectedKeyword,
    kParseStraySemicolon,
    kParseNestingTooDeep,
    kSynExpression,
    kSynSubstitution,
    kSynDuplicateKeyword,
    kVarUnsupportedScope,
    kDirInclude,
    kDirDirective,
    kDirUnknown,
    kFmtBinary,
    kEncBinary,
    kEncNonAscii,
    kEncBomRemoved,
    kMeshMultipleBlocks,
    kMeshNonHexBlock,
    kMeshAnisotropicScale,
    kMeshVertexCount,
    kMeshGradingShape,
    kMeshEdgesPresent,
    kMeshNotACuboid,
    kMeshMissingScale,
    kMeshInvalidCells,
    kDomainSuggestedTwoD,
    kDomainAssumedMetre,
    kPatchRegexKey,
    kPatchMissingBoundaryCondition,
    kPatchEmptyName,
    kPatchInvalidType,
    kPatchFaceConflict,
    kPatchUnknownType,
    kPatchBoundaryMismatch,
    kFieldMissingDimensions,
    kBoundaryUnknownType,
    kBoundaryMissingType,
    kTurbulenceMissing,
    kTransportModelDefaulted,
    kUnsupportedSubtree,
    kLimitFileTooLarge,
    kLimitNonUniform,
    kReportTruncated,
    kReportTooManyFiles,
    kExportTargetInsideSource,
    kExportTargetExists,
    kExportWriteFailed,
    kExportTargetNotWritable,
    kExportPdrCoexists,
    kExportRejectedByValidation,
    kExportNonAsciiText,
    kExportMirrorFailed,
    kExportManifestFailed,
    kExportMirrorIncomplete,
};

} // namespace ofgui::io::importcodes
