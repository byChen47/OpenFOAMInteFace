#include "io/ImportIssueFactory.h"

#include "io/ImportCodes.h"

#include <array>
#include <cstddef>
#include <utility>

namespace ofgui::io {
namespace {

using core::Severity;

struct CodeRow
{
    std::string_view code;
    ImportCodeInfo info;
};

constexpr std::array<CodeRow, importcodes::kCodeCount> kCodes{{
    {importcodes::kCaseMissingControlDict,
     {Severity::BlockingError,
      "缺少 system/controlDict，无法作为 OpenFOAM 算例导入",
      "确认选择的是算例根目录（含 0/、constant/、system/），而不是其上层目录"}},
    {importcodes::kCaseMissingStartDirectory,
     {Severity::Error, "既没有 0/ 也没有 0.orig/，无法读取初始场",
      "补上起始时间目录，或只导入网格与求解设置"}},
    {importcodes::kCaseConflictingTarget,
     {Severity::BlockingError, "目标目录等于源算例，或位于源算例内部",
      "选择一个与源算例互不包含的新目录"}},
    {importcodes::kCaseMissingConstant,
     {Severity::Warning, "缺少 constant/ 目录，物性参数将保持默认值",
      "确认算例完整；缺少 constant/ 时导出前需要补全物性"}},
    {importcodes::kCaseRootMissing,
     {Severity::BlockingError, "选择的路径不存在或不是目录", "重新选择算例根目录"}},
    {importcodes::kLexUnterminatedString,
     {Severity::Error, "字符串未闭合", "在该行末尾补上双引号"}},
    {importcodes::kLexUnterminatedComment,
     {Severity::Error, "块注释 /* 未闭合", "补上 */ 或删除该注释"}},
    {importcodes::kLexUnterminatedVerbatim,
     {Severity::Error, "#{ … #} 代码块未闭合", "补上 #} 结束标记"}},
    {importcodes::kLexInvalidCharacter,
     {Severity::Warning, "出现非法控制字符，已跳过", "检查文件是否被二进制内容污染"}},
    {importcodes::kLexUnbalancedParenthesis,
     {Severity::Warning, "单词中的括号不配对", "检查该单词里的 ( 与 ) 是否成对"}},
    {importcodes::kLexMalformedNumber,
     {Severity::Warning, "数字格式不合法，已按单词读入", "按 OpenFOAM 数值写法修正该值"}},
    {importcodes::kLexVariable,
     {Severity::Warning, "$ 变量表达式不完整", "补全变量名，例如 $internalField 或 ${name}"}},
    {importcodes::kParseMissingSemicolon,
     {Severity::Error, "缺少分号 ;", "在该条目值的末尾补上 ;"}},
    {importcodes::kParseMissingBracket,
     {Severity::Error, "括号未闭合", "补齐与开括号对应的 ) 或 }"}},
    {importcodes::kParseMissingHeader,
     {Severity::Error, "缺少 FoamFile 头", "在文件顶部补上 FoamFile { … } 头"}},
    {importcodes::kParseExpectedKeyword,
     {Severity::Error, "此处应为关键字", "检查是否漏写关键字或多写了标点"}},
    {importcodes::kParseHeaderIncomplete,
     {Severity::Error, "FoamFile 头缺少 format 或 class", "在 FoamFile 头中补上 format ascii; 与 class …;"}},
    {importcodes::kParseObjectMissing,
     {Severity::Warning, "FoamFile 头缺少 object，已按文件名回填", "补上 object 关键字，或确认文件名与对象名一致"}},
    {importcodes::kParseStraySemicolon,
     {Severity::Error, "多余的标点（孤立的 ;）", "删除没有对应关键字的 ;"}},
    {importcodes::kParseNestingTooDeep,
     {Severity::Error, "嵌套层数超过上限，该块其余内容已跳过",
      "检查括号是否成对；正常字典的嵌套不会超过 64 层"}},
    {importcodes::kParseExtraBracket,
     {Severity::Error, "多余的右括号", "删除多出来的 ) 或 }"}},
    {importcodes::kSynExpression,
     {Severity::Warning, "${{ … }} 表达式未求值，按原文保留", "如需可在导出前手动填入数值"}},
    {importcodes::kSynSubstitution,
     {Severity::Warning, "${a:-b} 形式的默认值未展开，按原文保留", "改用 $name 写法或手动填入值"}},
    {importcodes::kSynDuplicateKeyword,
     {Severity::Warning, "同一关键字重复出现", "确认第二处是否覆盖了第一处"}},
    {importcodes::kVarUnsupportedScope,
     {Severity::Warning, "$.. 或 $^ 作用域变量未解析，按原文保留", "改用当前作用域内的 $name"}},
    {importcodes::kDirInclude,
     {Severity::Warning, "包含指令未展开，该文件按 Partial 处理", "如需完整内容，先把被包含的文件合并进来"}},
    {importcodes::kDirDirective,
     {Severity::Warning, "该指令未执行，按原文保留", "导出时保留原指令，运行时由 OpenFOAM 处理"}},
    {importcodes::kDirUnknown,
     {Severity::Warning, "未知指令，按原文保留", "确认该指令是否属于目标 OpenFOAM 版本"}},
    {importcodes::kFmtBinary,
     {Severity::Warning, "format binary 文件不支持解析，仅镜像复制", "改为 format ascii 后重新导入"}},
    {importcodes::kEncBinary,
     {Severity::Error, "文件不是 UTF-8/ASCII 文本（UTF-16 或含 NUL 字节）", "另存为 UTF-8 文本后重新导入"}},
    {importcodes::kEncNonAscii,
     {Severity::Warning, "字典中出现非 ASCII 文本，已按原文保留",
      "OpenFOAM 一般可读取 UTF-8，但建议把名称与注释改为 ASCII"}},
    {importcodes::kEncBomRemoved,
     {Severity::Info, "已移除 UTF-8 BOM", "无需处理"}},
    {importcodes::kMeshMultipleBlocks,
     {Severity::BlockingError, "blockMeshDict 含多个块，首版只支持单块", "改用单块算例，或选择镜像导出模式"}},
    {importcodes::kMeshNonHexBlock,
     {Severity::BlockingError, "块类型不是 hex，首版只支持六面体单块", "改用 hex 单块模板"}},
    {importcodes::kMeshAnisotropicScale,
     {Severity::Warning, "scale 是各向异性张量，仅取对角分量", "确认缩放是否正确，必要时改成标量 scale"}},
    {importcodes::kMeshVertexCount,
     {Severity::Error, "vertices 不是 8 个顶点", "补成 8 个顶点或检查是否漏写"}},
    {importcodes::kMeshGradingShape,
     {Severity::Warning, "grading 不是 3 值形式，已按只读保留", "改用 simpleGrading (x y z)"}},
    {importcodes::kMeshEdgesPresent,
     {Severity::Warning, "edges 非空，计算域按顶点包围盒处理", "确认域尺寸是否需要按弧线调整"}},
    {importcodes::kMeshNotACuboid,
     {Severity::Error, "顶点无法构成长方体", "首版只支持长方体计算域"}},
    {importcodes::kMeshMissingScale,
     {Severity::Warning, "缺少 scale，按 1.0 处理", "如需缩放请在网格设置中填入 scale"}},
    {importcodes::kMeshInvalidCells,
     {Severity::BlockingError, "网格数存在非正数", "把每个方向的网格数改为正整数"}},
    {importcodes::kDomainSuggestedTwoD,
     {Severity::Info, "厚度方向只有一层网格，建议按二维算例处理", "可在计算域面板切换为二维"}},
    {importcodes::kDomainAssumedMetre,
     {Severity::Info, "字典没有单位信息，长度按 SI（米）解释", "如需其它单位，请在计算域面板换算后填写"}},
    {importcodes::kPatchRegexKey,
     {Severity::Warning, "正则 patch 键不进入模型，仅只读展示", "如需编辑，请为每个 patch 写精确键"}},
    {importcodes::kPatchMissingBoundaryCondition,
     {Severity::Error, "该 patch 缺少边界条件", "在边界条件面板为该 patch 补一条 BC"}},
    {importcodes::kPatchEmptyName,
     {Severity::Error, "patch 名称为空", "给该 patch 填一个合法名称"}},
    {importcodes::kPatchInvalidType,
     {Severity::Error, "patch 类型无法识别，已回落为 patch", "改为 patch/wall/empty/symmetry/symmetryPlane 之一"}},
    {importcodes::kPatchFaceConflict,
     {Severity::Error, "同一个面被多个 patch 同时认领",
      "在每个 patch 的 faces 列表中只保留唯一的归属，避免重叠"}},
    {importcodes::kPatchUnknownType,
     {Severity::Warning, "patch 类型未知，已回落为 patch", "确认该 patch 的 OpenFOAM 类型"}},
    {importcodes::kPatchBoundaryMismatch,
     {Severity::Warning, "blockMeshDict 与场文件列出的 patch 名单不一致",
      "对齐两处的 patch 名称；多余或缺失的 patch 需要在导出前补齐"}},
    {importcodes::kFieldMissingDimensions,
     {Severity::Error, "dimensions 缺失或不是 7 个指数", "补上 [质量 长度 时间 温度 摩尔 电流 光强] 七个指数"}},
    {importcodes::kBoundaryUnknownType,
     {Severity::Warning, "边界条件类型未知，按只读保留", "确认该 BC 类型是否属于目标 OpenFOAM 版本"}},
    {importcodes::kBoundaryMissingType,
     {Severity::Error, "边界条件缺少 type", "为该 patch 补一条 type"}},
    {importcodes::kTurbulenceMissing,
     {Severity::Info, "没有 constant/turbulenceProperties，按 laminar 处理",
      "如需湍流模拟，请补上 turbulenceProperties 并选择 LES/RAS 模型"}},
    {importcodes::kCaseUnreadableControlDict,
     {Severity::BlockingError,
      "system/controlDict 无法解析，得不到求解设置",
      "修正该文件的语法错误，或换用一份完整的 controlDict"}},
    {importcodes::kTransportModelDefaulted,
     {Severity::Info,
      "未显式指定 transportModel，按 OpenFOAM 默认 Newtonian 处理",
      "若需要别的输运模型，请在 constant/transportProperties 中显式写出"}},
    {importcodes::kUnsupportedSubtree,
     {Severity::Warning,
      "该子树不在前处理模型中，已按原文保留",
      "如需在 GUI 中编辑这部分内容，请在导出后手工维护"}},
    {importcodes::kLimitFileTooLarge,
     {Severity::Error, "文件超过 32 MiB 上限，未解析",
      "拆分该文件，或在导出时按镜像方式原样复制"}},
    {importcodes::kLimitNonUniform,
     {Severity::Warning, "nonuniform 元素数超过 5e6，按只读原文保留",
      "无需处理；编辑该场前请确认内存占用"}},
    {importcodes::kReportTruncated,
     {Severity::Info, "问题条目已截断至 200 条", "修正已有问题后重新导入即可看到后续条目"}},
    {importcodes::kReportTooManyFiles,
     {Severity::Warning, "算例文件数超过 5000，已停止继续列举",
      "确认算例是否包含大量后处理结果；必要时清理后重试"}},
    {importcodes::kExportTargetInsideSource,
     {Severity::BlockingError,
      "导出目标与源算例重叠，拒绝写出",
      "选择一个与源算例互不包含的新目录"}},
    {importcodes::kExportTargetExists,
     {Severity::BlockingError,
      "导出目标已存在且非空",
      "确认覆盖，或换一个尚不存在的目标目录"}},
    {importcodes::kExportWriteFailed,
     {Severity::Error,
      "写入导出文件失败",
      "检查目标目录的磁盘空间与写权限后重试"}},
    {importcodes::kExportTargetNotWritable,
     {Severity::Error,
      "导出目标目录不可写",
      "检查目录权限，或换一个可写目录"}},
    {importcodes::kExportPdrCoexists,
     {Severity::Info,
      "同时存在 PDRblockMeshDict 与 blockMeshDict，本次只解析后者",
      "二者通常只需保留一个；另存为只读参考即可"}},
    {importcodes::kExportRejectedByValidation,
     {Severity::BlockingError,
      "工程校验未通过，拒绝导出",
      "按校验面板列出的问题逐项修正后重试"}},
    {importcodes::kExportNonAsciiText,
     {Severity::Warning,
      "生成的字典包含非 ASCII 文本",
      "OpenFOAM 一般可以读取 UTF-8，但建议改用 ASCII 注释与名称"}},
    {importcodes::kExportMirrorFailed,
     {Severity::Error,
      "复制源算例文件到导出目录失败",
      "检查源文件是否存在且可读、目标磁盘是否可写后重试"}},
    {importcodes::kExportManifestFailed,
     {Severity::Error,
      "导出清单 .ofgport-export.json 写入失败",
      "检查目标目录权限后重试，或关闭导出清单后再次导出"}},
    {importcodes::kExportMirrorIncomplete,
     {Severity::Warning,
      "源算例中有文件未进入导出目录（符号链接或文件数超限）",
      "确认这些文件是否必要；符号链接请先展开为真实文件再导入"}},
}};

/// Every code of `importcodes::kAllCodes` must appear exactly once, so the
/// table can never drift from the header again (D-039: the UI maps by code).
constexpr bool everyCodeHasExactlyOneRow() noexcept
{
    for (const char* code : importcodes::kAllCodes)
    {
        std::size_t matches = 0;
        for (const CodeRow& row : kCodes)
        {
            if (row.code == code)
            {
                ++matches;
            }
        }
        if (matches != 1)
        {
            return false;
        }
    }
    return true;
}

static_assert(kCodes.size() == importcodes::kCodeCount,
              "the ImportIssueFactory table must describe importcodes::kCodeCount codes");
static_assert(everyCodeHasExactlyOneRow(),
              "every code of importcodes::kAllCodes needs exactly one ImportIssueFactory row");

} // namespace

std::optional<ImportCodeInfo> importCodeInfo(std::string_view code) noexcept
{
    for (const CodeRow& row : kCodes)
    {
        if (row.code == code)
        {
            return row.info;
        }
    }
    return std::nullopt;
}

core::ValidationIssue makeImportIssue(
    std::string_view code,
    std::string_view relativePath,
    int line,
    int column,
    std::string nodePath,
    std::string snippet,
    std::string detail)
{
    const std::optional<ImportCodeInfo> info = importCodeInfo(code);
    std::string message = info ? std::string(info->message) : std::string(detail);
    if (message.empty())
    {
        message = std::string(code);
    }

    core::ValidationIssue issue(info ? info->severity : core::Severity::Error, std::string(code), message);
    issue.setOrigin("import");
    if (!relativePath.empty())
    {
        int effectiveLine = line > 0 ? line : 0;
        issue.setLocation(core::SourceLocation(std::string(relativePath), effectiveLine, column > 0 ? column : 0));
    }
    if (!nodePath.empty())
    {
        issue.setNodePath(std::move(nodePath));
    }
    if (!snippet.empty())
    {
        issue.setSnippet(std::move(snippet));
    }
    if (info && !info->suggestedFix.empty())
    {
        issue.setSuggestedFix(std::string(info->suggestedFix));
    }
    return issue;
}

void appendDictionaryDiagnostics(
    std::vector<core::ValidationIssue>& issues,
    const std::vector<FoamDiagnostic>& diagnostics,
    std::string_view relativePath,
    std::string_view nodePathPrefix)
{
    for (const FoamDiagnostic& diagnostic : diagnostics)
    {
        std::string message;
        if (const std::optional<ImportCodeInfo> info = importCodeInfo(diagnostic.code))
        {
            message = std::string(info->message);
        }
        else
        {
            message = diagnostic.message;
        }

        core::ValidationIssue issue(
            diagnostic.severity == FoamSeverity::Error    ? core::Severity::Error
                : diagnostic.severity == FoamSeverity::Warning ? core::Severity::Warning
                                                               : core::Severity::Info,
            diagnostic.code,
            std::move(message));
        issue.setOrigin("import");
        if (!relativePath.empty())
        {
            issue.setLocation(core::SourceLocation(
                std::string(relativePath), diagnostic.location.line, diagnostic.location.column));
        }
        if (!nodePathPrefix.empty())
        {
            issue.setNodePath(std::string(nodePathPrefix));
        }
        if (!diagnostic.snippet.empty())
        {
            issue.setSnippet(diagnostic.snippet);
        }
        if (const std::optional<ImportCodeInfo> info = importCodeInfo(diagnostic.code);
            info && !info->suggestedFix.empty())
        {
            issue.setSuggestedFix(std::string(info->suggestedFix));
        }
        issues.push_back(std::move(issue));
    }
}

} // namespace ofgui::io
