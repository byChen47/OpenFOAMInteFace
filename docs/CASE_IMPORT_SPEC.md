# OpenFOAM v2206 算例识别与前处理规格

| 项 | 值 |
| --- | --- |
| 文档 ID | SPEC-CASE-IMPORT |
| 版本 | 1.0（首版规格，供实现任务直接执行） |
| 状态 | 待评审 |
| 作者 | analyst（team `openfoam-case-preprocessor`，任务 t1） |
| 依据 | `AGENTS.md`、`docs/STATUS.md`、`docs/TASKS.md`、`docs/ARCHITECTURE.md`、`docs/DECISIONS.md`、`src/` 全量代码、`OpenFOAM-v2206/`（只读参考） |
| 适用范围 | 首版基线：单块六面体计算域、`icoFoam` 不可压层流；外加“导入已有 v2206 算例 → 编辑 → 安全导出”这条新链路 |
| 非目标 | 不改 `src/`、`CMakeLists.txt`、`scripts/`；不实现代码；不编译/链接 OpenFOAM |

---

## 0. 阅读指引

| 角色 | 必读章节 |
| --- | --- |
| 队长 / 架构决策 | 全部，重点 §2、§9.3、§10、§13 |
| `core` 模型实现者 | §5、§6、§10.2、§12.1 |
| `io` 解析与写出实现者 | §5、§6.8、§9、§10.3、§12.2 |
| `ui` 实现者 | §7、§8、§10.4 |
| 测试实现者 | §9.4、§11、§12 |
| 评审者 | §4.7（首版不支持项）、§10.8（文件组织检查清单）、附录 B（出处索引） |

本规格的强制力：§4、§5、§6、§9.4、§10 中的 **必须 / 禁止** 条款。其余为设计建议，实现者可优化但需在实现任务的交接摘要中说明理由。

---

## 1. 目标、范围与非目标

### 1.1 目标

1. **识别**：给定一个 OpenFOAM v2206 算例目录（含 `0/` 或 `0.orig/`、`constant/`、`system/`），识别出与“计算域 / 初始条件 / 求解设置”相关的全部文件，并给出可定位到“文件 + 行 + 列”的导入报告。
2. **前处理**：把识别结果投影为 GUI 领域模型（`CaseProject` 及其子对象），使用户能在 GUI 中查看并修改计算域、网格、patch、场与边界条件、求解设置。
3. **安全导出**：把编辑后的模型导出为 **v2206 兼容** 的算例目录；未被 GUI 接管的文件按“镜像”策略原样保留；整次导出具备前置校验、临时目录写入与原子替换。
4. **规格可执行**：后续 5 个角色可以只读本文件就拆出实现任务，不需要再次调研 `OpenFOAM-v2206/`。

### 1.2 非目标（本规格与首版实现都不做）

- 不解析 `constant/polyMesh/` 的 `points/faces/owner/neighbour/boundary` 内容（**只识别**，见 §4.3）。
- 不执行 `blockMesh`、`checkMesh`、`decomposePar`、任何求解器（`AGENTS.md` §2、`docs/DECISIONS.md` D-008）。
- 不支持多块结构网格导入（`pitzDaily` 的 5 个 `hex` 块属于“识别并报错”的负例，见 §11.2）。
- 不支持 `snappyHexMesh` / `snappyHexMeshDict`、任意 CAD/STL 几何、曲面拾取。
- 不展开 `#include` / `#includeEtc` / `#includeFunc` 等指令（§5.5 给出取舍与理由）。
- 不实现 `${{ ... }}` 表达式求值、`${a:-b}` 默认值展开、`#codeStream` 代码执行。
- 不做多相/燃烧/电磁/可压热物性模板；不内置 `thermophysicalProperties` 语义。
- 不扫描时间目录集合（只认一个起始时间目录，§4.1）。
- 不实现模型编辑抽象层（`SolverTemplate`、`IMeshSource`，见 `docs/ARCHITECTURE.md` §11）；首版直接用具体投影器 + writer。

---

## 2. 现状审计

### 2.1 代码现状（`src/` 共 35 个文件）

| 模块 | 现状 | 与本需求的关系 |
| --- | --- | --- |
| `src/app/` | `main.cpp`、`ApplicationBootstrap.h/.cpp`、`SmokeTestController.h/.cpp` | 需要在启动链里注入导入服务与命令行 smoke 入口 |
| `src/render/` | `FaceId.h`、`FaceAppearance`、`PreviewDomain`、`FacePlaneFactory`、`FaceActorFactory`、`OutlineActorFactory`、`DomainSceneActors`、`DomainSceneBuilder`、`ViewportCameraController`、`OrientationMarkerController`、`VtkViewportWidget` | 首版**不新增** render 文件；导入后仍以长方体预览。`FaceId` 的权威定义应迁往 `core`（§10.2） |
| `src/ui/` | `ApplicationStyle`、`ProjectTreeWidget`、`InspectorPanel`、`ConfigurationPanel`、`MainWindow` | 需要新增导入向导、报告面板、各类 Inspector（§7、§10.4） |
| `src/core/` | **不存在** | 本需求的主要新增面 |
| `src/io/` | **不存在** | 本需求的主要新增面 |

补充事实：

- `src/render/PreviewDomain.h` 目前持有 `minX…maxZ` 共 6 个 `double` 与 `bounds()`，是“渲染侧临时域对象”；导入后的域必须来自 `core::DomainBox`，`PreviewDomain` 只作渲染侧快照（保持 `docs/ARCHITECTURE.md` §3.4 的“render 不是业务数据源”）。
- `tests/` 目前只有 `tests/render/DomainSceneBuilderTest.cpp`。没有任何解析、字典、黄金文件测试设施。

### 2.2 构建现状（`CMakeLists.txt`，144 行）

- `cmake_minimum_required(VERSION 3.24)`；`project(OpenFOAMGUI VERSION 0.1.0 LANGUAGES CXX)`；C++20；`AUTOMOC/AUTOUIC/AUTORCC ON`；`include(CTest)`。
- 目标：`ofgui_render`（STATIC，`src/render/*`，`PUBLIC` include 目录 `${CMAKE_CURRENT_SOURCE_DIR}/src`，链接 `Qt6::Widgets` + `${VTK_LIBRARIES}`）、`ofgui_ui`（STATIC，`src/ui/*`，链接 `ofgui_render` + `Qt6::Widgets`）、`OpenFOAMGUI`（`qt_add_executable ... MANUAL_FINALIZATION`，链接 `ofgui_ui`，`vtk_module_autoinit`）。
- 测试：`ofgui_render_scene_tests`（`tests/render/DomainSceneBuilderTest.cpp`）+ `add_test(NAME ofgui_render_scene_tests ...)`。
- 编译选项：MSVC `/W4 /permissive-`，否则 `-Wall -Wextra -Wpedantic`（三个目标各自设置，**新库必须照抄**）。
- `cmake/` 目录存在但为空；`resources/` 目录**尚不存在**。

### 2.3 文档与任务队列现状

| 结论 | 依据 |
| --- | --- |
| 导入已有算例目前被排到远期：`M6-03 导入已有 case 的基础信息` | `docs/TASKS.md` M6 段 |
| 本需求把它提前为首版能力，需要拆出新的任务编号（§13 的 `IMP-01…IMP-09` **已落入** `docs/TASKS.md`「算例识别与前处理（IMP）」小节） | 本规格 §13、`docs/TASKS.md` |
| `docs/ARCHITECTURE.md` §7.2 规定首版导出目录为 `0.orig/`；但导入的算例普遍是 `0/` | §9.3 给出目录名继承决策 |
| `docs/ARCHITECTURE.md` §8.2 的 `ValidationIssue` 没有源码定位字段 | §6.6 给出**追加**字段方案（不破坏既有字段） |
| `docs/ARCHITECTURE.md` §3.1 建议类清单里没有解析器、AST、投影器 | §10.2/§10.3 给出新增文件清单与依赖方向 |
| `docs/DECISIONS.md` 原只有 D-001…D-016；本规格涉及的 9 条决策已记录为 **D-017…D-025**（2026-10-27） | §14.1 |

---

## 3. 术语与约定

| 术语 | 含义 |
| --- | --- |
| case 根 | 用户选中的目录，其下至少有 `system/controlDict`（§4.1 判定规则） |
| 起始时间目录 | `0/` 或 `0.orig/`，二者取其一（§4.1） |
| AST | 字典语法树，`core::foam::FoamNode` 及其子类，纯数据、不依赖文件系统 |
| 投影（projection） | AST → GUI 领域模型的单向转换，不写文件 |
| 镜像（mirror） | 导出时把源 case 中未被 GUI 接管的文件原样复制到目标目录 |
| 重写（rewrite） | 导出时由 writer 依据模型重新生成的文件 |
| 支持级别 | `Editable` / `ReadOnly` / `Recognized` / `Unsupported`（§4 定义） |

约定：

- 所有文档内路径以 `OpenFOAM-v2206/` 为根的相对路径书写，行号为 1 基。
- 文档内 `只读参考` 指该文件仅用于确认 v2206 语法与样例，**不得**被本项目的构建或运行时依赖（`AGENTS.md` §2、`docs/DECISIONS.md` D-012、D-014）。
- 行号引用格式：`路径:行` 或 `路径:起-止`。

支持级别定义（全篇统一）：

| 级别 | 解析 | 进入模型 | UI 可编辑 | 导出行为 |
| --- | --- | --- | --- | --- |
| `Editable` | 是 | 是 | 是 | 重写 |
| `ReadOnly` | 是 | 是 | 否（只读展示） | 重写时保留原文值 |
| `Recognized` | 否（只探测存在性/大小/是否 gz） | 否（仅清单项） | 否 | 镜像复制（默认开）或用户取消 |
| `Unsupported` | 尝试识别边界并报 issue | 否 | 否 | 镜像复制（默认开） |

---

## 4. 识别范围与支持矩阵

### 4.1 目录级识别

| 目录 / 文件 | 判定规则 | 缺失时 | 级别 |
| --- | --- | --- | --- |
| `system/controlDict` | 存在即视为合法 case 根 | `IMPORT-CASE-001`，**BlockingError**（无法导入） | `Editable` |
| `0/` | 存在则用 | 回退 `0.orig/`；两者都无 → `IMPORT-CASE-002`（Error，可继续但无场） | `Editable` |
| `0.orig/` | 当 `0/` 不存在时使用 | 见上 | `Editable` |
| `constant/` | 存在即可 | 缺失 → `IMPORT-CASE-004`（Warning） | — |
| `system/` | 必须存在（与 `controlDict` 一致） | — | — |
| 其它时间目录（`0.0005`、`1.5`、`latest` 等） | 只按“目录名可解析为数值或 `0.orig`”列出**名称**，不读取内容 | — | `Recognized` |
| `Allrun`、`Allclean`、`All*` | 只列出 | — | `Recognized`（默认**不复制**，见 §9.2） |
| 符号链接 / 联接点 | 只列出，不跟随（v2206 tutorials 大量存在链接，跟随会读到 `OpenFOAM-v2206` 之外的路径） | — | `Recognized` |

> 时间目录命名由 `Time::timeName()` 生成（`OpenFOAM-v2206/src/OpenFOAM/db/Time/Time.C:780`），首版只认字面 `0` / `0.orig` 两个目录名，不做数值扫描。

### 4.2 场文件（起始时间目录下）

| 文件 | 判据 | 级别 | 说明 |
| --- | --- | --- | --- |
| `p`（`class volScalarField`） | 有合法 `FoamFile` 头 | `Editable` | 压力：`dimensions [0 2 -2 0 0 0 0]`（`tutorials/incompressible/icoFoam/cavity/cavity/0/p:17`） |
| `U`（`class volVectorField`） | 同上 | `Editable` | 速度：`dimensions [0 1 -1 0 0 0 0]`（同案例 `0/U:17`） |
| 其余 `class volScalarField` / `volVectorField`（`k`、`epsilon`、`omega`、`nut`、`nuTilda`、`T`、`p_rgh`、`alphat`…） | 同上 | `ReadOnly` | 解析后只读展示，导出时按 §9.2 处理；`tutorials/incompressible/simpleFoam/pitzDaily/0/` 有 7 个场文件可作样例 |
| `class volSymmTensorField` / `pointVectorField` / `pointScalarField` / `areaScalarField` / `areaVectorField` | 头可解析 | `ReadOnly`（有限） | 只解析 `FoamFile`、`dimensions`、`boundaryField` 键集合；`internalField` 只保留原文；`surfaceScalarField` 等其余 class → `Unsupported` |
| `class dictionary` 的杂项字典（如 `zoneID`） | 头可解析 | `Unsupported` | 镜像复制 |
| `format binary` 的任意文件 | 头第 11 行左右出现 `format binary;` | `Unsupported` | **不解析**，整文件跳过并报 `IMPORT-FMT-001`（Warning），镜像复制。样例：`tutorials/lagrangian/DPMFoam/Goldschmidt/0/U.air:11` |
| 头缺失 / 头里缺 `format` 或 `class` | — | 报 `IMPORT-PARSE-003` / `IMPORT-PARSE-004` | 该文件 `Unsupported`，其余文件继续解析 |

关于 `object` 键：v2206 读取时把 `version` 视为可选、`format`/`class`/`object` 视为必需、`note` 可选（`OpenFOAM-v2206/src/OpenFOAM/db/IOobject/IOobjectReadHeader.C:39-56`）。本项目**放宽**为：`format`、`class` 必需；`object` 缺失记 `IMPORT-PARSE-005`（Warning）并以文件名回填；`version`、`note`、`location` 只保存不校验。

### 4.3 `constant/` 子文件

| 文件 | 级别 | 读取内容 | 出处 |
| --- | --- | --- | --- |
| `constant/transportProperties` | `Editable` | `transportModel`（`Newtonian` 可编辑，其余只读）、`nu`（纯数值或带 `dimensions` 两种写法） | `tutorials/incompressible/icoFoam/cavity/cavity/constant/transportProperties:17`；带维度写法：`tutorials/incompressible/lumpedPointMotion/bridge/steady/constant/transportProperties:19` |
| `constant/turbulenceProperties` | `ReadOnly` | `simulationType`、`RAS{RASModel,turbulence,printCoeffs}`、`LES{LESModel,…}` | `tutorials/incompressible/simpleFoam/pitzDaily/constant/turbulenceProperties:17-28`；`tutorials/combustion/coldEngineFoam/freePiston/constant/turbulenceProperties` |
| `constant/polyMesh/` | `Recognized` | **只探测**：目录是否存在、`{points,faces,owner,neighbour,boundary}` 的 `.gz`/裸文件是否存在、字节大小、是否 gz | `tutorials/finiteArea/sphereSurfactantFoam/sphereTransport/constant/polyMesh/`（全部 `.gz`）；`tutorials/incompressible/adjointOptimisationFoam/resources/meshes/naca0012/polyMesh/`（全部 `.gz`） |
| `constant/g`、`constant/dynamicMeshDict`、`constant/thermophysicalProperties`、`constant/momentumTransport`、`constant/radiationProperties` 等 | `Recognized` | 只列名与大小 | 目录实存即证明 |
| `constant/` 下无法识别的其它文件 | `Recognized` | 只列名 | — |

`polyMesh/boundary` 的文本形状（仅供报错提示用，**不解析**）：`<nPatches> ( <patchName> { type …; nFaces …; startFace …; } … )`，样例 `tutorials/finiteArea/sphereSurfactantFoam/sphereTransport/constant/polyMesh/boundary.gz` 解压后 18-26 行。

给实现者的硬约束：`polyMesh` 存在时，**patch 名单不得**取自 `polyMesh/boundary`（本规格禁止解析它）。名单来源优先级见 §6.3。

### 4.4 `system/` 子文件

| 文件 | 级别 | 读取内容 | 出处 |
| --- | --- | --- | --- |
| `system/controlDict` | `Editable` | `application`、`startFrom`、`startTime`、`stopAt`、`endTime`、`deltaT`、`writeControl`、`writeInterval`、`purgeWrite`、`writeFormat`、`writePrecision`、`writeCompression`、`timeFormat`、`timePrecision`、`runTimeModifiable` | `tutorials/incompressible/icoFoam/cavity/cavity/system/controlDict:17-45` |
| `system/fvSchemes` | `Editable` | `ddtSchemes`、`gradSchemes`、`divSchemes`、`laplacianSchemes`、`interpolationSchemes`、`snGradSchemes`（各自 `default` + 条目表） | `cavity/system/fvSchemes:17-47` |
| `system/fvSolution` | `Editable` | `solvers` 子字典（`solver`、`preconditioner`、`smoother`、`tolerance`、`relTol`、`nSweeps`、`maxIter`）、算法字典 `PISO`/`PIMPLE`/`SIMPLE`（`nCorrectors`、`nNonOrthogonalCorrectors`、`pRefCell`、`pRefValue`、`momentumPredictor`、`residualControl`） | `cavity/system/fvSolution:17-48`（含 `$p;` 变量展开，见 §5.4） |
| `system/blockMeshDict` | `Editable`（**单块 hex**） | `scale`、`vertices`（8 点）、`blocks`（1 个 `hex`）、`edges`（必须为空或仅 `arc`/`spline` 的**识别**）、`boundary`（patch 名、`type`、`faces`） | `cavity/system/blockMeshDict:17-69` |
| `system/decomposeParDict` | `ReadOnly` | `numberOfSubdomains`、`method`、`coeffs{n}` | `cavity/system/decomposeParDict:17-24` |
| `system/PDRblockMeshDict` | `Recognized` | 只列名；与生成的 `blockMeshDict` 并存时给 `IMPORT-EXP-005`（Info） | `cavity/system/PDRblockMeshDict`（1262 B） |
| `system/mapFieldsDict`、`system/sampleDict`、`system/streamlines`、`system/snappyHexMeshDict`、`system/surfaceFeatureExtractDict` 等 | `Recognized` | 只列名 | `tutorials/incompressible/icoFoam/cavity/cavityClipped/system/mapFieldsDict`；`pitzDaily/system/streamlines` |
| `system/controlDict` 的 `functions{}` 子字典 | 不进入模型 | 整个 `functions` 子树保留原文；`Editable` 字典里出现它时记 `IMPORT-UNSUP-001`（Warning）并在导出时**原样保留**该子树 | v2206 tutorials 中 432 个 `controlDict` 里 181 个含 `functions`（本项目统计，见附录 B.3） |

`blockMeshDict` 的硬约束（v2206 事实，用来定“单块”边界）：

- `scale` 是标量（`cavity:17` `scale 0.1;`）；v2206 tutorials 的 381 个 `blockMeshDict` 里 `scale` 出现 376 次、旧写法 `convertToMeters` **0 次** → 只支持 `scale`。`scale` 为 `SymmTensor`（各向异性）时记 `IMPORT-MESH-003`（Warning）并只取对角。
- `blocks` 中出现 1 个以上 `hex` → `IMPORT-MESH-001`（**BlockingError**，拒绝进入可编辑导入，见 §11.2）；`pitzDaily` 为 5 个 `hex`（`tutorials/incompressible/simpleFoam/pitzDaily/system/blockMeshDict:66-84`）。
- `blocks` 里非 `hex` 元素（`poly`、`tet`、`wedge`、`quadTri`）→ `IMPORT-MESH-002`（BlockingError）。
- `grading` 只支持 `simpleGrading (x y z)`（3 值）；`edgeGrading`/`simpleGrading` 的 9 值/每边形式 → `IMPORT-MESH-005`（Warning，保留原文，`ReadOnly`）。样例：`pitzDaily` 的 `edgeGrading` 与 `$posY` 宏变量（`pitzDaily/system/blockMeshDict:70-86`）。
- `edges` 非空（`arc`/`spline`/`line`）→ 仍可导入，但域必须按“顶点包围盒”处理且记 `IMPORT-MESH-006`（Warning）；v2206 tutorials 有 408 处 `arc`。
- `vertices` 必须是 8 个 3 分量点，且能划分为长方体（每对轴只有 2 个不同值）；否则 `IMPORT-MESH-007`（Error，退回“只读导入”）。

### 4.5 字典语法子集

词法字母表直接采用 v2206 `token` 枚举（`OpenFOAM-v2206/src/OpenFOAM/db/IOstreams/token/token.H:77-107`）：

| v2206 token | 本项目处理 | 说明 |
| --- | --- | --- |
| `WORD` | 支持 | 关键字、`word` 值、文件名 |
| `STRING`（双引号） | 支持 | 关键字可带引号；`".*"` 作正则键 |
| `DIRECTIVE`（`#…`） | 识别但**不执行** | §5.5 |
| `EXPRESSION`（`${{ … }}`） | 不执行 | 记 `IMPORT-SYN-001` |
| `VARIABLE`（`$…`） | 部分展开 | §5.4 |
| `VERBATIM`（`#{ … #}`） | **必须**作为单个不透明 token 收下 | 否则 `codedFixedValue` 的 `code #{…#}` 会破坏整文件解析；样例 `tutorials/incompressible/simpleFoam/pipeCyclic/0.orig/U:31-40` |
| 标点 `( ) [ ] { } ; " # $` | 支持 | `(`/`)` 列表，`[`/`]` dimensionSet，`{`/`}` 字典 |
| 数字（整数/浮点/科学计数/负号前置） | 支持 | 保留原始字面文本，另存解析值 |
| 注释 `//`、`/* */` | 支持并**跳过** | banner 本身就是 `/* … */`（`cavity/0/p:1-7`） |
| 空白（含 CRLF、Tab） | 支持 | 导出统一 LF（`AGENTS.md` §5） |

语法子集（必须全部支持）：

| 构造 | 样例 | 出处 |
| --- | --- | --- |
| `FoamFile { … }` 头 | `version/format/class/object` | `cavity/0/p:8-14` |
| `keyword value;` | `nu 0.01;` | `cavity/constant/transportProperties:17` |
| `keyword [7 个指数] value;`（dimensioned） | `nu [0 2 -1 0 0 0 0] 1.5e-05;` | `tutorials/incompressible/lumpedPointMotion/bridge/steady/constant/transportProperties:19` |
| `keyword { … }`（嵌套字典） | `boundaryField { movingWall { … } }` | `cavity/0/p:21-37` |
| `keyword ( … )`（列表，可嵌套） | `vertices ( (0 0 0) … )` | `cavity/system/blockMeshDict:19-29` |
| `uniform <v>` | `internalField uniform 0;` | `cavity/0/p:19` |
| `nonuniform List<type> n ( … )` | `internalField nonuniform List<vector>` + `4096` + 4096 行 | `tutorials/DNS/dnsFoam/boxTurb16/0.orig/U:19-4118` |
| 多词值（tuple 摊平） | `type wall;`、`div(phi,U) Gauss linear;` | `cavity/system/fvSchemes:31` |
| 同一 keyword 多次出现（多值 entry） | 见 §5.6 | — |
| 引号正则键 | `".*"`、`"(free|hole)"` | `tutorials/basic/overLaplacianDyMFoam/heatTransfer/0.orig/pointDisplacement:23,29` |
| `patchType` 附加键 | `patchType overset;` | 同上 `:31` |

### 4.6 边界条件类型矩阵

统计基线：`OpenFOAM-v2206/tutorials/**/0*/` 下 2392 个文件里 `type <word>;` 的出现次数（本项目统计，附录 B.3）。级别定义见 §3。

| BC `type` | 出现次数 | 级别 | 可编辑参数 |
| --- | --- | --- | --- |
| `zeroGradient` | 1807 | `Editable` | 无 |
| `fixedValue` | 1204 | `Editable` | `value`（`uniform` 标量/向量） |
| `empty` | 1199 | `Editable` | 无（2D 前后面的强制类型，`docs/ARCHITECTURE.md` §4.4） |
| `noSlip` | 330 | `Editable` | 无 |
| `inletOutlet` | 838 | `Editable` | `inletValue`、`value`（可为 `$internalField`，见 §5.4） |
| `fixedFluxPressure` | 386 | `Editable` | `value` |
| `symmetryPlane` | 263 | `Editable` | 无 |
| `symmetry` | 43 | `Editable` | 无 |
| `slip` | 102 | `Editable` | 无 |
| `wedge` | 132 | `Editable` | 无 |
| `cyclic` | 196 | `ReadOnly` | `neighbourPatch`（只读） |
| `calculated` | 850 | `ReadOnly` | `value`（只读） |
| `uniformFixedValue` | 86 | `ReadOnly` | `uniformValue`（可为 `table ( … )`，只读保留原文） |
| `pressureInletOutletVelocity` | 177 | `ReadOnly` | `value`（只读） |
| `totalPressure` | 78 | `ReadOnly` | `p0`（只读） |
| `kqRWallFunction`、`nutkWallFunction`、`epsilonWallFunction`、`omegaWallFunction`、`turbulentIntensityKineticEnergyInlet`、`turbulentMixingLengthDissipationRateInlet`、`movingWallVelocity`、`mapped`、`overset`、`cyclicAMI` | 见附表 | `ReadOnly` | 全部参数只读 |
| `codedFixedValue`（含 `code`/`#{}` 块） | 2 | `Unsupported` | 整体只读原文；样例 `pipeCyclic/0.orig/U:28`、`tutorials/combustion/XiDyMFoam/oscillatingCylinder/0.orig/pointDisplacementy:32` |
| 其它未列出类型 | — | `ReadOnly`（默认策略） | 只读，绝不静默丢弃（§6.5） |

**硬约束**：任何未识别 `type` **禁止**导致字段被丢弃；必须按 `ReadOnly` 保留并用原文回写（`IMPORT-BC-001`）。

### 4.7 首版明确不支持项（评审用清单）

| # | 不支持项 | 行为 | 错误码 |
| --- | --- | --- | --- |
| 1 | 多块 `blockMeshDict`（>1 `hex`） | 拒绝可编辑导入，提示改为“只读镜像导出” | `IMPORT-MESH-001` (Blocking) |
| 2 | 非 `hex` 单块（`poly`/`tet`/`wedge`） | 同上 | `IMPORT-MESH-002` (Blocking) |
| 3 | `polyMesh` 内容解析 | 只探测存在性/大小 | — |
| 4 | `#include` / `#includeEtc` / `#includeIfPresent` / `#includeFunc` | 不展开，记 Warning，字典标 `Partial` | `IMPORT-DIR-001` |
| 5 | `#inputMode` / `#remove` / `#calc` / `#eval` / `#codeStream` / `#if` / `#ifeq` / `#message` | 不执行，记 Warning | `IMPORT-DIR-002` |
| 6 | `${{ … }}` 数学表达式 | 不求值 | `IMPORT-SYN-001` |
| 7 | `${a:-b}` / `${a:+b}` | 不展开 | `IMPORT-SYN-002` |
| 8 | `$..x` / `$...x` / `$^a.b` 作用域语法 | 不解析 | `IMPORT-VAR-002` |
| 9 | `format binary` 文件 | 跳过 | `IMPORT-FMT-001` |
| 10 | 非 UTF-8/ASCII（UTF-16 BOM、NUL 字节） | 拒绝该文件 | `IMPORT-ENC-001` |
| 11 | 正则 patch 键（`".*"`）参与编辑 | 只读展开展示，不进入模型 | `IMPORT-PATCH-001` |
| 12 | `snappyHexMesh` / 任意几何导入 | 不实现 | — |
| 13 | 多相/可压/燃烧模板与 `thermophysicalProperties` 语义 | 文件镜像，不校验 | — |
| 14 | 时间序列目录（多时刻）导入 | 只列名 | — |
| 15 | `decomposePar` 结果目录（`processor*`）语义 | 只列名 | — |
| 16 | 求解器执行与结果读取 | 不调用进程 | — |
| 17 | `#codeStream` 的编译执行 | 不执行（安全红线） | — |

---

## 5. 字典语法子集规范

本节给出实现 `io::FoamLexer` + `io::FoamParser` 的精确行为；每一条都对应 v2206 的源码或真实字典。

### 5.1 文件结构

```
[可选 banner 注释]           // cavity/0/p:1-7
FoamFile { … }              // 头字典，文件名无关
[注释与空行]
entry*
```

- 头的识别方式：文件开头（跳过注释与空白）的**第一条** entry 的 keyword 必须是 `FoamFile`（大小写敏感）；否则 `IMPORT-PARSE-003`（Error），全文按“无头字典”解析但仍可读。
- 若文件的第一个非注释 token 不是 `FoamFile`，且该文件是 `p`/`U`，视为致命（`Error`，字段标记 `Unsupported`）。

### 5.2 数值、字符串与保留文本

- 每个 token 保存 `text`（原始字面）与可选 `numeric`/`vector` 解析值；**禁止**把数值重新格式化为统一写法后再参与等价比较（等价比较用规范化数值，见 §9.4）。
- 字符串（`"…"`）保留引号内的原文，转义只处理 `\"`；其它反斜杠序列原样保留。
- `#{ … #}` verbatim 块整体收为一个 `FoamVerbatim` 节点，保留 `#` 后到 `#};` 之间的全部字节（含换行与缩进），用于只读回写。
- 非 ASCII 字节：不拒绝，但记 `IMPORT-ENC-002`（Warning），因为 `AGENTS.md` §5 要求“字典文本优先 ASCII”。导出时若某 token 含非 ASCII 且该文件由我们重写，则记 `IMPORT-EXP-007`（Warning）。

### 5.3 解析容错与前向恢复

| 场景 | 行为 |
| --- | --- |
| 缺 `;` | 在“本 entry 已读到值、下一个 token 是新 keyword”处报 `IMPORT-PARSE-001`，把新 keyword 当作下一条 entry 的开始（不中止文件） |
| 缺 `}` / `)` | 到达 EOF 时按“最近未闭合块”报 `IMPORT-PARSE-002`，位置指向**开括号所在行**（更易定位） |
| 多余的 `}` / `)` | 报 `IMPORT-PARSE-006`，跳过该 token 继续 |
| 未闭合字符串 | 报 `IMPORT-LEX-001`，跳到行尾继续 |
| 未闭合块注释 / verbatim | 报 `IMPORT-LEX-002` / `IMPORT-LEX-003`，到 EOF 截止 |
| 非法字符（控制字符、NUL） | 报 `IMPORT-LEX-004`，跳过 1 字节继续 |

**必须**：单个 entry 的错误不得导致同文件后续 entry 全部丢失；单个文件的错误不得中止其它文件解析（收集式）。总 issue 上限 200 条，超出时追加一条 `IMPORT-REPORT-001`（Info，“已截断”）。

### 5.4 `$` 变量展开子集

v2206 的变量语法（`OpenFOAM-v2206/src/OpenFOAM/db/dictionary/dictionary.H:55-74`）：

```
key2   $key1;              // 当前作用域（含递归查找）
key3   $.key1;             // 当前作用域（显式）
key4   $..key1;            // 父作用域
key5   $...key1;           // 祖父作用域
key6   $^subdict1.subdict2.key3;  // 绝对路径
```

实现要点（来自 `primitiveEntry.C:75-176`）：

- 查找使用 `keyType::LITERAL_RECURSIVE`，**不做正则匹配**（注释明确给出反例：`".*"` 会吞掉 `$internalField`）；本项目的 patch 正则键策略与此一致（§6.3）。
- 找不到字典项时回退到**环境变量**；仍为空则致命（v2206 直接 `FatalIOError`）。本项目：记 `IMPORT-VAR-001`（Error），该值标记 `Unresolved`，**不中止导入**。

支持子集（必须实现）：

| 语法 | 支持 | 说明 |
| --- | --- | --- |
| `$name` | 是 | 词法边界：`$` 后读到 `;`、空白、`)`、`}`、`(`、`[`、`]` 之一为止 |
| `${name}` | 是 | 同义 |
| `$.name` | 是 | 同义 |
| `$internalField`（BC 里引用场值） | 是 | 真实样例 `tutorials/basic/overPotentialFoam/cylinder/cylinderAndBackground/0.orig/U:41,48`、`p:40` |
| `$p;`（fvSolution 里复用子字典） | 是 | `cavity/system/fvSolution:29`；语义：把被引用子字典的条目**合并**进当前子字典，当前子字典已有键覆盖前者 |
| `$..`、`$...`、`$^a.b` | 否 | `IMPORT-VAR-002`（Warning），保留原文 |
| `${a:-b}`、`${a:+b}` | 否 | `IMPORT-SYN-002`（Warning） |
| `${{ … }}` | 否 | `IMPORT-SYN-001`（Warning）；样例 `tutorials/multiphase/compressibleInterFoam/laminar/climbingRod/0.orig/U:28` |

展开时机（与 §6.8 一致）：**解析期不展开**，AST 保留 `FoamVariableRef`；**投影期**按作用域链解析；解析失败只影响该字段。

### 5.5 `#` 指令取舍

v2206 的指令全集来自 `OpenFOAM-v2206/src/OpenFOAM/db/dictionary/functionEntries/`：`include`、`sinclude`（`includeIfPresent` 为其兼容名）、`includeEtc`、`includeFunc`、`inputMode`、`remove`、`calc`、`codeStream`、`eval`、`if`、`ifeq`、`message`。（`includeEntry.C:56-99` 显示 `include`/`sinclude`/`includeIfPresent` 的注册关系。）

| 指令 | 首版行为 | 错误码 | 理由 |
| --- | --- | --- | --- |
| `#include` | 不展开；记录目标路径与位置；字典标 `Partial` | `IMPORT-DIR-001` (Warning) | 展开需多文件行号映射与路径沙箱；cavity 零用例 |
| `#includeIfPresent` / `sinclude` | 同上 | `IMPORT-DIR-001` | — |
| `#includeEtc` | 同上（解析 `$FOAM_ETC` 需要环境变量，本项目不读环境） | `IMPORT-DIR-001` | `tutorials/incompressible/simpleFoam/pipeCyclic/0.orig/U:23` |
| `#includeFunc` | 同上 | `IMPORT-DIR-001` | 依赖 `functions{}` 语义 |
| `#inputMode` / `#remove` / `#calc` / `#eval` / `#codeStream` / `#if` / `#ifeq` / `#message` | 不执行，`Unsupported`，原文保留 | `IMPORT-DIR-002` (Warning) | `#codeStream` 会编译执行代码，属安全红线 |
| 未知 `#word` | `Unsupported`，原文保留 | `IMPORT-DIR-003` (Warning) | 前向兼容 |

真实影响面（必须写进导入报告首屏）：`etc/caseDicts/` 里 `#include` 出现 60 次、`#includeEtc` 54 次；`tutorials/**/0*/` 里 519 处 `#include`。典型情形 `tutorials/basic/chtMultiRegionFoam/2DImplicitCyclic/0.orig/p:23`（`#include "include/emptyPatches"`，**无分号**）。

给未来扩展的备忘（不是首版要求）：若以后展开 `#include`，相对路径解析基准是**包含该指令的文件所在目录**（`includeEntry.C:137` `resolveFile(is.name().path(), rawName, parentDict)`），不是 case 根。

### 5.6 多值与重复关键字

- 同一字典内同一 keyword 出现多次 → 保留为**有序条目序列**（不合并、不去重），记 `IMPORT-SYN-003`（Warning）并列出所有出现位置；GUI 只展示第一条，导出时**全部按原序回写**（保证等价性）。
- 一条 entry 的值由多个 token 组成（如 `div(phi,U) Gauss linear;`）→ `FoamValueSequence`，保留 token 序列与原文。

### 5.7 编码、换行、BOM

| 项 | 规则 |
| --- | --- |
| 编码 | 只接受 UTF-8 / ASCII；UTF-16 BOM（`FF FE` / `FE FF`）或文件头 1KB 内出现 `0x00` → `IMPORT-ENC-001`（Error），整文件 `Unsupported` |
| BOM | UTF-8 BOM 允许，读取时剥离并在导出时**不写回**（记 `IMPORT-ENC-003` Info） |
| 换行 | 读取容忍 CRLF/LF/CR；写出统一 LF（`AGENTS.md` §5） |
| 尾随空白 | 写出时去掉行尾空白 |

### 5.8 词法/语法 API 契约（给 io 实现者）

```cpp
// src/io/FoamLexer.h
struct FoamToken { core::foam::FoamSyntaxKind kind; std::string text;
                   std::size_t byteOffset; int line; int column; };

class FoamLexer {
public:
    explicit FoamLexer(std::string_view source);
    std::optional<FoamToken> next();      // EOF 返回 nullopt
    const core::ValidationResult& diagnostics() const;
};
```

**禁止**：lexer 抛异常作为常规控制流；**禁止**在 lexer 里做字符集转换（保持字节切片 + 独立 `TextFileReader` 负责解码）。

---

## 6. 数据结构映射

### 6.1 总图

```
io::TextFileReader → std::string source
        ↓
io::FoamLexer → FoamToken*
        ↓
io::FoamParser → core::foam::FoamDictionary (AST, 含每节点 SourceLocation)
        ↓
core::import::*Projector → core 模型对象（DomainBox / MeshSettings / BoundaryPatch /
                            FieldDefinition / BoundaryCondition / …）
        ↓
core::CaseProject（聚合根）
        ↓
core::CaseValidator → core::ValidationResult
        ↓
ui 面板 / render 预览 / io::OpenFoamCaseExporter → GeneratedFile*
```

依赖方向必须为 `ui → io → core`、`ui → render`、`render → core`（与 `docs/ARCHITECTURE.md` §2 一致，新增 `ofgui_core` 位于最底层）。

### 6.2 计算域与网格（`system/blockMeshDict` → `DomainBox` + `MeshSettings`）

| 字典路径 | 模型字段 | 类型 | 缺失/异常策略 |
| --- | --- | --- | --- |
| `scale` | `MeshSettings.scale` | `double` | 缺失 → 默认 `1.0` + `IMPORT-MESH-008`（Warning）；`SymmTensor` → 取对角 + `IMPORT-MESH-003` |
| `vertices[0..7]` | `MeshBlock.vertices`（8×3 原始值） | `std::array<Vec3d,8>` | 非 8 点 → `IMPORT-MESH-004`（Error，`Unsupported`） |
| `vertices × scale` 的包围盒 | `DomainBox.min` / `DomainBox.max` | `Vec3d` | 非长方体（出现第 3 个不同坐标值）→ `IMPORT-MESH-007`（Error） |
| `blocks[0]` 的 `(nx ny nz)` | `MeshSettings.cells` | `std::array<int,3>` | 任一 ≤0 → `IMPORT-MESH-009`（Blocking） |
| `blocks[0]` 的 `simpleGrading (x y z)` | `MeshSettings.grading` | `std::array<double,3>` | 缺失 → `1 1 1`；9 值/每边形式 → `IMPORT-MESH-005`，保留原文 `ReadOnly` |
| `blocks` 元素个数 | `MeshBlock.count` | `int` | >1 → `IMPORT-MESH-001`（Blocking） |
| `boundary` 的每个 `name` | `BoundaryPatch.name` | `std::string` | 空名 → `IMPORT-PATCH-004`（Error） |
| `boundary.name.type` | `BoundaryPatch.type` | 枚举 | 非法值 → `IMPORT-PATCH-005`（Error，回落 `patch`） |
| `boundary.name.faces` | `BoundaryPatch.faces` | `std::set<FaceId>` | 面重复归属 → `IMPORT-PATCH-006`（Error，交给 `FaceAssignmentValidator`） |

`DomainBox` 归一化规则：

- `min = min(vertex × scale)`、`max = max(vertex × scale)`，**按轴分别取**，不假设顶点顺序。
- `mode`：若 `nz == 1` 或 `(maxZ - minZ)` 为唯一非零且厚度远小于其它轴 → 建议 `TwoD` 并记 `IMPORT-DOM-001`（Info，仅建议，不自动改）。判定阈值写进 `core::DomainBox::suggestMode()`，不散落在 UI。
- `unit`：v2206 字典无单位信息 → 恒为 `"m"`，并记 `IMPORT-DOM-002`（Info，“尺寸单位按 SI 解释，未从字典读取”）。

### 6.3 patch 名单来源优先级（硬约束）

| 优先级 | 来源 | 采用条件 | 备注 |
| --- | --- | --- | --- |
| 1 | `system/blockMeshDict` 的 `boundary` 精确名 | 存在且单块 | 权威来源（我们重写它） |
| 2 | 起始时间目录各场的 `boundaryField` **精确**键（去重并集） | 无 `blockMeshDict` | `type` 未知 → `BoundaryPatch.type = patch` + `IMPORT-PATCH-007`（Warning） |
| — | `constant/polyMesh/boundary` | **禁止**使用 | §4.3 |
| — | 正则键（`".*"`、`"(free|hole)"`） | **禁止**进入模型 | `IMPORT-PATCH-001`（Warning），只读展示；v2206 `0*/` 里有 797 处 |

一致性校验（`CaseCrossValidator`）：

- `blockMeshDict.boundary` 与（来源 2）不一致 → `IMPORT-PATCH-008`（Warning，列出差异集合）。
- 某 patch 在 `p`/`U` 的 `boundaryField` 里没有精确键 → `IMPORT-PATCH-002`（Error，必须补 BC 才能导出）。

### 6.4 场与边界条件（`0/` → `FieldDefinition` / `BoundaryCondition`）

| 字典路径 | 模型字段 | 类型 | 缺失/异常策略 |
| --- | --- | --- | --- |
| `FoamFile.class` | `FieldDefinition.className` | 枚举 | 非 `volScalarField`/`volVectorField` → 该文件 `ReadOnly` 或 `Unsupported`（§4.2） |
| `FoamFile.object` / 文件名 | `FieldDefinition.name` | `std::string` | 二者不一致 → `IMPORT-PARSE-005`（Warning，以 `object` 为准） |
| `dimensions` | `FieldDefinition.dimensions` | `std::array<int,7>` | 缺失或元素数 ≠ 7 → `IMPORT-FIELD-001`（Error）；7 个指数顺序见下 |
| `internalField` | `FieldDefinition.internalField` | `FieldValue` | `uniform` → 可编辑；`nonuniform` → 只读原文（§6.5） |
| `boundaryField.<patch>.type` | `BoundaryCondition.type` | `std::string`（保留原文） | 缺失 → `IMPORT-BC-002`（Error） |
| `boundaryField.<patch>.<param>` | `BoundaryCondition.parameters[name]` | `FoamNodeRef` + 规范化值 | 见 §4.6 的白名单 |

`dimensions` 的 7 个指数顺序（`OpenFOAM-v2206/src/OpenFOAM/dimensionSet/dimensionSet.H` 的 `enum dimensionType`）：

| 位置 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 物理量 | 质量 MASS | 长度 LENGTH | 时间 TIME | 温度 TEMPERATURE | 物质的量 MOLES | 电流 CURRENT | 发光强度 LUMINOUS_INTENSITY |

**硬约束**：`dimensions` **必须原值保留**，不得按“icoFoam 压力应为 `[0 2 -2 0 0 0 0]`”之类规则改写。反例足以说明：`tutorials/basic/chtMultiRegionFoam/2DImplicitCyclic/0.orig/p:17` 是 `[1 -1 -2 0 0 0 0]`。

### 6.5 `FieldValue` 与原文保真

```cpp
// src/core/FieldValue.h
enum class FieldValueKind { Uniform, NonUniformList, VariableRef, Sequence, Raw, Unresolved };

struct FieldValue {
    FieldValueKind kind;
    std::string text;                  // 折叠空白后的原文（用于回写与展示）
    std::optional<double> scalar;      // Uniform 标量
    std::optional<Vec3d> vector;       // Uniform 向量
    std::optional<std::size_t> count;  // NonUniformList 的元素数
    std::optional<SourceLocation> location;
};
```

规则：

- `Uniform`：可编辑，写回时用统一格式（整数无小数点、浮点用 `%.6g` 或保留原字面 —— 见 §9.4 的等价判定）。
- `NonUniformList`：`ReadOnly`，`text` 保存**整段原文**（从 `nonuniform` 到列表结束的 `)`），导出时逐字回写 → 保证 `boxTurb16` 这类 4096 值场不因重排而漂移。
- `VariableRef`：保存变量名；解析结果另存 `resolved`，UI 展示“`$internalField` → `uniform (0 0 0)`”。
- `Unresolved`：解析失败（§5.4），UI 只读展示原文并挂 issue。

### 6.6 `ValidationIssue` 扩展（不破坏既有字段）

`docs/ARCHITECTURE.md` §8.2 已有 `severity/code/message/targetObjectId/targetField/suggestedFix`。本规格**追加**（全部带默认值，既有构造点不受影响）：

```cpp
// src/core/SourceLocation.h
struct SourceLocation {
    std::string file;          // case 相对 POSIX 路径，如 "system/blockMeshDict"
    int line = 0;              // 1 基
    int column = 0;            // 1 基，按 UTF-8 码点计
    std::size_t byteOffset = 0;
};

// src/core/ValidationIssue.h（追加字段）
struct ValidationIssue {
    Severity severity;             // Info | Warning | Error | BlockingError
    std::string code;              // 稳定错误码，见附录 C
    std::string message;           // zh-CN，面向用户，不含英文堆栈（D-039：core/io 数据文本，不走 tr()）
    ObjectId targetObjectId = 0;   // 0 表示无具体对象
    std::string targetField;       // 如 "mesh.cells[0]"
    std::string suggestedFix;
    // ---- 本规格追加 ----
    std::optional<SourceLocation> location;  // 源位置（导入/解析类 issue 必须填）
    std::string nodePath;                    // 字典路径，如 "boundaryField.movingWall.type"
    std::string snippet;                     // 该行原文，去尾空白，≤160 字符
    std::string origin;                      // "import" | "model" | "export"
};
```

已记录为 **D-017**（2026-10-27，`docs/DECISIONS.md`）：四个字段均带默认值（空/`std::nullopt`），既有构造点与聚合初始化保持可编译；`origin` 取值 `import`｜`model`｜`export`。

### 6.7 导入报告

```cpp
// src/core/ImportReport.h
struct ImportedFile {
    std::string relativePath;      // "0/p"
    std::string kind;              // "field" | "dict" | "mesh-dir" | "script" | "other"
    SupportLevel level;            // Editable | ReadOnly | Recognized | Unsupported
    std::size_t byteSize = 0;
    bool gzip = false;
    std::string objectClass;       // FoamFile.class（可空）
    bool partial = false;          // 是否因 #include 等被标 Partial
};

struct ImportReport {
    std::string caseRoot;          // 绝对路径（仅 UI 展示用，不入模型持久化）
    std::string startDirectory;    // "0" | "0.orig"
    std::string foamVersionNote;   // banner 里读到的 "v2206"（可空）
    std::vector<ImportedFile> files;
    std::vector<ValidationIssue> issues;
    int editableCount, readOnlyCount, recognizedCount, unsupportedCount;
    bool blocking() const;         // 存在 BlockingError
};
```

**禁止**把 `caseRoot` 绝对路径写进 `.ofgproj`（`AGENTS.md` §5：“不允许在源码中散落绝对路径”的同一精神：工程文件只存相对路径 + 用户可重定位的根）。

`.ofgproj` 的 `source` 段（**D-025**）：

```jsonc
"source": {
  "caseRelativePath": "../cavity",     // 相对 .ofgproj 所在目录；用户可重定位
  "startDirectory": "0",               // "0" | "0.orig"
  "importedAt": "2026-10-27T22:30:00+08:00",
  "files": [                           // 每个被读取的源文件一条，用于“源已变化”提示
    { "relativePath": "0/p", "sha256": "<64 hex>" },
    { "relativePath": "system/blockMeshDict", "sha256": "<64 hex>" }
  ]
}
```

- `caseRelativePath` 为空 ⇒ 新建工程（无源）。
- 重新打开工程时逐文件比对 SHA-256：不一致弹“源算例已在外部修改”，提供“重新导入/忽略”。
- 哈希计算必须流式读取（大 `polyMesh` 不整读进内存），由 `io` 的 `FileHash` 工具负责；不引入第三方密码库。

落地状态（t17，详见 `docs/DECISIONS.md` D-034）：

- `.ofgproj` 升到 **schemaVersion 2**，且 `source` 段**始终写出**（新建工程为 `caseRelativePath: ""` + `files: []`）。
- 读 v1 文件按“无来源”处理并在内存中迁移（Info `PROJ-SCHEMA-004`），**加载不回写文件**；保存时一律写 2；版本 > 2 拒绝（`PROJ-SCHEMA-002`）。
- `sha256` 必须 64 位**小写**十六进制（`PROJ-MODEL-008`）；同一 `relativePath` 重复 → Warning `PROJ-MODEL-009`（保留首条）。
- `caseRelativePath` / `files[].relativePath` 为绝对路径时：**读为 Error `PROJ-PATH-002`，写为 Warning 且原值照写**（不静默丢数据）。
- `caseRelativePath` 为空即“无来源”，其余键不再生效；若 `files` 非空追加 Warning `PROJ-MODEL-010`。

### 6.8 投影器职责切分（每个投影器只做一个字典）

| 投影器 | 输入 | 输出 | 关键约束 |
| --- | --- | --- | --- |
| `BlockMeshProjector` | `system/blockMeshDict` AST | `DomainBox` + `MeshSettings` + `std::vector<BoundaryPatch>` | 单块；`scale` 必乘 |
| `FieldProjector` | 一个 `0/*` AST | `FieldDefinition` | 只处理 `volScalarField`/`volVectorField` |
| `BoundaryFieldProjector` | `boundaryField` 子字典 | `std::vector<BoundaryCondition>` | 正则键不入模型 |
| `BoundaryConditionFactory` | `type` + 参数 AST | `BoundaryCondition` + `SupportLevel` | 白名单见 §4.6，未知类型→`ReadOnly` |
| `TransportPropertiesProjector` | `constant/transportProperties` | `TransportModelSettings` | `nu` 两种写法都要吃 |
| `TurbulencePropertiesProjector` | `constant/turbulenceProperties` | `TurbulenceSettings` | 只读 |
| `ControlDictProjector` | `system/controlDict` | `ControlDictSettings` | `functions{}` 原样保留 |
| `FvSchemesProjector` | `system/fvSchemes` | `FvSchemesSettings` | 6 类 scheme；未知类记 Warning |
| `FvSolutionProjector` | `system/fvSolution` | `FvSolutionSettings` | `$p;` 合并语义 |
| `DecomposeParProjector` | `system/decomposeParDict` | `DecomposeSettings` | 只读 |
| `PatchNameResolver` | 上述集合 | 最终 patch 名单 | 优先级见 §6.3 |

**禁止**：投影器里做文件 IO；投影器不接收路径，只接收 AST + `ImportContext`。

---

## 7. UI 需要的字段清单

通用规则：**UI 层**用户可见文本走 `tr()`（core/io 的诊断文本是数据、不走 `tr()`，见 D-039 与 §8.7）；技术名词保留 OpenFOAM 原文（`AGENTS.md` §10）；来源为 `ReadOnly` 的字段一律 `setReadOnly(true)` 并加 ToolTip 说明原因与源位置；每个可编辑字段失焦即写回模型并触发 `CaseValidator`（不做“保存”按钮再统一提交）。

### 7.1 导入向导（`CaseImportDialog`）

| 控件 | 类型 | 绑定 | 校验/说明 |
| --- | --- | --- | --- |
| 算例根目录 | `QLineEdit` + 浏览按钮 | `ImportOptions.caseRoot` | 必须存在 `system/controlDict`，否则禁用“下一步” |
| 起始时间目录 | `QComboBox`（`0/`、`0.orig/`） | `ImportOptions.startDirectory` | 只列实际存在的项 |
| 镜像未接管文件 | `QCheckBox` | `ImportOptions.mirrorUnknownFiles` | 默认勾选（§9.2） |
| 覆盖已有 `.ofgproj` | `QCheckBox` | — | 目标目录已有工程文件时默认不勾选 |
| 导入进度 | `QProgressBar` | 服务信号 | 按文件推进；解析在后台线程，UI 线程只更新计数 |

### 7.2 导入报告页（`CaseImportReportPanel`）

| 控件 | 类型 | 数据 |
| --- | --- | --- |
| 摘要行 | `QLabel` × 5 | 可编辑/只读/仅识别/不支持 文件数 + 阻断错误数 |
| 文件表 | `QTreeWidget`（4 列：路径、级别、class、大小） | `ImportReport::files` |
| 级别过滤 | `QComboBox` | 全部 / Editable / ReadOnly / Recognized / Unsupported |
| 问题列表 | `IssueListView` | `ImportReport::issues` |
| “继续导入”按钮 | `QPushButton` | `blocking() == false` 才可用 |

### 7.3 计算域与网格（`DomainInspector` + `MeshInspector`）

| 字段 | 控件 | 模型 | 校验 |
| --- | --- | --- | --- |
| `min` / `max`（6 个分量） | 6 × `QDoubleSpinBox`（6 位小数） | `DomainBox.min/max` | `min < max` 逐轴；否则阻断保存 |
| 单位 | `QLabel`（只读） | `DomainBox.unit` | 首版固定 `m`（§6.2） |
| 模式（2D/3D） | `QComboBox` | `DomainBox.mode` | 3D 时 `nz ≥ 2`；2D 时强制 `nz = 1` 且 `frontAndBack` 必须 `empty` |
| `scale` | `QDoubleSpinBox` | `MeshSettings.scale` | `> 0`；非 1 时显示“几何 = 顶点 × scale”提示 |
| `cells (nx ny nz)` | 3 × `QSpinBox` | `MeshSettings.cells` | `≥ 1`；估算单元数超 5e6 → Warning |
| `grading (x y z)` | 3 × `QDoubleSpinBox` | `MeshSettings.grading` | `> 0`；`edgeGrading` 存在时整组只读 |
| 块数 | `QLabel` | `MeshBlock.count` | >1 时显示“不支持多块”红字并禁用编辑 |

### 7.4 边界面（`BoundaryPatchInspector`）

| 字段 | 控件 | 模型 | 说明 |
| --- | --- | --- | --- |
| patch 列表 | `QTreeWidget`（名称 + 类型 + 面数） | `CaseProject.patches` | 顺序与 `blockMeshDict.boundary` 一致 |
| 名称 | `QLineEdit` | `BoundaryPatch.name` | 唯一性由 `FaceAssignmentValidator` 校验；`[A-Za-z0-9_.-]+`（避免污染字典键） |
| 类型 | `QComboBox`（`patch`/`wall`/`empty`/`symmetry`/`symmetryPlane`/`wedge`/`cyclic`/`processor`） | `BoundaryPatch.type` | `cyclic` 需 `neighbourPatch`（首版只读） |
| 面勾选 | 6 × `QCheckBox`（±X/±Y/±Z） | `BoundaryPatch.faces` | 同一面不得跨 patch；冲突即时红框 |
| 源位置 | `QLabel`（可点击跳转） | `SourceLocation` | 如 `system/blockMeshDict:57` |

### 7.5 场与边界条件（`FieldInspector`）

| 字段 | 控件 | 模型 | 说明 |
| --- | --- | --- | --- |
| 场列表 | `QComboBox` / `QTreeWidget` | `CaseProject.fields` | `p`、`U` 可编辑；其它只读图标 |
| `class` | `QLabel` | `FieldDefinition.className` | 只读 |
| `dimensions` | 7 × `QSpinBox` | `dimensions` | 只读（§6.4 硬约束）；悬停显示“来源 <file>:17” |
| `internalField` | 标量：`QDoubleSpinBox`；向量：3 × `QDoubleSpinBox`；不可编辑时 `QLabel` | `FieldValue` | `NonUniformList` → 只读 + “N 个值”摘要 |
| patch × BC 表 | `QTableWidget`（行 = patch，列 = type + 参数） | `BoundaryCondition` | 未知 `type` 行整行只读并灰色 |
| BC 参数编辑 | `QLineEdit`（按参数名动态生成） | `BoundaryCondition.parameters` | 参数内联校验（数值/向量/`uniform`） |
| 原文预览 | `QPlainTextEdit`（只读，折叠） | 该场原文 | 用于核对 `$internalField`、`#{…#}` |

**首版硬约束（D-024）**：`ReadOnly`/`Recognized` 级别场**不可改名、不可删除**——列表右键菜单隐藏“重命名/删除”，或置灰并给 tooltip（“该场为只读导入内容；改名需同步更新 `fvSolution.solvers{}`、`controlDict.functions` 等跨字典引用，首版不支持”）。仅 `Editable`（`p`、`U` 及新建工程的场）可改名/删除；`Editable` 场改名时由 `CaseCrossValidator` 检查 `boundaryField` 键与 `fvSolution.solvers{}` 键是否同步，未同步则报 `IMPORT-XREF-001`（Warning，提示需手动修 `fvSolution`）。

### 7.6 求解设置（`SolverInspector`）

分组控件（`QGroupBox`）：

1. **controlDict**：`application`（`QLineEdit`，合法值来自已知应用清单，含 `icoFoam`）、`startFrom`/`stopAt`/`writeControl`/`writeFormat`/`timeFormat`（`QComboBox`）、`startTime`/`endTime`/`deltaT`/`writeInterval`/`writePrecision`/`timePrecision`（`QDoubleSpinBox`/`QSpinBox`）、`purgeWrite`（`QSpinBox`）、`writeCompression`（`QCheckBox`）、`runTimeModifiable`（`QCheckBox`）。
2. **transportProperties**：`transportModel`（`QComboBox`，`Newtonian` 可编辑，其它只读）、`nu`（`QDoubleSpinBox`，`1e-12…1`，对数步进 0.01）。
3. **turbulenceProperties**（只读）：`simulationType` + 模型名。
4. **fvSchemes**：6 类 scheme 的 `default` 与条目表（`QTableWidget`：关键字、值）。
5. **fvSolution**：`solvers` 表（字段 × 字典）、算法选择（`PISO`/`PIMPLE`/`SIMPLE`）+ 其参数（各 `QSpinBox`/`QDoubleSpinBox`/`QCheckBox`）。
6. **decomposeParDict**（只读）：`numberOfSubdomains`、`method`、`coeffs{n}`。

### 7.7 校验面板（`ValidationPanel`）

| 控件 | 数据 |
| --- | --- |
| 分级统计 | 4 × `QLabel`（阻断/错误/警告/提示） |
| 问题列表 | `QTreeWidget`（严重级、码、消息、位置） |
| 双击 | 导航到目标面板并高亮字段；有 `location` 时打开只读源预览并滚动到行 |
| 修复建议列 | `suggestedFix` 文本；“应用建议”按钮只在 `suggestedFix` 可自动化时出现（首版只做 `cells`/`nu` 两项） |

### 7.8 导出预览（`ExportPreviewDialog`）

| 控件 | 数据 |
| --- | --- |
| 目标目录 | `QLineEdit` + 浏览（默认 `<caseRoot>_export`；禁止等于源目录、禁止位于源 case 内（D-022），§9.5） |
| 文件表 | `QTreeWidget`（路径、动作 `rewrite`/`mirror`/`skip`、字节） |
| 差异预览 | `QPlainTextEdit` 左右对照（源 vs 将写出） |
| 阻断提示 | 存在 `BlockingError` 时“导出”置灰并链接到 `ValidationPanel` |
| 结果 | 成功显示目标绝对路径 + 文件数 + 耗时 |

---

## 8. 导入失败的错误定位要求

### 8.1 分级语义与阻断规则

| 级别 | 含义 | 是否阻断导入 | 是否阻断导出 |
| --- | --- | --- | --- |
| `BlockingError` | 无法建立可用模型（无 `controlDict`、多块、非 hex、无法解析 `blocks`） | 是 | 是 |
| `Error` | 局部数据缺失/非法，模型可建立但该部分不可用 | 否（该子对象标记不可用） | 是（导出前必须修复） |
| `Warning` | 有损/降级：`#include` 未展开、未知 BC 类型、只读保留 | 否 | 否（但导出预览必须列出） |
| `Info` | 事实告知：单位假设、`PDRblockMeshDict` 存在、BOM 剥离 | 否 | 否 |

### 8.2 定位要求（强制）

每一条 `origin == "import"` 的 issue **必须**携带：

1. `location.file`：case 相对 POSIX 路径（如 `system/blockMeshDict`），不使用绝对路径；
2. `location.line`、`location.column`：1 基；列指向引起问题的 token 起点；
3. `nodePath`：字典路径，如 `boundaryField.movingWall.type`；
4. `snippet`：该行原文（去尾空白，≤160 字符，超长用 `…` 截断但保留首尾）；
5. `suggestedFix`：可执行的一句话中文建议（例：“`blocks` 只允许单个 `hex`，请改用单块模板或选择镜像导出”）。

禁止：只给“解析失败”而无位置；禁止用行号 0 表示未知（未知时用 `std::nullopt` 并在 UI 显示“位置未知”）。

### 8.3 报告呈现顺序

排序键：`severity` 降序 → `file` 字典序 → `line` 升序 → `code`。首屏必须显示：case 根、起始时间目录、阻断错误（若有）、被标 `Partial` 的文件清单、`Unsupported` 文件清单。

### 8.4 降级策略（不得静默丢数据）

| 情形 | 模型状态 | UI | 导出 |
| --- | --- | --- | --- |
| 某场文件 `Unsupported` | 不进入模型 | 报告里列出 | 镜像原文件 |
| 某 BC 参数无法解析 | `FieldValue::Unresolved` | 只读显示 `$…` 原文 | 原样回写该参数 |
| `#include` 未被展开 | 字典 `partial = true` | 文件名旁挂“含未展开指令”图标 | 重写时**保留该指令行原文** |
| `blocks` 有多块 | `MeshSettings` 不可编辑 | 红字阻断 | 仅允许“镜像导出”模式 |

### 8.5 日志要求

- 日志与 issue 分离：issue 面向用户（中文、可定位）；日志面向开发者（英文、含 token 类型与字节偏移）。
- 日志编码 UTF-8，路径写相对路径；**禁止**把 `OpenFOAM-v2206` 的绝对路径写进用户可见 issue。

### 8.6 规模上限（超限必须优雅失败，不得卡死 UI）

| 项 | 上限 | 超限行为 |
| --- | --- | --- |
| 单个字典文件 | 32 MiB | `IMPORT-LIM-001`（Error），跳过该文件 |
| 单个 case 扫描文件数 | 5000 | `IMPORT-REPORT-002`（Warning），只扫描 `0/`、`constant/`、`system/` 三层 |
| `nonuniform` 列表元素 | 5e6 | `IMPORT-LIM-002`（Warning），只保留原文不建索引 |
| issue 总数 | 200 | 截断 + `IMPORT-REPORT-001` |
| 解析总时长 | 10 s（软目标） | 超时只记 Info，不中断 |

### 8.7 诊断文本的语言归属与本地化（D-039）

core/io（`ofgui_core`、`ofgui_io`）是 Qt-free 层，它的诊断文本是**数据**而不是 UI 文本，因此**不使用 `tr()`**：

| 位置 | 内容 | 形态 | 本地化方式 |
| --- | --- | --- | --- |
| `src/core/ValidationIssue` | `message`、`suggestedFix` | UTF-8 `std::string` 中文常量 | UI 按 `code` 映射，原文本作回退 |
| `src/core/CaseValidator*`（Domain/Fields/Mesh/Patches/Solver） | 规则说明与建议（51 行中文） | 同上 | 同上 |
| `src/core/ValidationResult::formatCounts()` | “阻断 N、错误 N、警告 N、提示 N”计数摘要 | 同上 | 同上；UI 可改用自身 `tr()` 的组合式摘要 |
| `src/io/ImportIssueFactory` | `IMPORT-*` / `CASE-*` 码 → 中文文案（75 行） | 同上（码表，与附录 C 同源） | 同上 |

强制规则：

1. **稳定标识是 `code`，不是文案**：排序键（§8.3）、`ValidationNavigator` 定位、`ExportDictionaryCheck`、测试断言一律匹配 `code`/`targetField`/`targetObjectId`，禁止匹配中文文本。
2. **UI 自有文案必须 `tr()`**：severity 标签（`ValidationPanel`、`NodeDetailHtml`、`ExportController`）、按钮、标题、ToolTip、占位符走 `QObject::tr()`/`QCoreApplication::translate()`；`AGENTS.md` §10 的 `tr()` 要求只约束 UI 层。
3. **不得为文案给 core/io 引入 Qt**：不 `#include <QObject>`、不用 `QString`、不把 `ofgui_core` 链到 `Qt6::*`；新增中文文案必须配 `code` 且源文件为 UTF-8 无 BOM。
4. **多语言放在 UI 层**：引入英文界面时先在 `src/ui` 建“码 → 文案”映射文件（一码表一文件），再切界面语言；core/io 不参与 `.ts` 提取，翻译覆盖率统计只算 UI 层。

---

## 9. 导出规格与黄金文件

### 9.1 输出布局

```
<target>/                     // 用户选定或默认 `<caseRoot>_export`
├── 0/ 或 <startDirectory>/   // 继承源目录名（D-018）
│   ├── p                     // rewrite（Editable）
│   ├── U                     // rewrite（Editable）
│   └── <其它场>              // rewrite（ReadOnly 保真）或 mirror
├── constant/
│   ├── transportProperties   // rewrite
│   ├── turbulenceProperties  // rewrite（只读内容原样）
│   └── polyMesh/             // mirror（若源存在）
├── system/
│   ├── blockMeshDict         // rewrite
│   ├── controlDict           // rewrite
│   ├── fvSchemes             // rewrite
│   ├── fvSolution            // rewrite
│   └── decomposeParDict      // rewrite 或 mirror
└── .ofgport-export.json      // 导出清单（不含绝对路径）
```

### 9.2 文件动作表（`OutputFileAction`）

| 文件 | 动作 | 理由 |
| --- | --- | --- |
| `system/blockMeshDict` | `rewrite` | 由 `DomainBox`+`MeshSettings`+`patches` 生成 |
| `system/controlDict` | `rewrite` | 保留未接管键与 `functions{}` 原文 |
| `system/fvSchemes` | `rewrite` | 保留未识别 scheme 类 |
| `system/fvSolution` | `rewrite` | `$p;` 合并后按语义写出（展开而非保留 `$p`） |
| `system/decomposeParDict` | `rewrite`（`Recognized`→`mirror` 若未解析） | 首版若源存在且已解析则重写，否则镜像 |
| `system/*` 其它（`mapFieldsDict`、`sampleDict`、`streamlines`、`snappyHexMeshDict`…） | `mirror` | 与导出无关但必须保留 |
| `0/p`、`0/U` | `rewrite` | 可编辑 |
| `0/<其它场>` | `rewrite`（保真） | 只读内容逐字回写 |
| `0/<Unsupported>` | `mirror` | 不解析的原文件 |
| `constant/transportProperties` | `rewrite` | `nu` 可编辑 |
| `constant/turbulenceProperties` | `rewrite`（只读保真） | — |
| `constant/polyMesh/**` | `mirror` | 只识别不解析 |
| `constant/` 其它 | `mirror` | — |
| `Allrun`/`Allclean`/`All*`、`*.foam`、`*.log`、`postProcessing/`、`processor*/` | `skip`（默认） | 运行时产物；`Allrun` 若用户勾选“保留脚本”则 `mirror` |
| 其它时间目录 | `skip`（默认，可勾选 `mirror`） | 首版不做时间序列 |

镜像规则：逐字节复制；**不**跟随符号链接（§4.1）；`.gz` 保持 `.gz`（不解压）。镜像为**默认开启**（D-023）：`skip` 只用于运行时产物（`Allrun`/`Allclean`、`*.foam`、`*.log`、`postProcessing/`、`processor*/`、其它时间目录），用户可在导出预览中改为 `mirror`。

> 实现进度（t18，2026-10-28）：整个动作表已实现。`ExportPlanBuilder` 先用 writer 生成 7 个模型驱动文件（`Written`），再由 `CaseMirrorPlanner` 按 `SourceCaseWalker` 的清单分类：起始目录下的其它场与 `constant/turbulenceProperties`、`system/decomposeParDict` → `Copied`（保真复制）；`constant/polyMesh/**` 及其它未接管文件 → `Mirrored`；`ExportSkipRules` 命中的运行时产物与其它时间目录 → `skipped`。`CaseMirror` 逐字节复制（`.gz` 保持 `.gz`，不跟随符号链接），`AtomicCaseDirectory` 在临时目录内先写字典、再镜像、最后写导出清单，然后目录级 rename。清单由 `ExportManifest` 生成（`schemaVersion`/`generatedAt`/`counts`/`files[]`/`skipped[]`）。
> 未接管但无法镜像的情况（符号链接、超 `SourceCaseWalker::kMaxFiles`）记 `IMPORT-EXP-010`（Warning）：导出继续，但清单里不含这些文件，用户可见其被跳过。

### 9.3 `0/` vs `0.orig/` 决策（**已决策：D-018**，2026-10-27）

- 导入时记录 `startDirectory`（源实际使用的目录名）。
- 导出时**继承**源目录名：源为 `0/` → 导出 `0/`；源为 `0.orig/` → 导出 `0.orig/`。
- 新建工程（无源 case）走既有 `docs/ARCHITECTURE.md` §7.2 的 `0.orig/` 约定。
- 理由：v2206 的 `Allrun` 脚本普遍假定 `0.orig` 存在并 `cp -r 0.orig 0`；把 `0/` 强行改名为 `0.orig/` 会破坏用户既有工作流。此决策与既有 §7.2 的差异必须在 `docs/ARCHITECTURE.md` 更新时写明“导入场景例外”。

### 9.4 写出格式规范与等价判定

格式硬约束（`AGENTS.md` §5：LF、ASCII 优先、原子替换）：

| 项 | 规则 |
| --- | --- |
| 编码/换行 | UTF-8（纯 ASCII 内容时不写 BOM，不写 UTF-8 BOM）；`\n` |
| banner | 与源文件**同形**：若源有 banner 注释，沿用其文本；若无，写标准 `/*--------------------------------*- C++ -*----------------------------------*\` 块（逐字取自 `cavity/0/p:1-7`） |
| `FoamFile` | 固定顺序 `version`、`format`、`class`、`object`；`format` 恒 `ascii`；缩进 4 空格；`}` 单独一行 |
| entry 缩进 | 每层 4 空格；值列 = `缩进 + 16`（`FoamFile` 块固定第 16 列）：`dimensions      [0 2 -2 0 0 0 0];`、`        type            zeroGradient;`；`keyword` 长度抵到列位时退化为单个空格（`    nNonOrthogonalCorrectors 0;`）；`blockMeshDict` 的 `scale` 用第 8 列，patch 内的 `type`/`faces` 用单空格 |
| banner 宽度 | banner、`// * * *` 分隔行、`// *****` 尾行均为 **79** 字符定宽（逐字取自 `cavity`，黄金测试断言行长） |
| 数值格式 | 整数无小数点；浮点用 `std::to_chars` 最短往返表示（`1e-06` 而非 `1.00000e-06`）；向量 `(x y z)` 单空格分隔 |
| 尾行 | 文件末尾保留 `// ************************************************************************* //`（若源有） |
| 结尾换行 | 必须有且仅有一个 `\n` |
| 字典键顺序 | 已知键按 v2206 教程顺序（与附录 A 一致），未知键按源出现顺序追加在后 |
| 空行 | 条目之间 1 个空行；顶层 entry 之间 1 个空行（与 cavity 一致） |

**等价判定**（黄金文件测试用，宽松但可判定）：

1. **逐字等价**：`0/p`、`0/U` 在“未做任何编辑”的导入→导出后与源**逐字节相同**（本条是硬指标，用于锁定格式器质量）。
2. **结构等价**：其余字典按 AST 比较 —— 节点类型、keyword 序列（同序）、每个值的规范化值（数值按 `double` 比较，`abs/rel` 1e-12；字符串去首尾空白后逐字比较；列表逐元素）。注释、空行、缩进、对齐不参与比较。
3. **语义等价**：`fvSolution` 中 `$p;` 展开后的条目集合与源展开结果一致。

**t7 实测结论（`tests/io/CaseWriterGoldenTest.cpp`）**：`0/p`、`0/U`、`system/blockMeshDict`、`system/controlDict`、`system/fvSchemes`、`constant/transportProperties` 六个文件对 `tests/fixtures/foam/cavity/` 快照**逐字节相同**，达成第 1 条硬指标；`system/fvSolution` 因 `$p;` 展开而与源不同，期望文本存于 `tests/fixtures/golden/cavity/system/fvSolution`（第 1119 行“黄金文件”条的目的地）。展开规则：被引用字典的条目先写、跳过合并字典自己重定义的键，再写合并字典自己的条目（`pFinal` 因此得到 `solver`/`preconditioner`/`tolerance` + 自己的 `relTol`）。

**`$p;` 展开的实现位置**：`FvSolutionWriter` 打开 `FoamEmitOptions::expandVariableMerges`，`FoamDictionaryEmitter` 在遇到无 keyword 的 `Variable` 条目时按“先同级、后父级”查找同名子字典（OpenFOAM 的变量解析顺序）。

**禁止**：为了让“逐字等价”通过而写“源文件回拷”的捷径（即 `rewrite` 动作直接复制源文件）。`0/p` 的逐字等价必须由 `FieldFileWriter` 真正生成并通过；若首版做不到，必须在实现任务的交接摘要中显式降低为“结构等价”，并在本文档更新该条（评审关注点）。

### 9.5 写入安全（强制）

1. **目标目录保护（D-022）**：目标不得等于源 case 根，也不得**位于源 case 内**（任意层子目录）；否则 `IMPORT-EXP-001`（Blocking）。
2. **允许的目标形态（D-022）**：不存在的新目录，或已存在的空目录 → 直接可用。目标已存在且非空 → 要求用户显式确认（`ExportPreviewDialog` 勾选，默认不勾选），否则 `IMPORT-EXP-002`（Blocking）；确认后先改名为 `<target>.bak-<ts>` 再原子替换。
3. 全部文件先写入 `<target>.tmp-<seq>-<stamp>/`，逐文件写入后 `flush` 并检查流状态（t18 实现：`std::ofstream` 关闭后检查状态位；首版未调用 `FlushFileBuffers`，原子性由最后的目录级 rename 保证）。
4. 全部成功 → 目录级原子替换：若旧目标存在则先改名为 `<target>.bak-<stamp>`，再 rename 临时目录为 `<target>`；失败则回滚并删除临时目录（`docs/DECISIONS.md` D-033）。
5. 任一 `BlockingError`/`Error` → 不进入写入阶段（校验前置）。
6. 导出清单 `.ofgport-export.json` 只含相对路径、动作、字节数与 `schemaVersion`。t18 实现的动作拼写为 `written`（writer 生成）、`copied`（保真复制）、`mirrored`（镜像）；`skip` 的文件不进 `files[]`，而是在独立的 `skipped[]`（`relativePath` + `reason`）里列出，理由取 `runtime-artifact`/`other-time-directory`/`symbolic-link`/`unreadable`。清单在临时目录内**最后**写入，且不列自身。

### 9.6 黄金文件样例引用

| 黄金样例 | 源（v2206 教程相对路径） | 用途 |
| --- | --- | --- |
| G1 | `tutorials/incompressible/icoFoam/cavity/cavity/` | 正向导入 + 逐字等价（`0/p`、`0/U`） |
| G2 | 仅 `system/*` 与 `constant/*`（G1 的子集） | 结构等价（`controlDict`、`fvSchemes`、`fvSolution`、`transportProperties`、`decomposeParDict`、`blockMeshDict`） |
| G3 | `tutorials/incompressible/simpleFoam/pitzDaily/` | 负例：多块阻断；`turbulenceProperties` 只读解析 |
| G4 | `tutorials/incompressible/simpleFoam/pipeCyclic/0.orig/U` | 负例：`#include` 未展开 → `Partial` |
| G5 | `tutorials/DNS/dnsFoam/boxTurb16/0.orig/U` | 保真：`nonuniform List<vector>` 4096 值逐字回写 |

测试 fixture 组织：**必须**把上述文件复制进 `tests/fixtures/case/...`（快照），测试**禁止**在运行时读取 `OpenFOAM-v2206/`（该目录不进 Git，D-014；CI 环境可能不存在）。fixture 目录保留 `README.md` 记录来源相对路径与抓取版本 `v2206`。

### 9.7 导出前的显式提示

导出预览必须列出：`Recognized`/`Unsupported` 且将 `mirror` 的文件数与清单条目（D-023）、`Partial` 文件清单、`ReadOnly` 场清单、`skip` 的运行时产物清单。任一清单非空时“导出”按钮旁显示“本次导出不会修改 N 个文件”。镜像清单按目录折叠（`constant/polyMesh/` 折叠为一行 + 文件数），避免长列表淹没关键信息。

---

## 10. 文件组织规范（强制）

> 用户硬性要求：**每个子功能/类必须单独放在自己的 `.h`/`.cpp` 文件中，禁止把多个职责塞进一个文件**，便于后续定位与修改。
>
> **登记册分工（t25 明确）**：§10.2–§10.6 的表格是 t1 设计时写下的**计划稿**，只用于解释“为什么这样拆职责”；**当前实际文件清单的权威登记册是 `docs/ARCHITECTURE.md` §2.1/§5.x 与各模块 `CMakeLists.txt`**（`src/core/CMakeLists.txt`、`src/io/CMakeLists.txt`、根 `CMakeLists.txt`），评审与实现以那里为准。计划 → 实际的偏差摘要见 §10.2.1。§10.1 与 §10.7–§10.9 的**规则**仍然强制。

### 10.1 总则

1. **一个文件一个主类**：文件名 = 主类名（PascalCase）。例外：纯值类型/枚举可只有一个 `.h`（无实现时禁止造空 `.cpp`）。
2. `.h` 与 `.cpp` 同名同目录、成对出现（类有实现就必须有 `.cpp`）。
3. 类声明顺序：`#pragma once` → 标准库 → 项目头 → 前置声明 → `namespace ofgui::<layer>`。
4. **禁止杂物桶**：不得出现 `Utils.h/.cpp`、`Common.h`、`Helpers.h`、`Types.h` 这类多职责文件；纯函数集合命名为 `<Domain>Algorithms.h/.cpp`（例：`FaceAssignmentAlgorithms`）。
5. **禁止在一个 `.cpp` 里定义第二个对外可见的类**；`namespace {}` 内的文件局部辅助可以放在主类 `.cpp` 中，但不超过 60 行，且不得是另一职责。
6. 单文件软上限 400 行；超过则拆分（拆分方案先写进本文档 §10 清单再动手）。
7. 每个公开类型必须有对应测试文件（§10.6）；无测试的公开类型视为未完成。
8. 头文件不得 `#include` 具体 UI 类型（`QWidget` 等）到 `core`/`io` 中；`core`/`io` 头文件不得出现 `#include <Q...>`（`io/ProjectJson*` 例外，允许 `QJson*`）。
9. **诊断文本不本地化于 core/io**：`core`/`io` 的中文诊断文案是数据（UTF-8 `std::string`），不使用 `tr()`/`QString`，不得为文案新增 Qt 依赖；本地化在 UI 层按 `ValidationIssue::code` 映射（D-039、§8.7）。

### 10.2 `src/core/`（新建静态库 `ofgui_core`；**历史计划**）

> **本表是 t1 计划稿，不是当前源码清单。** 实际登记册：`docs/ARCHITECTURE.md` §5.1 与 `src/core/CMakeLists.txt`——截至 2026-10-28 共 42 个源/头文件（`Basics.h`、`Vec3d.h`、`Version.h/.cpp`、`ObjectId.h/.cpp`、`SourceLocation.h/.cpp`、`ValidationIssue.*`、`ValidationResult.*`、`FaceId.h`、`DomainBox.*`、`MeshSettings.*`、`BoundaryPatch.*`、`FaceAssignmentValidator.*`、`FieldValue.*`、`BoundaryCondition.*`、`FieldDefinition.*`、`CaseProject.*`、`CaseSourceInfo.*`、`ProjectSchemaVersion.h`、`SolverSettings.*`、`CaseValidator.h/.cpp` + 5 个规则族 `.cpp` + `CaseValidatorMessages.h`），另加 `CMakeLists.txt`。计划与实际的偏差见 §10.2.1。

| 文件 | 职责 | 关键类型 | 首个消费方 |
| --- | --- | --- | --- |
| `src/core/Basics.h` | 基础别名与容错开关（**唯一**允许的“集合型”头） | `Real`、`Index` | 全部 |
| `src/core/Vec3d.h` | 三维向量值类型（仅头文件） | `Vec3d` | `DomainBox` |
| `src/core/Version.h` / `.cpp` | 模块骨架锚点（模块名 + 语义版本，不含业务） | `ModuleVersion` | 构建自检、UI 关于页 |
| `src/core/ObjectId.h` | 稳定 ID 生成与比较（仅头文件） | `ObjectId`、`makeObjectId()` | `CaseProject` |
| `src/core/SourceLocation.h` | 源位置值类型（§6.6） | `SourceLocation` | `ValidationIssue` |
| `src/core/ValidationIssue.h` / `.cpp` | 单条校验问题 + 格式化 | `ValidationIssue`、`Severity` | `ValidationResult` |
| `src/core/ValidationResult.h` / `.cpp` | 问题集合、按级别统计、阻断判定 | `ValidationResult` | `CaseValidator`、UI |
| `src/core/FaceId.h` / `.cpp` | 六个面枚举与索引（**从 `src/render/FaceId.h` 迁入**，render 保留 `using` 别名） | `FaceId`、`kAllFaceIds` | `BoundaryPatch` |
| `src/core/DomainBox.h` / `.cpp` | 计算域包围盒与不变量 | `DomainBox`、`DomainMode` | `CaseProject` |
| `src/core/MeshSettings.h` / `.cpp` | 网格参数（cells/grading/scale） | `MeshSettings` | `MeshProjector` |
| `src/core/BoundaryPatch.h` / `.cpp` | 单个边界面 | `BoundaryPatch` | `CaseProject` |
| `src/core/FaceAssignmentValidator.h` / `.cpp` | 面归属唯一性与覆盖性校验 | `FaceAssignmentValidator` | `CaseValidator` |
| `src/core/FieldValue.h` / `.cpp` | 场值（uniform/nonuniform/变量） | `FieldValue`、`FieldValueKind` | `FieldDefinition` |
| `src/core/BoundaryCondition.h` / `.cpp` | 单 patch 的 BC | `BoundaryCondition` | `FieldDefinition` |
| `src/core/FieldDefinition.h` / `.cpp` | 一个场文件（含 `boundaryField`） | `FieldDefinition` | `CaseProject` |
| `src/core/TransportModelSettings.h` / `.cpp` | `transportProperties` 投影结果 | `TransportModelSettings` | `CaseProject` |
| `src/core/TurbulenceSettings.h` / `.cpp` | `turbulenceProperties` 投影结果 | `TurbulenceSettings` | `CaseProject` |
| `src/core/ControlDictSettings.h` / `.cpp` | `controlDict` 投影结果（含未接管子树） | `ControlDictSettings` | `CaseProject` |
| `src/core/FvSchemesSettings.h` / `.cpp` | 6 类 scheme | `FvSchemesSettings`、`SchemeCategory` | `CaseProject` |
| `src/core/FvSolutionSettings.h` / `.cpp` | solvers + 算法字典 | `FvSolutionSettings` | `CaseProject` |
| `src/core/DecomposeSettings.h` / `.cpp` | `decomposeParDict` 投影结果 | `DecomposeSettings` | `CaseProject` |
| `src/core/CaseProject.h` / `.cpp` | 聚合根（域、网格、patch、场、求解设置、源信息） | `CaseProject` | UI、exporter |
| `src/core/CaseSourceInfo.h` / `.cpp` | 源 case 元信息（`sourceRelativePath`、`startDirectory`、`importedAt`、逐文件 `sha256`，D-025） | `CaseSourceInfo` | `CaseProject` |
| `src/core/ProjectSchemaVersion.h` | 工程文件版本常量与迁移表声明 | `kCurrentSchemaVersion` | `ProjectJson*` |
| `src/core/ImportOptions.h` | 导入选项 | `ImportOptions` | `CaseImportService` |
| `src/core/ImportedFile.h` / `.cpp` | 单文件识别结果（§6.7） | `ImportedFile`、`SupportLevel` | `ImportReport` |
| `src/core/ImportReport.h` / `.cpp` | 导入报告 | `ImportReport` | UI |
| `src/core/CaseValidator.h` / `.cpp` | 模型级校验（不碰文件）：`validate()` 编排 + `CaseValidatorDomain/Mesh/Patches/Fields/Solver.cpp` 五个规则族实现 + `CaseValidatorMessages.h` 共享消息/格式化辅助（D-021 拆分，§10.9） | `CaseValidator` | UI、exporter |
| `src/core/CaseCrossValidator.h` / `.cpp` | 跨字典一致性（patch 名单、BC 覆盖、场维度） | `CaseCrossValidator` | `CaseImportService` |

`src/core/foam/`（字典语法树，与文件系统无关）：

| 文件 | 职责 |
| --- | --- |
| `src/core/foam/FoamSyntaxKind.h` | token/节点种类枚举（对齐 v2206 `token` 枚举） |
| `src/core/foam/FoamNode.h` / `.cpp` | 节点基类（位置、父子、克隆） |
| `src/core/foam/FoamEntry.h` / `.cpp` | keyword + 值 + 位置 |
| `src/core/foam/FoamDictionaryNode.h` / `.cpp` | 具名/匿名字典与查找（literal，含递归） |
| `src/core/foam/FoamValueNode.h` / `.cpp` | 标量/词/字符串统一值节点 |
| `src/core/foam/FoamVectorNode.h` / `.cpp` | `(x y z)` 向量 |
| `src/core/foam/FoamListNode.h` / `.cpp` | `( … )` 列表（可嵌套） |
| `src/core/foam/FoamDimensionSetNode.h` / `.cpp` | `[7 个指数]` |
| `src/core/foam/FoamVariableRef.h` / `.cpp` | `$name` 引用与作用域种类 |
| `src/core/foam/FoamVerbatimNode.h` / `.cpp` | `#{ … #}` |
| `src/core/foam/FoamDirectiveNode.h` / `.cpp` | `#include` 等 |
| `src/core/foam/FoamHeader.h` / `.cpp` | `FoamFile` 头模型 |
| `src/core/foam/FoamPath.h` / `.cpp` | 字典路径解析/格式化（`a.b.c`） |
| `src/core/foam/FoamScopeResolver.h` / `.cpp` | `$` 局部/父/绝对查找（§5.4） |
| `src/core/foam/FoamNodeVisitor.h` / `.cpp` | 访问者基类（避免在节点里塞业务） |

`src/core/import/`（投影器，每个字典一个文件）：

| 文件 | 职责 |
| --- | --- |
| `BlockMeshProjector.h` / `.cpp` | `blockMeshDict` → `DomainBox`/`MeshSettings`/`BoundaryPatch` |
| `FieldProjector.h` / `.cpp` | 一个场字典 → `FieldDefinition` |
| `BoundaryFieldProjector.h` / `.cpp` | `boundaryField` → `BoundaryCondition` 列表 |
| `BoundaryConditionFactory.h` / `.cpp` | `type` → BC 类型与支持级别（§4.6 白名单） |
| `TransportPropertiesProjector.h` / `.cpp` | → `TransportModelSettings` |
| `TurbulencePropertiesProjector.h` / `.cpp` | → `TurbulenceSettings` |
| `ControlDictProjector.h` / `.cpp` | → `ControlDictSettings` |
| `FvSchemesProjector.h` / `.cpp` | → `FvSchemesSettings` |
| `FvSolutionProjector.h` / `.cpp` | → `FvSolutionSettings` |
| `DecomposeParProjector.h` / `.cpp` | → `DecomposeSettings` |
| `PatchNameResolver.h` / `.cpp` | patch 名单优先级（§6.3） |
| `CaseImportService.h` / `.cpp` | 编排：扫描 → 解析 → 投影 → 报告（唯一“编排者”） |

#### 10.2.1 计划 → 实际偏差摘要（t25，2026-10-28 按源码实测）

| 计划（§10.2–§10.4） | 实际落地 | 说明 |
| --- | --- | --- |
| `core/foam/*`（token/AST/节点 16 个文件） | `src/io/`：`FoamLexer*`、`FoamParser*`、`FoamNode`、`FoamValue`、`FoamToken`、`FoamDictionary`、`FoamDiagnostic` | 词法/语法树要读文件、带行列诊断，整体落在 io；`core` 保持零文件 IO、零 Qt |
| `core/import/*`（12 个投影器 + 编排者） | `src/io/`：`CaseScanner`(+`CaseDirectoryProbe`、`CaseNameRules`)、`CaseImporter`(+`ImportDictionaryLoader`、`ImportReportBookkeeping`)、`BlockMeshProjector`、`BlockMeshPatchProjector`、`BlockMeshValueReader`、`FieldProjector`、`BoundaryConditionFactory`、`ControlDictProjector`、`TransportPropertiesProjector`、`ReadOnlyDictionaryProjector`、`CaseFieldReader`/`CaseFieldApplier`、`CaseDirectoryScan` | 投影器必须读字典文件，属 io 职责；编排者命名 `CaseImporter`（不是 `CaseImportService`）；子功能文件按域命名，无 `Helpers`/`Support` 杂物桶（D-041） |
| `core/ImportOptions.h`、`ImportedFile.h`、`ImportReport.h`、`CaseCrossValidator.*` | `src/io/CaseImportOptions.h`、`CaseSupportLevel.*`、`CaseImportResult.h`、`ImportReport`、`ImportIssueFactory`/`ImportCodes.h`、`CaseSourceRecorder.*`；跨字典一致性并入 `CaseValidator` 规则族（`CaseValidatorMesh/Patches/Fields.cpp`） | “文件识别结果”随 io 走，避免 core 依赖文件系统概念 |
| `core/TransportModelSettings`、`TurbulenceSettings`、`ControlDictSettings`、`FvSchemesSettings`、`FvSolutionSettings`、`DecomposeSettings`（6 个独立类） | 未落地为独立类：`controlDict`/`fvSchemes`/`fvSolution` 的编辑由 `core/SolverSettings` 承接；`transportProperties` 编辑值只存在于会话内（D-040）；`turbulenceProperties`/`decomposeParDict` 只读镜像 | 首版只结构化编辑“能安全回写”的子集 |
| `core/FaceId.h` / `.cpp` | `src/core/FaceId.h`（纯头文件，无 `.cpp`） | 枚举 + `constexpr`，无实现 |
| `io/TextFileReader`、`FoamKeywords`、`FoamWriterOptions.h`、`FoamDictionaryWriter` | `FoamDictionaryFileReader`、`FoamDictionaryLookup`、`FoamDictionaryEmitter`、`FoamLayout`、`FoamNumberFormat`、`FoamValueText.h`、`FoamLexerChars.h`、`FoamParserTokens.h`、`FoamValueConversions.cpp` | 统一 `Foam*` 前缀；读、查、写、格式化各自成文件 |
| `io/CaseDirectoryScanner`、`ExportPlan`、`CaseMirror`、`AtomicDirectoryWriter`、`PathSafety`、`OpenFoamCaseExporter`、`ExportManifest` | `src/io/CaseScanner`（顶层）；导出设施全部收进 `src/io/writers/`（42 个文件）：`CaseExportPlan`、`ExportPlanBuilder`、`ExportAction`、`CaseMirror`+`CaseMirrorPlanner`、`AtomicCaseDirectory`、`ExportSkipRules`、`SourceCaseWalker`、`CaseExporter`、`ExportManifest`+`ExportManifestSchema.h` | 计划/镜像/原子替换/清单在 `writers/` 内按子功能一文件 |
| `io/TurbulencePropertiesWriter`、`DecomposeParDictWriter` | **未实现**（这两类字典只读镜像、不重写） | D-020 / D-040 |
| `io/ProjectJsonReader` / `Writer` | `ProjectJsonRead*.cpp`、`ProjectJsonWrite.cpp`、`ProjectJsonRepository`、`ProjectJsonMapping.h`、`ProjectJsonValueReader`、`ProjectJsonSource`、`ProjectJsonSchema.h`、`ProjectJsonDiagnostic` | 读/写/映射/诊断/来源各成文件 |
| `ui/CaseImportDialog`、`CaseImportReportPanel`、`IssueListView`、`CaseOverviewPanel`、`TrDimensional`、`DomainInspector`、`MeshInspector`、`BoundaryPatchInspector`、`FieldInspector`、`SolverInspector` | `InspectorPanel` + `DomainEditor`、`FieldEditor`(+`Fields`/`Groups`)、`FieldBoundaryTable`(+`Rows`/`TypeRules`)、`SolverEditor`(+`Refresh`/`Writes`/`Accessors`)、`ValidationPanel`、`ValidationNavigator`、`ExportController`、`ExportPreviewDialog`(+`Preview`)、`DefaultCaseTemplate`；导入无向导（菜单项直接导入，报告以工程节点摘要 + 状态栏计数呈现） | 导入向导与独立报告页在 Backlog（§10.4 偏差注、`docs/TASKS.md`） |
| `app/CaseImportCommand` | **未实现**：命令行导入未做，导入链路由 `tests/io` 的集成测试覆盖 | 首版无命令行导入入口 |

### 10.3 `src/io/`（新建静态库 `ofgui_io`；**历史计划**）

> 实际登记册：`src/io/CMakeLists.txt` 的 `ofgui_io` 源列表（顶层 92 个文件）+ `src/io/writers/`（42 个文件）；偏差见 §10.2.1。

| 文件 | 职责 |
| --- | --- |
| `TextFileReader.h` / `.cpp` | 字节读取、编码判定、BOM 剥离、大小上限 |
| `FoamLexer.h` / `.cpp` | 词法（§5.1–5.3） |
| `FoamParser.h` / `.cpp` | 语法 → `core::foam`（含前向恢复） |
| `FoamKeywords.h` / `.cpp` | 关键字表（`FoamFile`、`dimensions`、`internalField`、`boundaryField`、`type`…） |
| `FoamWriterOptions.h` | 写出选项（缩进、banner、换行） |
| `FoamDictionaryWriter.h` / `.cpp` | 通用字典发射器（唯一负责缩进/对齐/转义） |
| `BlockMeshDictWriter.h` / `.cpp` | `system/blockMeshDict` |
| `FieldFileWriter.h` / `.cpp` | `0/p`、`0/U`、只读场（保真回写） |
| `ControlDictWriter.h` / `.cpp` | `system/controlDict` |
| `FvSchemesWriter.h` / `.cpp` | `system/fvSchemes` |
| `FvSolutionWriter.h` / `.cpp` | `system/fvSolution` |
| `TransportPropertiesWriter.h` / `.cpp` | `constant/transportProperties` |
| `TurbulencePropertiesWriter.h` / `.cpp` | `constant/turbulenceProperties` |
| `DecomposeParDictWriter.h` / `.cpp` | `system/decomposeParDict` |
| `CaseDirectoryScanner.h` / `.cpp` | 目录扫描（时间目录识别、链接跳过、polyMesh 探测） |
| `ExportPlan.h` / `.cpp` | 文件动作规划（rewrite/mirror/skip，§9.2） |
| `CaseMirror.h` / `.cpp` | 逐字节镜像（含 `.gz`） |
| `AtomicDirectoryWriter.h` / `.cpp` | 临时目录 + 原子替换 + 回滚（§9.5） |
| `PathSafety.h` / `.cpp` | 目标/源路径关系校验 |
| `OpenFoamCaseExporter.h` / `.cpp` | 导出编排（唯一“编排者”） |
| `ExportManifest.h` / `.cpp` | `.ofgport-export.json` 读写 |
| `ProjectJsonReader.h` / `.cpp` | `.ofgproj` 读取（Qt6::Core 的 `QJson*`） |
| `ProjectJsonWriter.h` / `.cpp` | `.ofgproj` 写出 + 原子替换 |
| `Version.h` / `.cpp` | 模块骨架锚点（复用 `core::formatModuleVersion`，自检 io→core 链接边） |

**禁止**：writer 里拼 UI 文案；writer 不得读模型以外的文件（镜像除外，且只经 `CaseMirror`）；lexer/parser 不得依赖 `core/import`。

### 10.4 `src/ui/`（扩展现有静态库 `ofgui_ui`；**历史计划**）

> 实际登记册：根 `CMakeLists.txt` 的 `ofgui_ui` 源列表 + `docs/ARCHITECTURE.md` §5.3–§5.6；偏差见 §10.2.1 与本小节末尾的“实现偏差”注。

| 新增文件 | 职责 | 依赖面板 |
| --- | --- | --- |
| `CaseImportDialog.h` / `.cpp` | 导入向导（源选择 + 选项） | `CaseImportReportPanel` |
| `CaseImportReportPanel.h` / `.cpp` | 导入报告页 | `IssueListView` |
| `IssueListView.h` / `.cpp` | 可复用问题列表（分级/定位/跳转信号） | — |
| `CaseOverviewPanel.h` / `.cpp` | 算例总览（源信息、起始目录、文件统计） | — |
| `DomainInspector.h` / `.cpp` | 计算域编辑（§7.3 上半） | — |
| `MeshInspector.h` / `.cpp` | 网格参数编辑（§7.3 下半） | — |
| `BoundaryPatchInspector.h` / `.cpp` | patch 名称/类型/面勾选（§7.4） | `FaceAssignmentValidator` |
| `FieldInspector.h` / `.cpp` | 场与 BC 表（§7.5） | `BoundaryCondition` |
| `SolverInspector.h` / `.cpp` | 求解设置分组（§7.6） | — |
| `ValidationPanel.h` / `.cpp` | 校验面板（§7.7） | `IssueListView` |
| `ExportPreviewDialog.h` / `.cpp` | 导出预览（§7.8） | `ExportPlan` |
| `TrDimensional.h` / `.cpp` | 维度指数 ↔ 中文单位提示的展示助手（唯一放这类文案的地方） | — |

既有文件修改（**只改装配，不搬职责**）：`MainWindow`（菜单/动作：导入、校验、导出）、`ConfigurationPanel`（切换 Inspector 的面板栈）、`ProjectTreeWidget`（树节点增加“源文件/级别”列）。三者不得承载解析或文件写入逻辑。

> 实现偏差（t13 记录，2026-10-28）：t8–t12 实际落地为 `InspectorPanel` + `DomainEditor`/`FieldEditor`/`FieldBoundaryTable`/`SolverEditor` 堆叠页，`CaseOverviewPanel` 职责并入工程节点详情；`CaseImportDialog`、`CaseImportReportPanel`、`IssueListView`、`TrDimensional` 未实现——导入报告以工程节点“导入结果”摘要 + 状态栏计数呈现，`ValidationPanel` 单独承载模型校验问题。当前文件清单以 `docs/ARCHITECTURE.md` §5.3–§5.6 与根 `CMakeLists.txt` 为准；导入向导/独立报告页列入 Backlog（`docs/TASKS.md`）。

### 10.5 `src/render/` 与 `src/app/`

- `src/render/FaceId.h` 迁往 `core`，render 侧保留 `namespace ofgui::render { using core::FaceId; using core::kAllFaceIds; }` 的兼容头（`FaceIdCompat.h`），避免一次性改 12 个文件；**迁移任务必须同步删除重复定义**，不得两份并存定义。
- `src/render/` 首版**不新增**文件；若 `DomainSceneBuilder` 需要“只显示某 patch 的面”，加 `DomainSceneFilters.h/.cpp`（单独文件，不塞进 builder）。
- `src/app/` 新增 `CaseImportCommand.h/.cpp`（命令行 smoke：`--import-case <dir> --report <file>`）与既有 `SmokeTestController` 的扩展点；不得把解析逻辑写进 `main.cpp`。

### 10.6 `tests/`（**历史计划**）

> 实际登记册：`tests/core`（17 个）与 `tests/io`（19 个）由各自模块 `CMakeLists.txt` 的 `add_test` 登记；`tests/render`（2 个）与 `tests/ui`（5 个：`CaseDocumentTreeTest`、`DomainEditorTest`、`ExportWorkflowTest`、`FieldEditorTest`、`SolverEditorTest`）由根 `CMakeLists.txt` 登记。下表是 t1 计划稿。

| 目录/文件 | 覆盖 |
| --- | --- |
| `tests/core/DomainBoxTest.cpp` | 包围盒归一化、非长方体检测、2D 建议 |
| `tests/core/Vec3dTest.cpp`、`tests/core/VersionTest.cpp`、`tests/io/VersionTest.cpp` | 值类型语义与模块边界骨架 |
| `tests/core/MeshSettingsTest.cpp` | cells/grading/scale 不变量 |
| `tests/core/BoundaryPatchTest.cpp` | patch 名校验、面集合 |
| `tests/core/FaceAssignmentValidatorTest.cpp` | 重叠/未覆盖/空 patch |
| `tests/core/FieldValueTest.cpp` | uniform/nonuniform/变量/原文保真 |
| `tests/core/CaseValidatorTest.cpp`、`tests/core/CaseCrossValidatorTest.cpp` | 模型校验与跨字典一致性 |
| `tests/core/foam/FoamDictionaryNodeTest.cpp`、`FoamScopeResolverTest.cpp`、`FoamPathTest.cpp` | AST 查找与 `$` 作用域 |
| `tests/io/FoamLexerTest.cpp`、`FoamParserTest.cpp` | 词法/语法表驱动 + 错误注入（§5.3 每种场景一个用例） |
| `tests/io/CaseDirectoryScannerTest.cpp` | 目录识别、链接跳过、polyMesh 探测 |
| `tests/io/BlockMeshProjectorTest.cpp`、`FieldProjectorTest.cpp`、`BoundaryFieldProjectorTest.cpp`、`SolverProjectorsTest.cpp`（**按投影器拆文件**） | 投影正确性 |
| `tests/io/BlockMeshDictWriterTest.cpp`、`FieldFileWriterTest.cpp`、`SolverWritersTest.cpp`（**按 writer 拆文件**） | 写出文本 |
| `tests/io/GoldenCaseImportExportTest.cpp` | G1–G5 全链路（逐字/结构等价） |
| `tests/io/AtomicDirectoryWriterTest.cpp` | 临时目录、回滚、目标非空保护 |
| `tests/ui/CaseImportFlowSmokeTest.cpp` | 向导 → 报告 → 主界面装配（不校验像素） |

规则：一个测试类一个文件；fixture 路径通过 `QFINDTESTDATA` 或编译期宏注入，**禁止**硬编码 `D:\…`。

### 10.7 CMake 登记规则（强制）

1. 新增 `src/core/**` 文件 → 加入 `ofgui_core` 的**显式**源列表；新增 `src/io/**` → `ofgui_io`；新增 `src/ui/**` → `ofgui_ui`。**禁止 `file(GLOB …)`**（`AGENTS.md` §5 的“收敛改动”与增量构建可预测性）。
2. 目标定义（照抄既有风格，含编译选项块）：

```cmake
add_library(ofgui_core STATIC
  src/core/CaseProject.cpp
  # … 每个 .cpp 一行，按 src/core/CMakeLists.txt 既有分组顺序追加
)
target_include_directories(ofgui_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_library(ofgui_io STATIC
  src/io/FoamLexer.cpp
  # …
)
target_include_directories(ofgui_io PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(ofgui_io PUBLIC ofgui_core Qt6::Core)

target_link_libraries(ofgui_ui PUBLIC ofgui_io ofgui_render Qt6::Widgets)
target_link_libraries(OpenFOAMGUI PRIVATE ofgui_ui)
```

3. `ofgui_core` **不得**链接 Qt（除 `Qt6::Core` 仅在确有需要时，且 `CaseProject` 等纯模型文件不得出现 Qt 头）；`ofgui_io` 可链 `Qt6::Core`，不得链 `Qt6::Widgets`/`Qt6::Gui`；`ofgui_ui` 才允许 Widgets（对应 `docs/ARCHITECTURE.md` §2）。
4. 每个新测试文件 → 一个独立可执行目标 + `add_test`（命名 `ofgui_<module>_<subject>_tests`），便于定位失败；测试目标同样复制编译选项块。
5. 新增文件后必须：①加入所属 `CMakeLists.txt` 的**显式**源列表；②在 `docs/ARCHITECTURE.md` §2.1/§5.x 对应小节登记职责。**清单的权威来源是 CMakeLists + ARCHITECTURE**；本文件 §10.2–§10.6 是 t1 计划稿（偏差见 §10.2.1），不作为逐文件比对依据。
6. 若新文件引入第三方依赖 → 先在 `docs/DECISIONS.md` 增条目（`AGENTS.md` §5 末尾要求）。

### 10.8 评审检查清单（文件组织）

| # | 检查项 | 不通过示例 |
| --- | --- | --- |
| 1 | 每个 `.cpp` 只定义一个对外类 | `FoamParser.cpp` 里同时定义 `FoamParser` 与 `FoamLexer` |
| 2 | 无 `Utils/Common/Helpers/Types` 杂物文件 | 新增 `src/io/Utils.cpp` |
| 3 | 文件名 = 主类名 | `BlockMesh.cpp` 里定义 `BlockMeshDictWriter` |
| 4 | 每个 writer/投影器/面板独立文件 | `CaseWriters.cpp` 里 6 个 writer |
| 5 | `.h`/`.cpp` 成对（有实现时） | 只有 `DomainBox.cpp` 没有声明来源 |
| 6 | 已在 CMake 显式登记 + `docs/ARCHITECTURE.md` §2.1/§5.x 登记职责 | 文件存在但未进 `add_library` |
| 7 | 有对应测试文件 | 新增 12 个 core 类型却只加 1 个测试 |
| 8 | `core`/`io` 头文件不出现 Widgets | `CaseProject.h` 里 `#include <QWidget>` |
| 9 | 单个 `.cpp` ≤ 400 行（超出需在 §10 记录拆分方案） | `CaseImportService.cpp` 900 行 |
| 10 | `core`/`io` 不含 `tr()`/`QString`，中文文案配稳定 `code`（D-039） | `CaseValidatorSolver.cpp` 里写 `tr("时间步长必须为正")` |

### 10.9 超 400 行文件的拆分方案（t13 记录；t19/t20 已全部执行）

D-021 第 4 条要求超出 400 行的文件先在本节记录拆分方案。2026-10-28 t13 独立核查发现下列 7 个文件超限（`ReadAllLines` 计数，含注释）。它们都是“一个对外类一个文件”，没有多类混装，但按规则必须拆分；拆分在后续任务执行，t14 评审复核。**状态（t25，2026-10-28）：t19（UI 6 个）与 t20（`CaseValidator`）已全部拆分完成；`src/` + `tests/` 全量复核无文件超过 400 行，最长 395 行（`tests/io/ProjectJsonRepositoryErrorsTest.cpp`）。**

| 文件 | 当前行数 | 拆分方案 |
| --- | --- | --- |
| `src/ui/MainWindow.cpp` | 633 | 动作创建 / 菜单 / 工具栏 → `MainWindowActions.cpp`；中央布局 / 校验 dock / 状态栏 → `MainWindowLayout.cpp`；打开 / 保存 / 关闭与 dirty 保护 → `MainWindowDocumentActions.cpp` |
| `src/ui/SolverEditor.cpp` | 601 | controlDict 页、constant 页、只读原文页、导出预览页各拆一个构造辅助类（`SolverControlDictPage`、`SolverTransportPage`、`SolverRawDictionaryPage`、`SolverPreviewPage`），`SolverEditor` 只做装配与信号 |
| `src/ui/FieldEditor.cpp` | 520 | dimensions / internalField / boundary / 预览四个分组各拆构造辅助类（`FieldDimensionsGroup`、`FieldInternalGroup`、`FieldBoundaryGroup`、`FieldPreviewGroup`） |
| `src/ui/FieldBoundaryTable.cpp` | 456 | 表格重建、行编辑提交、类型白名单/校验拆到 `FieldBoundaryRowBuilder` 与 `FieldBoundaryValidator` |
| `src/ui/CaseDocumentController.cpp` | 439 | 导入 / 工程文件读写 / transport 与只读设置访问拆三个实现文件（同一类，只搬实现） |
| `src/ui/ExportPreviewDialog.cpp` | 409 | 文件树填充、文本预览、目标与覆盖确认拆构造辅助类（`ExportPreviewTree`、`ExportPreviewText`、`ExportTargetControls`） |
| `src/core/CaseValidator.cpp` | 402 | 按已有 `validateDomain` / `validateMesh` / `validatePatches` / `validateFields` / `validateSolver` 方法组拆为 5 个实现文件（同一类，只搬实现） |

本节在拆分完成前是技术债清单；现由 t19/t20 全部关闭，表格保留作拆分前后对照。后续新增代码若再超 400 行，按 D-021 第 4 条先在本节补记拆分方案再动手。

#### 10.9.1 拆分结果（t19 与 t20，2026-10-28）

UI 侧 6 个文件已由 t19 按“一个子功能一个实现文件、文件名以主类名开头、公共 API 与行为不变”完成拆分，全部 ≤400 行；新文件已显式登记根 `CMakeLists.txt` 的 `ofgui_ui` 源列表（无 GLOB）。`src/core/CaseValidator.cpp` 在 t20 拆为「20 行入口 `validate()` + 5 个规则族实现 + 1 个内部支持头」，规则、消息码与严重度逐字保留（下表最后一行；t25 实测行数见 §10.2 的 core 清单）。

| 原文件 | 拆分后文件（行数） |
| --- | --- |
| `MainWindow.cpp` 633 | `MainWindow.cpp` 169（构造 / 访问器 / 文档信号 / dirty 标题 / 帮助）<br>`MainWindowActions.cpp` 188（动作、菜单、工具栏、中央布局、校验 dock、状态栏）<br>`MainWindowDocumentActions.cpp` 216（新建 / 打开算例 / 打开工程 / 保存 / 另存为 / 关闭 / 未保存提示）<br>`MainWindowExportActions.cpp` 115（导出对话框、进度、可用性、默认目标目录） |
| `SolverEditor.cpp` 601 | `SolverEditor.cpp` 286（控件构造与编辑信号）<br>`SolverEditorRefresh.cpp` 175（读取模型 / 只读导入值 → 控件，不回写）<br>`SolverEditorWrites.cpp` 94（统一写回与脏标记）<br>`SolverEditorAccessors.cpp` 100（测试与截图 hook 用的控件访问器）<br>`SolverEditorPaths.h` 9（字典相对路径常量） |
| `FieldEditor.cpp` 520 | `FieldEditor.cpp` 332（生命周期、刷新、internalField 提交、预览、错误）<br>`FieldEditorGroups.cpp` 127（dimensions / internalField / boundary / preview 分组构造）<br>`FieldEditorFields.cpp` 97（新增 / 删除场） |
| `FieldBoundaryTable.cpp` 456 | `FieldBoundaryTable.cpp` 144（表格装配、写回、错误）<br>`FieldBoundaryTableRows.cpp` 218（行重建、选择、提交）<br>`FieldBoundaryTableTypeRules.cpp` 128（BC 类型规则：可编辑类型白名单、需值类型、uniform 值校验） |
| `CaseDocumentController.cpp` 439 | `CaseDocumentController.cpp` 158（状态、访问器、信号）<br>`CaseDocumentControllerImport.cpp` 90（导入算例）<br>`CaseDocumentControllerPersistence.cpp` 99（工程文件读写 / 另存为）<br>`CaseDocumentControllerDictionaries.cpp` 130（transport 与只读字典缓存）<br>`CaseDocumentControllerPaths.h` 16（case 内字典路径常量） |
| `ExportPreviewDialog.cpp` 409 | `ExportPreviewDialog.cpp` 231（控件构造、目标选择、覆盖确认）<br>`ExportPreviewDialogPreview.cpp` 192（计划树填充、文本预览、按钮状态） |
| `CaseValidator.cpp` 402（t20） | `CaseValidator.cpp` 20（入口 `validate()` 编排，顺序 DOM → MESH → PATCH → FIELD → SOLVER）<br>`CaseValidatorDomain.cpp` 40（`CASE-DOM-*`）<br>`CaseValidatorMesh.cpp` 80（`CASE-MESH-*`）<br>`CaseValidatorPatches.cpp` 145（`CASE-PATCH-*`）<br>`CaseValidatorFields.cpp` 76（`CASE-FIELD-*`）<br>`CaseValidatorSolver.cpp` 56（`CASE-SOLVER-*`）<br>`CaseValidatorMessages.h` 64（内部消息/格式化辅助，非公开 API） |

验证（t19）：Debug 构建 0 warning，`ctest --preset windows-mingw-debug` 43/43 通过（含 `ofgui_ui_smoke_tests`、`ofgui_ui_domain_tests`、`ofgui_ui_field_tests`、`ofgui_ui_solver_tests`、`ofgui_ui_workflow_tests`）。

验证（t20 + t25 复核）：t20 后全量 `Build-Project.ps1 -Configuration Debug` 退出码 0、0 warning、ctest 43/43；2026-10-28 t25 逐文件读行复核 `src/` + `tests/` 共 0 个文件超过 400 行（最长 395 行 `tests/io/ProjectJsonRepositoryErrorsTest.cpp`），拆分后各文件行数与上表一致。

#### 10.9.2 t23 评审整改（2026-10-28）

| 评审项 | 整改 |
| --- | --- |
| F-06（ui） | `FieldBoundaryTableTypes.cpp` 属 §10.1 禁止的 `*Types` 杂物桶，按域更名为 `FieldBoundaryTableTypeRules.cpp`，并更新根 `CMakeLists.txt`；无其它文件包含该实现，故无 include 变更 |
| F-12（测试目标） | 渲染场景测试目标按 §10.7 模块前缀规则由 `ofgui_scene_tests` 更名为 `ofgui_render_scene_tests`（`add_executable`/`vtk_module_autoinit`/`add_test` 同步），并更新本文档与 `docs/STATUS.md` 的引用 |

F-05（`transportModel`/`nu` 不写入 `.ofgproj`）按策略 B 处理：不实现持久化，改由 `SolverEditor` 的 constant 页提示与保存/重开状态栏说明显式告知用户；详见 `docs/ARCHITECTURE.md` §5.6。

---

## 11. 验收示例

### 11.1 正向：导入 `icoFoam/cavity/cavity`（G1）

输入：`OpenFOAM-v2206/tutorials/incompressible/icoFoam/cavity/cavity/`（fixture 快照）。

期望结果（必须逐项命中）：

| 项 | 期望 | 出处 |
| --- | --- | --- |
| 起始时间目录 | `0` | 目录实存 |
| 场文件 | `p`（`volScalarField`）、`U`（`volVectorField`），均为 `Editable` | `0/p:8-14`、`0/U:8-14` |
| `p.dimensions` | `[0 2 -2 0 0 0 0]` | `0/p:17` |
| `p.internalField` | `uniform 0` | `0/p:19` |
| `U.dimensions` | `[0 1 -1 0 0 0 0]` | `0/U:17` |
| `U.internalField` | `uniform (0 0 0)` | `0/U:19` |
| patch 名单 | `movingWall`、`fixedWalls`、`frontAndBack` | `system/blockMeshDict` 的 `boundary` |
| `p` 的 BC | `movingWall=zeroGradient`、`fixedWalls=zeroGradient`、`frontAndBack=empty` | `0/p:23-37` |
| `U` 的 BC | `movingWall=fixedValue value uniform (1 0 0)`、`fixedWalls=noSlip`、`frontAndBack=empty` | `0/U:23-38` |
| `DomainBox` | 顶点 × `scale 0.1` → `min=(0,0,0)`、`max=(0.1,0.1,0.01)` | `system/blockMeshDict:17,19-29` |
| `MeshSettings.cells` | `(20 20 1)` | `blockMeshDict` 的 `blocks` |
| `MeshSettings.grading` | `(1 1 1)` | 同上 |
| `DomainBox.mode` | `TwoD`（建议 + `IMPORT-DOM-001` Info） | `nz=1` |
| transportProperties | 文件只有 `nu 0.01;`（v2206 cavity 未写 `transportModel`）→ `nu=0.01`，`transportModel` 为空但按 OpenFOAM 默认 `Newtonian` 解释，并报 `IMPORT-TRANS-001`（Info） | `constant/transportProperties:17` |
| `turbulenceProperties` | **不存在** → `simulationType` 记为 `laminar`（v2206 默认语义）+ `IMPORT-TURB-001`（Info） | 目录无该文件 |
| `controlDict.application` | `icoFoam`；`endTime=0.5`；`deltaT=0.005`；`writeInterval=20` | `system/controlDict:17,25,27,31` |
| `fvSolution` | `solvers.p=PCG/DIC/1e-06/0.05`、`solvers.U=smoothSolver/symGaussSeidel/1e-05/0`、`PISO.nCorrectors=2` | `system/fvSolution` |
| `decomposeParDict` | `numberOfSubdomains=9`、`method=hierarchical`、`coeffs.n=(3 3 1)` | `system/decomposeParDict` |
| 报告 | 0 个 `BlockingError`；`system/PDRblockMeshDict` 为 `Recognized` | 目录实存 |

### 11.2 负向：`simpleFoam/pitzDaily`（G3）

| 项 | 期望 |
| --- | --- |
| `blockMeshDict.blocks` | 5 个 `hex` → `IMPORT-MESH-001`（**BlockingError**） |
| issue 位置 | `system/blockMeshDict`，行指向 `blocks` 关键字（v2206 该文件 `blocks` 关键字在 66 行，5 个 `hex` 元素在 68-84 行） |
| UI | 只提供“镜像导出”与“取消”，可编辑导入置灰 |
| `turbulenceProperties` | `simulationType=RAS`、`RASModel=kEpsilon` → `ReadOnly` 展示 |
| `0/` 场 | 7 个文件全部 `ReadOnly`（`k/epsilon/omega/nut/nuTilda/T/p/U`） |

### 11.3 部分降级：`simpleFoam/pipeCyclic/0.orig/U`（G4）

| 项 | 期望 |
| --- | --- |
| 起始目录 | `0.orig`（`0/` 不存在） |
| `#include` | 不展开 → `IMPORT-DIR-001`（Warning），该文件 `partial=true` |
| 报告首屏 | 显示“含未展开指令的文件：`0.orig/U`” |
| 导出 | 重写后保留 `#include` 行原文，位置与源一致（行号允许因 banner 对齐变化） |

### 11.4 导出等价：G1 未编辑 → 导出

| 断言 | 判定 |
| --- | --- |
| `0/p` | 与源**逐字节相同**（§9.4 第 1 条） |
| `0/U` | 与源**逐字节相同** |
| `system/controlDict`、`fvSchemes`、`fvSolution`、`blockMeshDict`、`constant/transportProperties`、`decomposeParDict` | **结构等价** |
| `system/PDRblockMeshDict` | 原样镜像（字节相同） |
| 目标目录 | `<caseRoot>_export`，与源目录不同，无残留 `*.tmp-*` |
| 清单 | `.ofgport-export.json` 列出 10 个文件动作，无绝对路径 |

### 11.5 编辑后导出：`cells 20 20 1 → 30 20 1`

| 断言 | 判定 |
| --- | --- |
| `system/blockMeshDict` | `blocks … (30 20 1)`，其余条目与源结构等价 |
| `0/p`、`0/U` | 与源逐字节相同（未编辑） |
| `MeshSettings` 校验 | 单元数 30×20×1=600，无 issue |
| 二次导入 | 导出的目录可被再次导入且得到相同模型（往返稳定性，Round-trip 测试） |

---

## 12. 测试策略

1. **分层**：`core` 纯单测（无文件系统）；`io` 以 `std::string` 为输入的单测 + fixture 文件测试；`ui` 用 `QTest::keyClicks`/信号槽断言做 smoke。
2. **表驱动**：`FoamLexerTest`/`FoamParserTest` 用 `(输入片段, 期望 token/节点, 期望 issue)` 表格，一行一个语法点（§5 每个子节 ≥1 行）。
3. **错误注入**：§5.3 表格每行一个用例；断言 `location.line` 与 `location.column` 精确值。
4. **黄金文件**：`tests/fixtures/golden/cavity/…` 存放期望输出（首次由实现者人工核对后提交），测试逐字节比较 `0/p`、`0/U`。
5. **往返**：导入 → 导出 → 再导入 → 比较模型快照（`CaseProject` 序列化为 JSON 后比较）。
6. **不依赖 `OpenFOAM-v2206/`**：CI 与本地测试均只用 `tests/fixtures/`（D-014）。
7. **回归**：修 bug 必须加最小复现用例（`AGENTS.md` §11）。

---

## 13. 任务拆分建议（供队长排期）

| ID | 任务 | 依赖 | 产出 | 验收要点 |
| --- | --- | --- | --- | --- |
| IMP-01 | 本规格（t1） | — | `docs/CASE_IMPORT_SPEC.md` | 评审通过 |
| IMP-02 | `core` 模型 + `CaseValidator` | IMP-01 | §10.2.1 列出的实际文件 + 单测 | 无 Qt 依赖；单测绿 |
| IMP-03 | `foam` AST + `FoamLexer`/`FoamParser` | IMP-02 | 实际文件清单（`src/io/Foam*`，见 §10.2.1）+ 表驱动测试 | §5.3 全场景通过 |
| IMP-04 | 投影器 + `CaseImportService` + `ImportReport` | IMP-03 | `core/import/*` | §11.1/§11.2/§11.3 断言通过 |
| IMP-05 | writers + `ExportPlan` + `AtomicDirectoryWriter` + `OpenFoamCaseExporter` | IMP-04 | `io/*Writer*` | §11.4/§11.5 通过 |
| IMP-06 | 导入向导 + 报告面板 | IMP-04 | `CaseImportDialog`、`CaseImportReportPanel`、`IssueListView` | 手动 smoke + 截图 |
| IMP-07 | Domain/Mesh/Patch/Field/Solver Inspector | IMP-04、IMP-06 | §10.4 面板 | 编辑写回模型并触发校验 |
| IMP-08 | 校验面板 + 导出预览 | IMP-05、IMP-07 | `ValidationPanel`、`ExportPreviewDialog` | 阻断态置灰；双击跳转 |
| IMP-09 | 端到端 + 文档收尾 | IMP-08 | fixtures、`docs/STATUS.md`/`TASKS.md` 更新 | G1–G5 全绿；无绝对路径 |

本表已原样落入 `docs/TASKS.md` 的「算例识别与前处理（IMP）」小节（含与 AgentTeams 任务 t1–t14 的映射与依赖边），后续排期以 `docs/TASKS.md` 为准。

---

## 14. 决策与风险

### 14.1 最终决策表（已记录于 `docs/DECISIONS.md`，2026-10-27）

| 编号 | 决策（摘要） | 理由 / 规格出处 |
| --- | --- | --- |
| **D-017** | `ValidationIssue` 追加 `location`/`nodePath`/`snippet`/`origin`，全部带默认值 | 定位要求（§6.6、§8.2）；不破坏既有构造点 |
| **D-018** | 导入场景导出目录名继承源（`0/` 或 `0.orig/`）；新建工程仍 `0.orig/` | v2206 `Allrun` 约定（§9.3） |
| **D-019** | 只读内容“原文保真”回写（`nonuniform` 列表、`#{…#}`、未展开 `#include`） | 避免重排/重算导致语义漂移（§6.5、§9.4） |
| **D-020** | 首版不展开 `#include` 家族、不解析 `polyMesh`、不执行 `#codeStream` | 范围与安全红线（§4.7、§5.5） |
| **D-021** | 拆出 `ofgui_core`/`ofgui_io` 静态库；一个子功能一个文件；显式源列表（禁止 GLOB）；单文件 ≤400 行 | 用户硬性要求 + 可定位性（§10） |
| **D-022** | 禁止导出到源目录（不得等于源根、不得位于源内）；新目录/空目录可直接用；已存在非空需显式确认并备份 `<target>.bak-<ts>` | 可回滚与源目录安全（§9.5） |
| **D-023** | 镜像（mirror）默认开启；未编辑/未识别文件逐字节复制；预览必须列出文件数与清单 | 保住导入算例可运行（§9.2、§9.7） |
| **D-024** | 首版只读场（`ReadOnly`/`Recognized`）不可改名、不可删除 | 避免跨字典引用失配（§7.5） |
| **D-025** | `.ofgproj` 记录源 case 相对路径与每文件 SHA-256（不记绝对路径） | “源已变化”提示与重导入（§6.7） |

### 14.2 风险

| 风险 | 影响 | 缓解 |
| --- | --- | --- |
| 逐字等价（§9.4 第 1 条）难以一次达成 | 黄金测试红 | 允许降级为结构等价，但必须显式记录（§9.4 末尾） |
| 教程里大量符号链接 | 扫描越界读到 `OpenFOAM-v2206` 之外 | 扫描不跟随链接（§4.1），测试覆盖 |
| 正则 patch 键（797 处）被误当 patch | 模型污染、导出错误 | 硬约束：正则键不入模型（§6.3） |
| `$` 展开与 `#include` 未展开导致显示值与实际求解值不一致 | 用户误判 | UI 标 `Partial`/`Unresolved` 并给原文（§8.4） |
| 单文件超 400 行被塞职责 | 违反用户硬性要求 | §10.8 评审清单强制 |
| `0/p` banner 对齐差异破坏逐字等价 | 黄金测试红 | banner 从源逐字复制（§9.4） |

---

## 15. 已决策问题（2026-10-27）

以下 5 个问题原为开放问题，已由队长拍板并记录为 `docs/DECISIONS.md` 的对应条目；实现时直接遵循，不再讨论。

| # | 问题 | 结论 | 决策 |
| --- | --- | --- | --- |
| 1 | 导出是否允许覆盖源目录？ | **禁止**：目标不得等于源 case 根，也不得位于源 case 内（任意层子目录）。允许形态 = 新目录或空目录；已存在且非空时必须显式确认，先备份为 `<target>.bak-<ts>` 再原子替换。不做“原地更新”模式。 | D-022 |
| 2 | 镜像默认值？ | **默认开启**（保住用户未在 GUI 中出现的文件）；`skip` 只用于运行时产物；导出预览必须列出将镜像的文件数与清单条目。 | D-023 |
| 3 | 只读场是否允许改名/删除？ | **否**：`ReadOnly`/`Recognized` 场只读展示、保真导出；仅 `Editable`（`p`、`U`、新建场）可改名/删除。 | D-024 |
| 4 | `0/` vs `0.orig/`？ | 导入场景**继承源目录名**（`0/`→`0/`，`0.orig/`→`0.orig/`）；新建工程仍 `0.orig/`。`ARCHITECTURE.md` §7.2 记为“导入场景例外”。 | D-018 |
| 5 | `.ofgproj` 是否记录源路径与哈希？ | **记录**：源相对路径、`startDirectory`、导入时间、每文件 SHA-256（不记绝对路径）；用于“源已变化”提示与重导入。 | D-025 |

理由要点（保留以备追溯）：

- 覆盖源目录会让“读源 → 写目标”在同一棵树上中途改写，无法保证一致性与回滚；源是用户唯一副本。
- 丢弃未知文件会让导入的算例无法运行，保真优先；清单可见即可控。
- 只读场改名会让 `boundaryField` 键、`fvSolution.solvers{}`、`controlDict.functions` 引用失配，首版无跨文件重命名追踪。
- v2206 的 `Allrun` 普遍假定 `0.orig` 存在并 `cp -r 0.orig 0`，强制改目录名会破坏用户工作流。
- 没有哈希就无法判断 GUI 内模型是否已过期。

---

## 附录 A 关键字典原文（黄金样例引用）

`OpenFOAM-v2206/tutorials/incompressible/icoFoam/cavity/cavity/0/p`（8-37 行，**逐字**）：

```text
FoamFile
{
    version     2.0;
    format      ascii;
    class       volScalarField;
    object      p;
}
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

dimensions      [0 2 -2 0 0 0 0];

internalField   uniform 0;

boundaryField
{
    movingWall
    {
        type            zeroGradient;
    }

    fixedWalls
    {
        type            zeroGradient;
    }

    frontAndBack
    {
        type            empty;
    }
}
```

`…/cavity/0/U`（8-38 行，**逐字**）：

```text
FoamFile
{
    version     2.0;
    format      ascii;
    class       volVectorField;
    object      U;
}
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

dimensions      [0 1 -1 0 0 0 0];

internalField   uniform (0 0 0);

boundaryField
{
    movingWall
    {
        type            fixedValue;
        value           uniform (1 0 0);
    }

    fixedWalls
    {
        type            noSlip;
    }

    frontAndBack
    {
        type            empty;
    }
}
```

`…/cavity/system/controlDict`（17-45 行）：

```text
application     icoFoam;

startFrom       startTime;

startTime       0;

stopAt          endTime;

endTime         0.5;

deltaT          0.005;

writeControl    timeStep;

writeInterval   20;

purgeWrite      0;

writeFormat     ascii;

writePrecision  6;

writeCompression off;

timeFormat      general;

timePrecision   6;

runTimeModifiable true;
```

`…/cavity/system/fvSolution` 关键片段：

```text
solvers
{
    p
    {
        solver          PCG;
        preconditioner  DIC;
        tolerance       1e-06;
        relTol          0.05;
    }

    pFinal
    {
        $p;
        relTol          0;
    }

    U
    {
        solver          smoothSolver;
        smoother        symGaussSeidel;
        tolerance       1e-05;
        relTol          0;
    }
}

PISO
{
    nCorrectors          2;
    nNonOrthogonalCorrectors 0;
    pRefCell        0;
    pRefValue       0;
}
```

---

## 附录 B 出处索引

### B.1 字典文件（`OpenFOAM-v2206/` 相对路径）

| 用途 | 路径 |
| --- | --- |
| 最小可导入算例（G1） | `tutorials/incompressible/icoFoam/cavity/cavity/` |
| 场文件 | `…/cavity/cavity/0/p`、`…/cavity/cavity/0/U` |
| 网格字典 | `…/cavity/cavity/system/blockMeshDict` |
| `scale` 写法 | `…/cavity/cavity/system/blockMeshDict:17` |
| `controlDict` | `…/cavity/cavity/system/controlDict:17-45` |
| `fvSchemes` | `…/cavity/cavity/system/fvSchemes:17-47` |
| `fvSolution`（含 `$p;`） | `…/cavity/cavity/system/fvSolution:17-48` |
| `transportProperties` | `…/cavity/cavity/constant/transportProperties:17` |
| `decomposeParDict` | `…/cavity/cavity/system/decomposeParDict:17-24` |
| `PDRblockMeshDict`（仅识别） | `…/cavity/cavity/system/PDRblockMeshDict` |
| `dimensioned nu` 写法 | `tutorials/incompressible/lumpedPointMotion/bridge/steady/constant/transportProperties:19` |
| `turbulenceProperties`（RAS） | `tutorials/incompressible/simpleFoam/pitzDaily/constant/turbulenceProperties:17-28` |
| 多块负例（5 hex） | `tutorials/incompressible/simpleFoam/pitzDaily/system/blockMeshDict:66-84` |
| `#includeEtc` 样例 | `tutorials/incompressible/simpleFoam/pipeCyclic/0.orig/U:23` |
| `#include`（无分号）样例 | `tutorials/basic/chtMultiRegionFoam/2DImplicitCyclic/0.orig/p:23` |
| `$internalField` 样例 | `tutorials/basic/overPotentialFoam/cylinder/cylinderAndBackground/0.orig/U:41,48` |
| `${{ … }}` 样例 | `tutorials/multiphase/compressibleInterFoam/laminar/climbingRod/0.orig/U:28` |
| `nonuniform List<vector>` 大列表 | `tutorials/DNS/dnsFoam/boxTurb16/0.orig/U:19-4118` |
| 正则 patch 键 + `patchType` | `tutorials/basic/overLaplacianDyMFoam/heatTransfer/0.orig/pointDisplacement:23,29,31` |
| `#{ … #}` verbatim | `tutorials/incompressible/simpleFoam/pipeCyclic/0.orig/U:31-40` |
| `format binary` 负例 | `tutorials/lagrangian/DPMFoam/Goldschmidt/0/U.air:11` |
| `polyMesh/boundary` 形状（不解析） | `tutorials/finiteArea/sphereSurfactantFoam/sphereTransport/constant/polyMesh/boundary.gz` |

### B.2 v2206 源码（只读参考，用于确认语法与默认值）

| 主题 | 路径:行 |
| --- | --- |
| token 枚举（字母表） | `src/OpenFOAM/db/IOstreams/token/token.H:77-107` |
| `FoamFile` 头必需键 | `src/OpenFOAM/db/IOobject/IOobjectReadHeader.C:39-56` |
| `$` 作用域语法注释 | `src/OpenFOAM/db/dictionary/dictionary.H:55-74` |
| `$` 查找与错误 | `src/OpenFOAM/db/dictionary/entries/primitiveEntry.C:75-176` |
| `#include` 路径基准 | `src/OpenFOAM/db/dictionary/functionEntries/includeEntry.C:137` |
| 指令注册表（含 `includeIfPresent`） | 同上 `:56-99` |
| 指令全集目录 | `src/OpenFOAM/db/dictionary/functionEntries/` |
| 维度指数顺序 | `src/OpenFOAM/dimensionSet/dimensionSet.H`（`enum dimensionType`） |
| 时间目录命名 | `src/OpenFOAM/db/Time/Time.C:780`（`timeName()`） |

### B.3 统计数字的来源方法

- BC 类型、`class` 分布、`#include` 出现次数：对 `OpenFOAM-v2206/tutorials/**/0*/` 下 2392 个文件做正则计数（`type\s+(\w+)\s*;`），跳过符号链接/联接点。
- `controlDict` / `turbulenceProperties` / `transportProperties` / `blockMeshDict` 分布：对同名文件做全文正则计数，样本数分别为 432 / 320 / 224 / 381。
- 口径说明：同一文件内重复出现按次计数；`empty` 等类型在 2D 算例中重复出现较多属正常。
- 复现方式（PowerShell，只读）：`Get-ChildItem -Recurse -File -Filter '<pattern>' | Where-Object { $_.LinkType -eq $null }` 再逐个读取计数；**不要**用 `Select-String`/ripgrep 直接扫 tutorials（会因符号链接报 `os error 1920`）。

---

## 附录 C 错误码总表

| 码 | 级别 | 触发条件 | 定位要求 |
| --- | --- | --- | --- |
| `IMPORT-CASE-001` | Blocking | 缺 `system/controlDict` | 目录 |
| `IMPORT-CASE-002` | Error | 无 `0/` 且无 `0.orig/` | 目录 |
| `IMPORT-CASE-003` | Blocking | 目标目录 = 源或源子目录 | 路径 |
| `IMPORT-CASE-004` | Warning | 缺 `constant/` | 目录 |
| `IMPORT-CASE-005` | Blocking | 算例根目录不存在或不可读 | 路径 |
| `IMPORT-CASE-006` | Blocking | `system/controlDict` 无法解析，得不到求解设置 | 文件行+列 |
| `IMPORT-LEX-001` | Error | 未闭合字符串 | 行+列 |
| `IMPORT-LEX-002` | Error | 未闭合块注释 | 行+列（注释起点） |
| `IMPORT-LEX-003` | Error | 未闭合 `#{ … #}` | 行+列 |
| `IMPORT-LEX-004` | Warning | 非法控制字符，已跳过 | 行+列 |
| `IMPORT-LEX-005` | Warning | 单词内 `(` 未闭合，按读到 EOF 处理 | 行+列 |
| `IMPORT-LEX-006` | Warning | 数字格式非法，已按单词读入 | 行+列 |
| `IMPORT-LEX-007` | Warning | 孤立 `$` 或未闭合 `${ … }` | 行+列 |
| `IMPORT-PARSE-001` | Error | 缺 `;` | 行+列（新 entry 起点） |
| `IMPORT-PARSE-002` | Error | 缺 `}`/`)` 到 EOF | 行+列（开括号） |
| `IMPORT-PARSE-003` | Error | 缺 `FoamFile` 头 | 行 1 |
| `IMPORT-PARSE-004` | Error | 头缺 `format`/`class` | 头内行+列 |
| `IMPORT-PARSE-005` | Warning | 头缺 `object` 或与文件名不一致 | 头内行 |
| `IMPORT-PARSE-006` | Error | 多余 `}`/`)` | 行+列 |
| `IMPORT-PARSE-007` | Error | 期望 keyword，读到其它 token | 行+列 |
| `IMPORT-PARSE-008` | Error | 多余的 `;`（无条目可结束） | 行+列 |
| `IMPORT-SYN-001` | Warning | `${{ … }}` 未求值 | 行+列 |
| `IMPORT-SYN-002` | Warning | `${a:-b}` 未展开 | 行+列 |
| `IMPORT-SYN-003` | Warning | 同 keyword 重复 | 每处行+列 |
| `IMPORT-VAR-001` | Error | `$` 目标不存在 | 行+列 |
| `IMPORT-VAR-002` | Warning | `$..`/`$...`/`$^` 未支持 | 行+列 |
| `IMPORT-DIR-001` | Warning | `#include*` 未展开 | 行+列 |
| `IMPORT-DIR-002` | Warning | 其它指令未执行 | 行+列 |
| `IMPORT-DIR-003` | Warning | 未知指令 | 行+列 |
| `IMPORT-FMT-001` | Warning | `format binary` | 头内行 |
| `IMPORT-ENC-001` | Error | UTF-16/NUL 字节 | 行+列 |
| `IMPORT-ENC-002` | Warning | 非 ASCII token | 行+列 |
| `IMPORT-ENC-003` | Info | UTF-8 BOM 已剥离 | — |
| `IMPORT-MESH-001` | Blocking | `blocks` >1 个 `hex` | `blocks` 行 |
| `IMPORT-MESH-002` | Blocking | 非 `hex` 块元素 | 元素行 |
| `IMPORT-MESH-003` | Warning | `scale` 为 `SymmTensor` | `scale` 行 |
| `IMPORT-MESH-004` | Error | 顶点数 ≠ 8 | `vertices` 行 |
| `IMPORT-MESH-005` | Warning | `grading` 非 3 值 | grading 行 |
| `IMPORT-MESH-006` | Warning | `edges` 非空 | `edges` 行 |
| `IMPORT-MESH-007` | Error | 顶点无法构成长方体 | `vertices` 行 |
| `IMPORT-MESH-008` | Warning | 缺 `scale`，按 1.0 | — |
| `IMPORT-MESH-009` | Blocking | `cells` 任一 ≤0 | `blocks` 行 |
| `IMPORT-DOM-001` | Info | 建议 2D 模式 | — |
| `IMPORT-DOM-002` | Info | 单位按 SI 假设 | — |
| `IMPORT-PATCH-001` | Warning | 正则 patch 键未入模型 | 键行 |
| `IMPORT-PATCH-002` | Error | patch 缺 `p`/`U` 的 BC | 场文件 `boundaryField` 行 |
| `IMPORT-PATCH-004` | Error | 空 patch 名 | 名行 |
| `IMPORT-PATCH-005` | Error | `type` 非法 | 值行 |
| `IMPORT-PATCH-006` | Error | 同一面跨 patch | patch 行 |
| `IMPORT-PATCH-007` | Warning | patch 类型未知，回落 `patch` | 场文件行 |
| `IMPORT-PATCH-008` | Warning | `blockMeshDict` 与场文件 patch 不一致 | 双方行 |
| `IMPORT-FIELD-001` | Error | `dimensions` 缺失/非 7 元素 | 行+列 |
| `IMPORT-BC-001` | Warning | 未知 BC `type`，只读保留 | 行+列 |
| `IMPORT-BC-002` | Error | BC 缺 `type` | patch 行 |
| `IMPORT-TURB-001` | Info | 无 `turbulenceProperties`，按 laminar | — |
| `IMPORT-TRANS-001` | Info | 无 `transportModel`，按默认 `Newtonian` | — |
| `IMPORT-UNSUP-001` | Warning | `functions{}` 等未接管子树 | 行+列 |
| `IMPORT-LIM-001` | Error | 文件超 32 MiB | 路径 |
| `IMPORT-LIM-002` | Warning | `nonuniform` 元素超 5e6 | 行+列 |
| `IMPORT-REPORT-001` | Info | issue 已截断至 200 | — |
| `IMPORT-REPORT-002` | Warning | 文件数超 5000，已限深 | 目录 |
| `IMPORT-EXP-001` | Blocking | 目标与源路径冲突 | 路径 |
| `IMPORT-EXP-002` | Blocking | 目标目录非空未确认 | 路径 |
| `IMPORT-EXP-003` | Error | 写导出文件失败 | 路径 |
| `IMPORT-EXP-004` | Error | 目标目录不可写 | 路径 |
| `IMPORT-EXP-005` | Info | `PDRblockMeshDict` 与 `blockMeshDict` 并存 | 路径 |
| `IMPORT-EXP-006` | Blocking | 工程校验未通过拒绝导出 | 路径 |
| `IMPORT-EXP-007` | Warning | 输出含非 ASCII | 行+列 |
| `IMPORT-EXP-008` | Error | 镜像源文件复制失败（源缺失/不可读） | 路径 |
| `IMPORT-EXP-009` | Error | 导出清单 `.ofgport-export.json` 写入失败 | 路径 |
| `IMPORT-EXP-010` | Warning | 镜像不完整（符号链接未跟随或触及文件数上限） | 路径 |

> `IMPORT-PARSE-004`/`005` 由导入器在 `FoamFile` 头层判定（头缺 `format`/`class`、`object` 缺失或不一致）；
> 字典解析器自身只发出结构层编号 `001`–`003`、`006`–`008`，不再占用 `004`/`005`。

---

## 附录 D 字段速查（keyword → 模型 → writer）

| 字典 | keyword | 模型 | writer | 可编辑 |
| --- | --- | --- | --- | --- |
| `0/p`、`0/U` | `dimensions` | `FieldDefinition.dimensions` | `FieldFileWriter` | 否 |
| 同上 | `internalField` | `FieldValue` | 同上 | 是（`uniform`） |
| 同上 | `boundaryField.<patch>.type` | `BoundaryCondition.type` | 同上 | 是（白名单） |
| 同上 | `boundaryField.<patch>.value` 等 | `BoundaryCondition.parameters` | 同上 | 视类型 |
| `system/blockMeshDict` | `scale` | `MeshSettings.scale` | `BlockMeshDictWriter` | 是 |
| 同上 | `vertices` | `MeshBlock.vertices` | 同上 | 否（域驱动） |
| 同上 | `blocks` | `MeshSettings.cells`/`grading` | 同上 | 是（cells/grading） |
| 同上 | `boundary.<name>.type` | `BoundaryPatch.type` | 同上 | 是 |
| 同上 | `boundary.<name>.faces` | `BoundaryPatch.faces` | 同上 | 是（勾选） |
| `system/controlDict` | `application` | `ControlDictSettings.application` | `ControlDictWriter` | 是 |
| 同上 | `startFrom`/`stopAt`/`writeControl`/`writeFormat`/`timeFormat` | 同名枚举字段 | 同上 | 是 |
| 同上 | `startTime`/`endTime`/`deltaT`/`writeInterval`/`writePrecision`/`timePrecision`/`purgeWrite` | 同名字段 | 同上 | 是 |
| 同上 | `writeCompression`/`runTimeModifiable` | 同名 `bool` | 同上 | 是 |
| 同上 | `functions` | `ControlDictSettings.unmanagedSubtree` | 同上 | 否（原文） |
| `system/fvSchemes` | 6 类 scheme | `FvSchemesSettings.categories` | `FvSchemesWriter` | 是 |
| `system/fvSolution` | `solvers.<name>.*` | `FvSolutionSettings.solvers` | `FvSolutionWriter` | 是 |
| 同上 | `PISO`/`PIMPLE`/`SIMPLE` 参数 | `FvSolutionSettings.algorithm` | 同上 | 是 |
| `constant/transportProperties` | `transportModel` | `TransportModelSettings.model` | `TransportPropertiesWriter` | 仅 `Newtonian` |
| 同上 | `nu` | `TransportModelSettings.nu` | 同上 | 是 |
| `constant/turbulenceProperties` | `simulationType`、`RAS.*`、`LES.*` | `TurbulenceSettings` | `TurbulencePropertiesWriter` | 否 |
| `system/decomposeParDict` | `numberOfSubdomains`/`method`/`coeffs.n` | `DecomposeSettings` | `DecomposeParDictWriter` | 否 |

---

## 附录 E 变更记录

| 版本 | 日期 | 变更 |
| --- | --- | --- |
| 1.0 | 2026-10-27 | 首版：支持矩阵、语法子集、数据结构映射、UI 字段、错误定位、导出与黄金文件、文件组织规范、验收示例 |
| 1.1 | 2026-10-27 | IMP-03/IMP-04 落地回填：错误码表补 `CASE-005`/`CASE-006`/`LEX-005`–`007`/`PARSE-007`/`008`/`TRANS-001`；说明 `PARSE-004`/`005` 由导入器判定；§11.1 修正 cavity `transportProperties` 实际只有 `nu 0.01;`（无 `transportModel`），按默认 `Newtonian` 解释 |
| 1.2 | 2026-10-27 | IMP-05 写出器落地回填：§9.4 布局改为“值列 = 缩进 + 16 / `FoamFile` 固定第 16 列 / 79 字符定宽 banner”，记录 cavity 六个字典逐字节复现与 `fvSolution` 的 `$p;` 展开规则；§9.2 说明 t7 只生成模型驱动的 7 个文件，镜像与清单属下一段；附录 C 补 `EXP-001/002/003/004/006` |
| 1.3 | 2026-10-28 | t17 落地回填：`.ofgproj` 升到 schemaVersion 2，§6.7 补“`source` 段常写、v1 内存迁移、绝对路径读 Error/写 Warning、`sha256` 小写 64 位”的落地状态（详见 D-034） |
| 1.4 | 2026-10-28 | t18 落地回填：§9.2 动作表全部实现（`Written`/`Copied`/`Mirrored`/`skipped` + `CaseMirror` + `ExportManifest`）；§9.5 第 3/4/6 条改写为实际实现（临时目录 `<target>.tmp-<seq>-<stamp>`、清单最后写入且不列自身、`skipped[]` 独立列出并带 `reason`）；附录 C 的 `EXP-008` 更正为“镜像复制失败”，新增 `EXP-009`（清单写入失败）与 `EXP-010`（镜像不完整） |

