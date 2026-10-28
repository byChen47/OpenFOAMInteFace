# OpenFOAM 字典测试样本（fixtures）

本目录是 OpenFOAM v2206 字典的**快照**，用于 `tests/io` 的解析器回归测试。
运行期测试只读本目录，**不得**读取仓库根目录的 `OpenFOAM-v2206/`（该目录仅作只读参照，
见 `docs/CASE_IMPORT_SPEC.md` §9.4 与 §12）。`OFGUI_FOAM_REFERENCE_DIR` 宏只在
`FoamReferenceCorpusTest.cpp` 中用于“真实教程语料”探针，并在目录缺失时 `QSKIP`。

## 来源（逐字复制，未做任何修改）

| 快照路径 | 原始文件 |
| --- | --- |
| `cavity/0/p` | `OpenFOAM-v2206/tutorials/incompressible/icoFoam/cavity/cavity/0/p` |
| `cavity/0/U` | `OpenFOAM-v2206/tutorials/incompressible/icoFoam/cavity/cavity/0/U` |
| `cavity/constant/transportProperties` | `.../cavity/cavity/constant/transportProperties` |
| `cavity/system/controlDict` | `.../cavity/cavity/system/controlDict` |
| `cavity/system/fvSchemes` | `.../cavity/cavity/system/fvSchemes` |
| `cavity/system/fvSolution` | `.../cavity/cavity/system/fvSolution` |
| `cavity/system/blockMeshDict` | `.../cavity/cavity/system/blockMeshDict` |
| `pipeCyclic/U` | `OpenFOAM-v2206/tutorials/incompressible/simpleFoam/pipeCyclic/0.orig/U` |
| `pointDisplacement/pointDisplacement` | `OpenFOAM-v2206/tutorials/basic/overLaplacianDyMFoam/heatTransfer/0.orig/pointDisplacement` |

## 导入测试用算例目录（`case/`）

`case/` 是**单个完整算例**，供 `tests/io/CaseImporter*Test.cpp` 使用。其中场文件与字典是上面
cavity 快照的副本；`constant/polyMesh/*` 与 `system/decomposeParDict` 是为测试手工编写的最小 
ASCII 占位文件（真实教程的 polyMesh 由 `blockMesh` 生成，约为几 MB 的二进制/ASCII 网格）。

| 路径 | 来源 |
| --- | --- |
| `case/0/p`、`case/0/U` | cavity 快照副本 |
| `case/constant/transportProperties` | cavity 快照副本（只有 `nu 0.01;`，无 `transportModel`） |
| `case/system/{blockMeshDict,controlDict,fvSchemes,fvSolution}` | cavity 快照副本 |
| `case/constant/polyMesh/{points,faces,owner,neighbour,boundary}` | 手工最小占位（仅用于“polyMesh 五件套齐全”探测） |
| `case/system/decomposeParDict` | 手工编写（`numberOfSubdomains 4; method scotch;`） |

`case/` 的期望导入结果见 `docs/CASE_IMPORT_SPEC.md` §11.1（`scale 0.1` → `max=(0.1,0.1,0.01)`、
`cells (20 20 1)`、3 个 patch、`icoFoam`/`endTime 0.5`/`nu 0.01`、0 个 `BlockingError`）。

覆盖的语法点：

- `cavity/*`：80 列 banner 注释、`FoamFile` 头、`dimensions [...]`、`uniform`、
  `( … )` 列表/元组、`blocks ( hex ( … ) (20 20 1) simpleGrading (1 1 1) )` 嵌套列表、
  `grad(p)` / `div(phi,U)` 这类带括号的单词关键字、`scale 0.1;`。
- `cavity/system/fvSolution`：`pFinal { $p; relTol 0; }`，`$p;` 变量合并条目。
- `pipeCyclic/U`：无分号的 `#includeEtc "caseDicts/setConstraintTypes"`、
  `code #{ … #}` codedFixedValue 代码块（内含 `//`、`^`、`operator==`）、
  `value $internalField;`。
- `pointDisplacement/pointDisplacement`：带引号的正则关键字 `".*"`、`"(free|hole)"`，
  以及 `uniformValue table` 后跟跨行 `( (0.0 (0 0 0)) … );`。

## 手工构造的样本（用于错误路径与边界情况）

| 文件 | 用途 | 期望诊断 |
| --- | --- | --- |
| `invalid/missing_semicolon` | `default Euler` 后缺 `;`，下一行开始新条目 | `IMPORT-PARSE-001` |
| `invalid/unterminated_string` | 字符串未闭合即换行 | `IMPORT-LEX-001` |
| `invalid/unterminated_comment` | `/*` 未闭合到 EOF | `IMPORT-LEX-002` |
| `invalid/unterminated_verbatim` | `#{` 未闭合到 EOF | `IMPORT-LEX-003` |
| `invalid/missing_brace` | 字典未闭合到 EOF | `IMPORT-PARSE-002` |
| `invalid/stray_brace` | 多余 `}` | `IMPORT-PARSE-006` |
| `invalid/duplicate_keyword` | 同一关键字出现两次 | `IMPORT-SYN-003`（Warning） |
| `field/nonuniformT` | `nonuniform List<scalar>` + 计数 + 跨行列表、`)` 与 `;` 分行 | 无 |

`field/nonuniformT` 是按 v2206 写法手工构造的小样本（真实样本
`tutorials/DNS/dnsFoam/boxTurb16/0.orig/U` 约 138 KiB，不适合做快照）。

## 维护规则

1. 新增快照必须逐字复制原始文件，并在上表登记来源。
2. 不要为了让测试通过而修改快照内容；如原始文件与解析器不符，先修解析器或记入
   `docs/TASKS.md`。
3. 快照使用 LF 换行；测试同时覆盖 CRLF 输入（在测试代码内构造字符串，不落盘）。
