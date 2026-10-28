# 独立集成验证报告（t13）

> 验证日期：2026-10-28（Asia/Shanghai）  
> 验证人：verifier（独立验证，未参与业务实现）  
> 范围：首版可交付性——clean Debug/Release 构建、全量测试、运行 smoke 与截图、真实 v2206 教程算例端到端走查、AGENTS 硬边界、文件组织规则、文档与代码一致性。  
> 结论：**可交付（有 1 项需跟进的技术债）**。所有构建/测试/smoke/走查命令退出码为 0；硬边界逐项有证据；发现 7 个超过 D-021 400 行上限的源文件，拆分方案已按规则记录在 `docs/CASE_IMPORT_SPEC.md` §10.9，作为 t14 前技术债。验证过程中修复 6 处 `-Wunused-result`（一行级）。

## 1. 环境

| 项 | 值 |
| --- | --- |
| 工作区 | `D:\6.developOpenFOAM` |
| Qt | 6.11.2（`D:\3.wpsAndOther\Qt\setting\6.11.2\mingw_64`） |
| CMake | 3.30.5（`D:\3.wpsAndOther\Qt\setting\Tools\CMake_64\bin\cmake.exe`） |
| Ninja | `D:\3.wpsAndOther\Qt\setting\Tools\Ninja\ninja.exe` |
| MinGW | 13.1.0（`D:\3.wpsAndOther\Qt\setting\Tools\mingw1310_64\bin`） |
| VTK | 9.6.2（`D:\3.wpsAndOther\VTK\install`） |
| 构建目录 | `build/windows-mingw-debug`、`build/windows-mingw-release` |
| 证据目录 | `build/verify-logs/`（构建日志、截图、走查程序与日志） |

复现前先设置工具链 PATH（`scripts/Set-ProjectEnvironment.ps1` 等价）：

```powershell
$env:Path = 'D:\3.wpsAndOther\Qt\setting\6.11.2\mingw_64\bin;D:\3.wpsAndOther\Qt\setting\Tools\mingw1310_64\bin;D:\3.wpsAndOther\Qt\setting\Tools\Ninja;D:\3.wpsAndOther\VTK\install\bin;' + $env:Path
```

> 注意：不设置该 PATH 直接运行 `ctest` 会因缺少 Qt/VTK DLL 报 `0xc0000139`（入口点未找到），这是运行环境问题，不是测试失败；`scripts\Build-Project.ps1` 会自动设置。

## 2. 构建与测试

### 2.1 Clean Debug（最终一次）

```powershell
Remove-Item -Recurse -Force build\windows-mingw-debug
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\Build-Project.ps1 -Configuration Debug
```

- 退出码：**0**
- 警告：**0**（`Select-String 'warning:'` 计数 0，日志 `build/verify-logs/debug-clean-final.log`）
- CTest：**43/43 passed**（`100% tests passed, 0 tests failed`）
- 编译选项含 `-Wall -Wextra -Wpedantic`（见构建日志 compile 行）。
- 构建日志：`build/verify-logs/debug-clean-final.log`

### 2.2 Clean Release

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\Build-Project.ps1 -Configuration Release
```

- 退出码：**0**
- 警告：**0**
- CTest：**43/43 passed**
- 日志：`build/verify-logs/release-build.log`

### 2.3 单独复跑 CTest（两配置）

```powershell
ctest --test-dir build/windows-mingw-debug --output-on-failure    # exit 0，43/43
ctest --test-dir build/windows-mingw-release --output-on-failure  # exit 0，43/43
```

测试构成与 `docs/STATUS.md` 一致：core 17 个可执行 + io 19 个套件 + 2 个 render 套件 + 5 个 UI 套件 = 43。

### 2.4 验证中修复的警告（一行级）

首轮 clean Debug 暴露 6 处 `-Wunused-result`：`[[nodiscard]] MeshSettings::setCellCount` 的返回值在“构造后立即使用对象”的上下文被忽略。

| 文件:行 | 修复 |
| --- | --- |
| `src/app/main.cpp:78` | 调用前加 `(void)` |
| `tests/ui/ExportWorkflowTest.cpp:198` | 调用前加 `(void)` |
| `tests/ui/ExportWorkflowTest.cpp:293` | 调用前加 `(void)` |
| `tests/ui/ExportWorkflowTest.cpp:317` | 调用前加 `(void)` |
| `tests/ui/ExportWorkflowTest.cpp:322` | 调用前加 `(void)` |
| `tests/ui/ExportWorkflowTest.cpp:323` | 调用前加 `(void)` |

修复后增量构建 0 warning（`build/verify-logs/debug-rebuild.log`），最终 clean Debug 再次确认 0 warning。

## 3. 运行 smoke 与截图

所有命令退出码 **0**。截图均在 `build/verify-logs/`，已逐张目视核对。

| # | 命令 | 截图 | 核对结论 |
| --- | --- | --- | --- |
| 1 | `scripts\Run-Application.ps1 -SmokeTest` | — | 退出码 0，无异常输出 |
| 2 | `scripts\Run-Application.ps1 -SmokeTest -Screenshot build\verify-logs\smoke-default.png` | `smoke-default.png`（1440×860） | 启动即显示默认 cavity 模板：3 patch、2 场、icoFoam；校验 0/0/0/0；左侧树/Inspector/VTK 视口/校验 dock 布局正常 |
| 3 | `OpenFOAMGUI.exe --open-case <cavity> --smoke-test --screenshot ...` | `smoke-cavity-tree.png` | 工程节点显示来源算例、patch 3、场 2、icoFoam、校验 0/0/1/0、导入时间、记录文件 9；状态栏“已加载算例：9 个文件，5 个校验问题（阻断 0）”；树含六面→patch 映射与 U/p→BC |
| 4 | `--open-case <cavity> --select-domain` | `smoke-cavity-domain.png` | 计算域页回填 min 0/0/0、max 0.1/0.1/0.01、cells 20/20/1；尺寸 0.1×0.1×0.01 m、400 单元；3D 下 z=1 的灰色提示存在 |
| 5 | `--open-case <cavity> --select-field U` | `smoke-cavity-field-U.png` | 场页显示 volVectorField、dimensions `[0 1 -1 0 0 0 0]` 只读、internalField 三轴为 0；下半部 BC 表在滚动区内（`FieldEditor.cpp:63-65` 的 `QScrollArea`） |
| 6 | `--open-case <cavity> --select-solver` | `smoke-cavity-solver.png` | controlDict 页回填 icoFoam/0/0.5/0.005/20；只读提示说明 startFrom/stopAt/writeControl 导出固定写法 |
| 7 | `--open-case <cavity> --select-solver --solver-transport` | `smoke-cavity-solver-transport.png` | constant 页显示 transportModel（默认 Newtonian）与 nu 0.01；simulationType 未声明→按 laminar 只读说明 |
| 8 | `--open-case <cavity> --export-preview --export-target ...` | `smoke-export-preview.png`（1000×660） | 预览文件树 9 项（新写 7/复制 1/镜像 1），U 预览含 `fixedValue`/`uniform (1 0 0)`，页脚统计与计划一致 |
| 9 | `--open-case <cavity> --export-preview --export-blocked --export-target ...` | `smoke-export-blocked.png` | 阻断态：页脚“导出预览失败：阻断 CASE-MESH-001 网格数 nx 必须大于 0”，导出按钮置灰 |
| 10 | Release 版 `--open-case <cavity> --smoke-test --screenshot ...` | `smoke-release-cavity.png` | Release 构建可启动并正确加载算例 |

## 4. 真实 v2206 教程算例端到端走查

教程路径（注意实际算例在二级 `cavity/cavity`）：
`OpenFOAM-v2206/tutorials/incompressible/icoFoam/cavity/cavity`。

为避免给产品加测试专用代码，走查程序放在 `build/verify-logs/walkthrough/`（仅验证产物，不进入产品源码/CMake）：

- `CMakeLists.txt`：编译产品源码 `src/ui/CaseDocumentController.cpp`、`ExportController.cpp`、`DefaultCaseTemplate.cpp`、`ExportDictionaryCheck.cpp`（AUTOMOC），链接 `build/windows-mingw-debug/src/io/libofgui_io.a`、`src/core/libofgui_core.a` 与 Qt6。
- `main.cpp`：用产品 API 完成导入→改模型→校验→预览→导出→再导入→逐项断言。
- 运行日志：`build/verify-logs/walkthrough.log`，退出码 **0**，输出 `WALKTHROUGH RESULT: PASS (failures=0)`。

### 4.1 导入

| 项 | 实测 |
| --- | --- |
| 工程名 | cavity |
| 计算域 | `[0,0,0] .. [0.1,0.1,0.01]` |
| 网格 | 20×20×1 |
| patch | movingWall、fixedWalls、frontAndBack |
| 场 | U（volVectorField）、p（volScalarField） |
| 求解 | icoFoam，startTime 0、endTime 0.5、deltaT 0.005、writeInterval 20 |
| transport | nu=0.01（原文 `nu 0.01;`，无 transportModel） |
| 导入报告 | 9 个文件、5 个 issue、阻断 0 |
| 模型校验 | 阻断 0、错误 0、警告 1（CASE-MESH-006 3D 下 z 只有 1 层）、提示 0 |

### 4.2 编辑（全部经产品模型 API，与 UI 路径同源）

| 对象 | 修改后 |
| --- | --- |
| 计算域 | max.x 0.10 → 0.12（y/z 不变） |
| 网格 | cells 20×20×1 → 30×20×1 |
| U internalField | `uniform (2 0 0)` |
| U movingWall BC | `fixedValue`，`value uniform (2 0 0)` |
| p internalField | 1.5 |
| 求解 | endTime 0.6、deltaT 0.0025、writeInterval 10 |
| transport | nu 0.02（原文文本清空，走结构化值） |

编辑后 `isDirty()==true`；校验仍为 0 阻断，可导出。

### 4.3 导出与再导入

导出目标：`build/verify-logs/walkthrough/export-cavity`（新目录）。

- 预览：`issues=1`，可导出。
- 导出结果：`导出完成：新写 7、复制 1、镜像 1、跳过 0，共 9 个文件。`
- 清单回读：`manifestVerified=true`、`manifestFiles=9`，实际写出 10 个文件（含 `.ofgport-export.json`）。
- 文件格式：CRLF 文件 0、BOM 文件 0、缺少行尾换行 0、非 ASCII 字节 0（导出字典全 ASCII + LF）。
- 清单统计：written 7、copied 1（`system/decomposeParDict`）、mirrored 1（`system/PDRblockMeshDict`）、skipped 0。

再次导入导出目录后逐项断言通过：

| 断言 | 结果 |
| --- | --- |
| 域 max.x = 0.12 | PASS |
| 网格 30×20×1 | PASS |
| 3 patch / 2 场 | PASS |
| endTime 0.6 / deltaT 0.0025 / writeInterval 10 | PASS |
| nu 0.02 | PASS |
| 阻断 0 | PASS |
| U internalField `uniform (2 0 0)` | PASS |
| p internalField 1.5 | PASS |
| movingWall `fixedValue` + `uniform (2 0 0)` | PASS |
| 导出文本精确包含 `internalField   uniform (2 0 0)`、`value           uniform (2 0 0)`、`endTime         0.6`、`nu              0.02` | PASS |

导出 `0/U` 片段（日志）：

```text
internalField   uniform (2 0 0);

boundaryField
{
    movingWall
    {
        type            fixedValue;
        value           uniform (2 0 0);
    }
    ...
}
```

## 5. AGENTS 硬边界核查

| 边界 | 证据 | 结论 |
| --- | --- | --- |
| 不编译 OpenFOAM | 根 `CMakeLists.txt` 无 `add_subdirectory(OpenFOAM-v2206)`；`OpenFOAM-v2206/` 不在任何 `add_library`/源列表；构建 299 步全部为本项目源文件 | 通过 |
| 不链接 OpenFOAM 库 | 链接行只含 ofgui_core/ofgui_io/ofgui_ui/ofgui_render + Qt6 + VTK | 通过 |
| 不包含 OpenFOAM 头文件 | `src/`、`tests/` 中无 `#include <OpenFOAM...>` / `#include "OpenFOAM...`；仅有本项目 `Foam*` 头 | 通过 |
| 不使用 wmake / OpenFOAM 构建 | 仓库无 `wmake` 调用；构建入口只 `Build-Project.ps1`/CMake/Ninja | 通过 |
| OpenFOAM-v2206 只读参照 | 近 2 天 `OpenFOAM-v2206/` 修改文件数 = **0**；测试仅以 `OFGUI_FOAM_REFERENCE_DIR`/`OFGUI_FOAM_FIXTURE_DIR` 编译定义读取路径（`src/io/CMakeLists.txt:177`、根 `CMakeLists.txt:208,233,257,282,307`） | 通过 |
| 无 OpenFOAM 也可启动/编辑/导出 | 走查全程未安装/未调用 OpenFOAM；GUI 与导出只依赖自身代码 | 通过 |
| `core` 不依赖 Qt Widgets/VTK | `src/core` 头/源无 Qt/VTK/QObject/QString/vtk 符号（grep 无命中）；`src/core/CMakeLists.txt` 链接空，仅 C++20 | 通过 |
| `io` 不负责界面交互 | `src/io` 只做解析/投影/写盘；无 Widgets 头；`io` 仅链 `Qt6::Core`（`AtomicFileWriter` 用 QString 报错文本） | 通过 |
| `ui` 不直接拼 OpenFOAM 字典 | UI 预览走 `io::FieldFileWriter`/`ControlDictWriter`/`TransportPropertiesWriter`；UI 无字典文本拼接（只显示导入原文） | 通过 |
| 业务源码无绝对路径 | `src/`、`tests/` 中 `\b[A-Za-z]:[\\/]` 命中仅测试数据（`tests/io/FoamLexerTest.cpp:147,152`、`ProjectJsonMigrationTest.cpp:309`、`ProjectJsonRepositoryErrorsTest.cpp:319,330`、`ProjectJsonSourceTest.cpp:271,284`），无产品硬编码路径 | 通过 |
| 字典 LF / ASCII 优先 | `src/`、`tests/` 的 `*.cpp/*.h` CRLF=0、BOM=0；导出算例 CRLF=0、非 ASCII=0、均有行尾换行 | 通过 |
| 所有输出经校验 + 临时文件/目录后原子替换 | 导出：`io::writers::AtomicCaseDirectory`（`src/io/writers/CaseExporter.cpp:130` 附近），`<target>.tmp-<seq>-<stamp>` → 目录 rename；工程保存：`io::AtomicFileWriter`（`src/io/AtomicFileWriter.cpp:93-140`）先写 `.tmp-<random>` 再 rename，失败清理；`CaseExporterTest`/`ExportWorkflowTest` 断言无 `.tmp-*` 残留 | 通过 |
| 覆盖保护 | 目标位于源算例内 → 拒绝；非空目标未确认 → `IMPORT-EXP-002`；确认后备份 `<target>.bak-<ts>`；工程另存为走原子替换（`ExportController` + `AtomicFileWriter`） | 通过 |
| 稳定 ID | `core::ObjectId`（`src/core/ObjectId.h`，64 位、与显示名解耦）；树 `ProjectNodeRef`（NodeKind + FaceId/fieldId/patchId）、`.ofgproj` 以 id 持久化、VTK 面以 `core::FaceId` 标识；改名不换 id | 通过（面高亮 API 未实现属已知 M3-04） |
| 用户可见文本 tr() | `tr()` 调用 364 处；`ui` 中含中文但未直接 `tr()` 的 15 行均为 `QCoreApplication::translate` 或跨行 `tr()` 续行/注释（`MainWindow.cpp:628-630`、`NodeDetailFormatter.cpp:213`、`SolverEditor.cpp:347`、`ExportPreviewDialog.cpp:37-45`、`ValidationPanel.cpp:28-36`、`ProjectTreeModel.h` 注释）。core/io 的诊断中文是数据字符串（详见 §6.4） | 部分符合（低风险偏差，见 F2） |

## 6. 用户额外规则：文件组织

### 6.1 一个类/子功能一个文件

- `src/` 共 244 个头/源文件（另有 2 个 `CMakeLists.txt`）；`.cpp` 内未发现第二个对外类定义。仅有的 `.cpp` 内类是文件私有的实现细节：
  - `src/io/BlockMeshProjector.cpp:43` `struct Vertices`
  - `src/io/ImportIssueFactory.cpp:13` `struct CodeRow`
  - `src/io/ProjectJsonReadFields.cpp:123` `struct LoadedCondition`
  - `src/io/writers/BlockMeshDictWriter.cpp:20` `struct GridCorner`
- 公共类均一文件一 `.h`/`.cpp`（如 `DomainEditor`、`SolverEditor`、`FieldBoundaryTable`、`ExportController`、`CaseExporter`）。

### 6.2 CMake 与源文件清单一致

- 根 `CMakeLists.txt` 显式列出全部 `src/ui`、`src/app`、`src/render` 源文件；`src/core/CMakeLists.txt`、`src/io/CMakeLists.txt` 同。
- 校验脚本结果：ui 缺登记 0、core 缺登记 0、io 缺登记 0、writers 缺登记 0；无 `file(GLOB ...)`。
- 新增测试均“先落盘再登记”，configure/build 通过。

### 6.3 文件行数（>300 需理由；D-021 上限 400）

合规但超过 300 行的单职责文件（理由：一个类的构造/校验分支较多，或单一测试套件）：

| 文件 | 行数 | 理由 |
| --- | --- | --- |
| `src/io/FoamParserValues.cpp` | 374 | 单一 `FoamParser` 的值语法实现（列表/字典/维度集/指令） |
| `src/ui/DomainEditor.cpp` | 367 | 单一编辑器控件 + 候选值校验 |
| `src/io/ProjectJsonReadFields.cpp` | 359 | 单一投影读取实现（internalField/boundaryField） |
| `src/io/BlockMeshProjector.cpp` | 356 | 单一投影器（含顶点/块解析私有结构） |
| `src/io/CaseScanner.cpp` | 345 | 单一只读扫描器（多目录探测） |
| `src/io/FoamDictionary.cpp` | 342 | 单一 AST 渲染器 |
| `src/io/ProjectJsonRepository.cpp` | 328 | 单一工程 JSON 读写仓库 |
| `tests/ui/ExportWorkflowTest.cpp` 等 | 302–395 | 每个测试文件一个测试类/一套纵向场景 |

**超 400 行（D-021 第 4 条违反项，已记录拆分方案）**：

| 文件 | 行数 | 拆分方案位置 |
| --- | --- | --- |
| `src/ui/MainWindow.cpp` | 633 | `docs/CASE_IMPORT_SPEC.md` §10.9 |
| `src/ui/SolverEditor.cpp` | 601 | 同上 |
| `src/ui/FieldEditor.cpp` | 520 | 同上 |
| `src/ui/FieldBoundaryTable.cpp` | 456 | 同上 |
| `src/ui/CaseDocumentController.cpp` | 439 | 同上 |
| `src/ui/ExportPreviewDialog.cpp` | 409 | 同上 |
| `src/core/CaseValidator.cpp` | 402 | 同上 |

这些文件均为“一个对外类一个文件”、没有多类混装，但确实超出项目自定的 400 行上限；t13 已按规则补记拆分方案，拆分执行列入 Backlog，建议 t14 前完成或由评审明确接受。

### 6.4 tr() 覆盖

- `tr()` 调用：364 处。
- 含中文但未直接 `tr()`/`translate` 的行：141 行 = core 51、io 75、ui 15。
- ui 的 15 行已逐条确认全部是 `QCoreApplication::translate`（`ExportPreviewDialog.cpp:37-45`、`ValidationPanel.cpp:28-36`）或跨行 `tr()` 的续行/注释，不是漏翻。
- core 51 / io 75 行是诊断/报错字符串（如 `src/core/CaseValidator.cpp:87` 的中文校验消息、`src/io/ImportIssueFactory.cpp` 的 60 条中文码表条目）。core 按 D-021 必须 Qt-free，无法使用 `tr()`；这些字符串由 `ValidationPanel`/导入摘要原样显示。建议在 `docs/DECISIONS.md` 明确“诊断数据文本不走 tr()，UI 框架文本才走”的例外，或让 io 在展示层翻译。属低风险文档缺口（F2），已由 D-039 记录例外（t21）：诊断文本是数据，UI 层按 issue code 本地化。

## 7. 文档与代码一致性

核查中发现并修正的文档偏差：

| 位置 | 原内容 | 修正 |
| --- | --- | --- |
| `docs/STATUS.md:13` | `CASE_IMPORT_SPEC.md（1447 行）` | 实际 1524 行（`ReadAllLines`），已改 |
| `docs/STATUS.md` 构建结果 | Release“通过（M0 基线）”、Debug 仅 t12 时点 | 改为 t13 clean Debug/Release 各 0 warning、43/43，并注明日志路径 |
| `docs/STATUS.md` 当前任务/下一步/最近变更/已知问题 | t13 进行中，未记技术债 | 已更新为 t13 完成、§10.9 技术债、验证证据 |
| `docs/TASKS.md` IMP-06 | “向导与报告页随 t12 完成” | 实际无独立导入向导/报告面板，仅工程节点“导入结果”摘要 + 状态栏计数；已改为 Backlog |
| `docs/CASE_IMPORT_SPEC.md` §10.4 | 规划文件清单（`CaseImportDialog`/`CaseImportReportPanel`/`IssueListView`/`TrDimensional` 等） | 已加实现偏差说明：实际为 `InspectorPanel` + `DomainEditor`/`FieldEditor`/`SolverEditor` 堆叠页，文件清单以 ARCHITECTURE/CMake 为准 |

已核对且一致的关键文档声明：

- `docs/STATUS.md` 测试构成（core 17 + io 19 + render 2 + UI 5 = 43）与实际 CTest 列表一致。
- `docs/ARCHITECTURE.md:442` 明确“3D 模式下 z=1 只给灰色提示、不阻断”，与实测导入 cavity 后 `CASE-MESH-006` 警告一致；`docs/CASE_IMPORT_SPEC.md:429` 规定 2D 只作建议（`IMPORT-DOM-001`），导入不自动改 `DomainMode`，代码行为与规格一致（非缺陷）。
- `docs/DECISIONS.md` D-022/D-033 的“禁止导出到源目录、非空目标需确认、`.bak` 备份、临时目录原子替换”与 `CaseExporter`/`ExportController` 行为一致（测试覆盖）。
- `docs/ARCHITECTURE.md` §5.4/§5.5/§5.6 描述的 Inspector 路由、只读降级、预览同源，与截图及走查一致。

## 8. 问题清单

| ID | 严重度 | 位置 | 问题 | 要求修复 | 状态 |
| --- | --- | --- | --- | --- | --- |
| F1 | 中 | `src/ui/MainWindow.cpp:1`（633 行）、`SolverEditor.cpp:1`（601）、`FieldEditor.cpp:1`（520）、`FieldBoundaryTable.cpp:1`（456）、`CaseDocumentController.cpp:1`（439）、`ExportPreviewDialog.cpp:1`（409）、`src/core/CaseValidator.cpp:1`（402） | 超过 D-021 第 4 条 400 行上限，且提交时未按规则先记录拆分方案 | 已按 `docs/CASE_IMPORT_SPEC.md` §10.9 执行拆分：t19 拆 6 个 UI 文件、t20 拆 `CaseValidator`；全部文件 ≤400 行 | 已解决（t19/t20，43/43 复核） |
| F2 | 低 | `src/core/CaseValidator.cpp:87` 等 core 51 行、`src/io/ImportIssueFactory.cpp` 等 io 75 行 | 用户可见中文诊断字符串未走 `tr()` | 已由 D-039 声明例外（t21）：诊断文本是数据，UI 按 issue code 本地化；不阻塞功能 | 已解决（D-039） |
| F3 | 低 | `docs/TASKS.md` IMP-06（已修） | 文档声称导入向导/报告页已完成，实际只有摘要 | 已完成文档修正；向导与报告面板列入 Backlog | 已修复 |
| F4 | 信息 | `docs/STATUS.md:13` 等（已修） | 规格行数与 Release 状态过期 | 已按实测更新 | 已修复 |

无需修复的观察：

- 导入 2D cavity 后仍为 3D 模式并提示 `CASE-MESH-006`：规格明确“仅建议、不自动改”，行为正确。
- `FieldEditor` 的 internalField/BC 表需要滚动查看：面板内已用 `QScrollArea`（`src/ui/FieldEditor.cpp:63-65`），属布局空间限制。
- `ExportPreviewDialog` 文件列表在阻断态仍显示计划内容、导出按钮置灰：属预览信息展示，不误导执行。

## 9. 已知限制（非本次缺陷，已在 STATUS 记录）

- `VtkViewportWidget` 无面高亮 API（M3-04），树选中只联动 Inspector。
- transport（transportModel/nu）不写入 `.ofgproj`，关闭工程后编辑值不保留（D-037 缺口）。
- `fvSchemes`/`fvSolution` 只读透传；导出预览无逐行 diff。
- `DomainBox::unit()` 无 UI 编辑。
- 独立 patch 编辑页未做（边界页为只读详情）。

## 10. 证据文件

- 构建日志：`build/verify-logs/debug-clean-final.log`（最终 clean Debug）、`debug-build.log`（首轮，含已修复的 6 条警告）、`debug-rebuild.log`（修复后增量）、`release-build.log`
- 截图：`build/verify-logs/smoke-default.png`、`smoke-cavity-tree.png`、`smoke-cavity-domain.png`、`smoke-cavity-field-U.png`、`smoke-cavity-solver.png`、`smoke-cavity-solver-transport.png`、`smoke-export-preview.png`、`smoke-export-blocked.png`、`smoke-release-cavity.png`
- 端到端走查：`build/verify-logs/walkthrough.log`、`build/verify-logs/walkthrough/`（验证专用 CMake 工程 + `export-cavity/` 导出结果）
- 测试清单：`ctest --test-dir build/windows-mingw-debug -N`（43 项）

> 走查程序与导出结果只存在于 `build/`（已被 `.gitignore` 忽略），不进入产品源码、不修改 `OpenFOAM-v2206/`。
