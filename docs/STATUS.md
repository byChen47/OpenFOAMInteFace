# 项目状态

> 最后更新：2026-10-28  
> 当前阶段：IMP 算例识别与前处理 + 导出纵向流程已完成；t14 第二轮评审 pass（12 项 findings 全部关闭），首版可交付  
> 当前任务：无活动开发任务（t28 收尾：本文档最终态同步 + Backlog 记账）

## 当前结论

- 项目定位：独立 Qt 6 + VTK 桌面 GUI，不编译、不链接、不嵌入 OpenFOAM。
- M0-01、M0-02、M0-03、M0-04 已完成。
- 新增 `ofgui_core`（纯 C++20，Qt-free）与 `ofgui_io`（链 core + Qt6::Core）两个静态库。
- 领域模型与解析/投影/写出层已落地：`ofgui_core`（19 个模型与校验类、`CaseValidator` 5 个规则族、`FaceAssignmentValidator`）；`ofgui_io`（Foam 词法/语法/AST、7 个投影器、`CaseScanner`/`CaseImporter`、`writers/` 全套字典写出与目录原子导出）。导出在**任何 Error / BlockingError** 时拒绝写盘（t22 F-02）。
- 规格 `docs/CASE_IMPORT_SPEC.md`（1615 行）与决策 D-017…D-041 已记录（D-040：transport 编辑值不持久化、由 UI 明示，schema v3 进 Backlog；D-041：io 文件按域命名，禁止 `Helpers`/`Support`/`Types` 杂物桶）。
- Git 仓库已建立，`main` 保存最新版本，更新使用自动归档分支流程。

## 已完成

### M0-01 Qt/VTK 最小应用

- CMake、Ninja、MinGW 工程。
- Qt Widgets 主窗口。
- 左侧工程树和属性面板。
- 右侧 VTK 视口。
- 菜单、工具栏、状态栏。
- Smoke test 和截图验证。

### M0-02 六面长方体预览

- 六个独立 FaceId 和面 actor。
- 六种独立颜色。
- 长方体边线、坐标轴。
- 适配视图和重置视角。
- 自动化场景测试。

### M0-03 构建与部署

- Debug/Release 构建脚本。
- `windeployqt` 和 PE 依赖闭包部署。
- 最小 PATH 部署验证。
- 完整构建与部署文档。

### M0-04 Git 仓库

- 初始化 `main` 分支。
- 首次基线提交。
- `OpenFOAM-v2206/` 作为本地只读参考，不进入 Git。
- 自动归档旧 `main` 的脚本。
- 新提交写入 `main` 的更新脚本。
- Git 分支与更新流程文档。

### 算例识别与前处理（IMP，2026-10-27 起）

> 以下每条记录的是**其完成时点**的测试数；当前最新全量结果（45/45）见「构建与部署结果」。

- t1：`docs/CASE_IMPORT_SPEC.md` 规格（四级支持、patch 名单优先级、字节保真回写、目录原子导出）。
- t2：`src/core` / `src/io` 模块骨架与 CMake 边界；模块内登记测试，禁止 GLOB。
- t3：`src/core` 19 个模型/校验类一文件一职责；`FaceId` 迁入 core，`src/render/FaceId.h` 改为兼容别名。
- t4：`src/io` 解析层（lexer/parser/AST/dictionary + 诊断与位置信息），23/23 测试全绿，真实语料 cavity/pipeCyclic/tutorials 250 个字典解析通过。
- t5：`.ofgproj` schemaVersion 1 持久化（16 个 io 文件，未知字段保留、原子替换），25/25 测试全绿；D-025 的 source 段缺 core 模型，已拆出 t16/t17。
- t16：core `CaseSourceInfo` + `CaseProject.source`（D-031，来源为事实记录，不阻断导出），26/26 测试全绿。
- t6：`src/io` 算例扫描与导入（44 个文件：CaseScanner/CaseImporter/7 个投影器/ImportReport/Sha256Hasher/CaseSourceRecorder），cavity 全链路断言通过、pitzDaily 多块 Blocking，30/30 测试全绿；source 已按 D-031 填入 CaseProject。
- t7：`src/io/writers/` 28 个文件（FoamLayout/NumberFormat/DictionaryEmitter + 7 个 writer + ExportPlan/AtomicCaseDirectory/CaseExporter），cavity 六个文件逐字节复现 v2206 快照（fvSolution 按规格展开 `$p;`），io 15/15 套件通过；D-033。t8 落盘后全量构建与 36/36 测试已复核。
- t17：`.ofgproj` schemaVersion 1→2，新增 `source` 段（键名按 §6.7，数据来自 `core::CaseSourceInfo`）；读 1..2、v1 内存迁移不回写、保存恒写 2、>2 拒读；source 段恒写出（新建为空来源），未知键保留；D-034（文件版本归 io，core schemaVersion 语义为模型修订号）。36/36 全绿。
- t15：D-017…D-025、IMP-01…IMP-09、ARCHITECTURE §7.2 导入例外落档；t3 追加 D-026…D-028，t4 追加 D-029。
- t8：工程树改为模型驱动（`ProjectTreeModel`/`ProjectTreeView`/`ProjectNodeRef`），根节点“工程 · <显示名>”下挂计算域/网格/边界（六面）→ 各 patch/场（0/）→ 各 patch BC/求解器设置/导出预览；`CaseDocumentController` 统一新建、打开算例、打开/保存 `.ofgproj`、dirty 状态与校验；MainWindow 增加文件菜单、保存/另存为、关闭 dirty 保护与窗口 `[*]` 标记；Inspector 按稳定 ID（NodeKind + FaceId/fieldId/patchId）取值，新增 `NodeDetailHtml`/`NodeDetailSections`/`NodeDetailBoundaryDetail` 拆分格式化逻辑。ARCHITECTURE §5.3 记录树/选中设计。
- t9：`DomainEditor`（`InspectorPanel` 的堆叠页）编辑 min/max、cells 与 2D/3D，非法值只提示不写模型；`CaseDocumentController` 默认新建 1 m 单位立方体；render 新增 `DomainBoundsAdapter`，`VtkViewportWidget::setDocumentDomain()`/`sceneBounds()`/`sceneRevision()` 在 bounds 变化时重建六面 actor、只改网格数时保持不变；打开算例/新建/关闭后 `fitView()`；新增测试目标 `ofgui_render_domain_tests` 与 `ofgui_ui_domain_tests`（7 槽），全量 40/40 通过。ARCHITECTURE §5.4/§6.4/§10 更新。
- t18：导出补全：`src/io/writers` 新增 `ExportAction`/`ExportSkipRules`/`SourceCaseWalker`/`CaseMirrorPlanner`/`CaseMirror`/`ExportManifest` 等 15 个文件；`CaseExporter` 支持 Written/Copied/Mirrored/skipped 四类动作，临时目录内先写字典→再镜像→最后清单→目录级 rename；polyMesh 逐字节镜像、`.gz` 保持；新增 `EXP-008/009/010`；全量 40/40（io 19/19）；D-035。
- t10：场编辑器：`FieldEditor`（dimensions 只读、internalField uniform 编辑/非 uniform 原文只读、逐 patch BC 表格、0/ 预览）、`FieldBoundaryTable`（patchId 稳定 ID + 类型白名单，白名单外只读）、`UniformValueEditor`/`UniformValueText`、`ExportPreviewPanel`；Inspector 增加 Field/FieldBoundary 路由；新增 `ofgui_ui_field_tests`（8 槽），全量 41/41 通过；cavity 的 p/U 与逐 patch BC 回填正确，预览与 `io::FieldFileWriter` 一致。ARCHITECTURE §5.5/§10.4、D-036。
- t11：求解设置编辑器 `SolverEditor`（四页签 controlDict/constant/只读原文/导出预览）：可编辑 application/startTime/endTime/deltaT/writeInterval 与 transportModel/nu；只读降级 startFrom/stopAt/writeControl/simulationType/fvSchemes/fvSolution 并说明原因；非法输入回滚不写模型；nu 维度化时原文回写；预览直调 `ControlDictWriter`/`TransportPropertiesWriter`；新增 `ofgui_ui_solver_tests`（9 槽），全量 42/42；D-037。
- t12：校验/导出纵向流程：`ExportController`（预览与写盘唯一入口，校验模型 + 校验计划字典 + 原子导出 + 回读 `.ofgport-export.json`）、`ExportPreviewDialog`（文件树/动作/文本预览/覆盖确认）、`ValidationPanel`（级别着色 + 双击定位）、`ValidationNavigator`（`targetField`/对象 ID → `ProjectNodeRef`）、`ExportDictionaryCheck`（`EXPORT-DICT-001/002`）、`DefaultCaseTemplate`（新建即 cavity 模板，导出开箱可用）；`MainWindow` 增加“导出算例”（Ctrl+E）与底部校验 dock，导出按钮受阻断问题控制；D-037 的 transport 接线由 `ExportController::makeOptions()` 闭合；新增 `ofgui_ui_workflow_tests`（10 槽），全量 43/43（当时）；D-038。
- t13：独立集成验证完成（`docs/VERIFICATION.md`）：clean Debug/Release 各 0 warning、ctest 43/43（当时）；smoke + 7 张截图核对；`icoFoam/cavity/cavity` 教程导入→改域/初始条件/求解→导出→再导入全链路 PASS（manifest 9 文件、ASCII+LF、再导入值一致）；AGENTS 硬边界逐项有证据。发现 7 个 >400 行文件（D-021 第 4 条），拆分方案已记录于 `docs/CASE_IMPORT_SPEC.md` §10.9，作为 t14 前技术债。
- 规格与文档对应的实现任务 t5–t18 见 `docs/TASKS.md`。

## Git 分支规则

- `main`：最新版本。
- `archive/<时间>-<旧main短SHA>`：每次更新前的旧版本。
- 归档分支只读。
- 每次更新运行 `scripts\Commit-Update.ps1`。
- 固定触发词 `提交到github` 会自动执行归档、提交、推送和远程校验。

## 构建与部署结果

- Debug 构建：通过（2026-10-28 t26 复核：`Build-Project.ps1 -Configuration Debug` 退出码 0，0 error / 0 warning，全树 181 个目标；t13 的全新目录复验日志 `build/verify-logs/debug-clean-final.log`）。
- 全量 CTest：`45/45` 通过（2026-10-28 t26 复核：`core` 17 + `ofgui_io` 21 + `render` 2 + `ui` 5；t13 时为 43/43，t22 新增 io 码表套件、t23/t10 等新增 UI 槽后增至 45）。渲染场景测试目标在 t23 按 §10.7 由 `ofgui_scene_tests` 更名为 `ofgui_render_scene_tests`。
- Release 构建：通过（2026-10-28，t13：全新目录构建 + 43/43 测试，0 warning，日志 `build/verify-logs/release-build.log`；Release 版 smoke 截图 `build/verify-logs/smoke-release-cavity.png`）。t22–t27 的修复只在 Debug 上复验，发版前需重跑一次 Release 构建 + CTest。
- 部署目录：`dist\OpenFOAMGUI\`
- 最小 PATH smoke test：退出码 0。
- 部署截图：`dist\OpenFOAMGUI\deployment-smoke.png`

## 当前环境

- Qt：`6.11.2`
- CMake：`3.30.5`
- MinGW：`13.1.0`
- VTK：`9.6.2`
- Ninja：`1.12.1`
- Git：`2.55.0.windows.3`
- GitHub：`https://github.com/byChen47/OpenFOAMInteFace.git`

## 下一步

1. 首版功能已收口：t14 第二轮评审 pass（F-01…F-12 全部关闭）、Debug 0 warning + 45/45；发布前只剩 Release 重跑与部署复验（`Deploy-Release.ps1`），见「构建与部署结果」。
2. 技术债已清零：t19/t20 拆分完成、t25 逐文件复核（无文件 >400 行），t13 技术债关闭。
3. Backlog 保留增强项：schema v3 持久化 transport（D-040）、导入向导/导入报告面板、面高亮 M3-04、2D 平面视图、导出目录最终 rename 失败回滚的注入测试。
4. M0 遗留修复（控制台子系统等）待用户确认后插入。

## 已知问题

- Vulkan headers 未安装，但当前不使用 Vulkan。
- windeployqt 提示缺少 DXC DLL，但当前不使用 Direct3D 12。
- 并行成员在 CMakeLists 中登记尚未落盘的测试源文件曾导致整树 configure 失败（t4 期间发生，已恢复）；后续要求“先落盘再登记”。
- 测试源文件必须 UTF-8：`tests/ui/DomainEditorTest.cpp` 曾以 GBK 中文导致全树构建失败（t18 期间发现，已由 ui-engineer 修复）。
- t8/t9 遗留：`VtkViewportWidget` 仍没有面高亮 API（`setSelectedFaces()`，M3-04），树选中只联动 Inspector；计算域已按真实 `DomainBox` 显示（t9），但 patch 颜色/可见性尚未接到场景。
- t9 范围外：`DomainBox::unit()` 的 UI 编辑未做（Backlog），当前编辑器只改 min/max/cells/2D-3D。
- M0 遗留待修：发布 exe 为控制台子系统（缺 WIN32_EXECUTABLE）、部署不清理 dist 且截图写入发布目录、依赖闭包对缺失 DLL 静默跳过、面透明度 0.92 过透。
- t11 边界：`startFrom`/`stopAt`/`writeControl`/`fvSchemes`/`fvSolution`/`simulationType` 尚无领域模型，UI 只读展示导入原值；导出入口已在 t12 接入，`transportSettings()` 经 `ExportController::makeOptions()` 进入 `io::CaseExportOptions.transport`。
- t12 边界：transport（transportModel/nu）仍不写入 `.ofgproj`（D-037 的模型缺口），关闭工程后编辑值不保留；按 D-040 本轮采取“不持久化 + UI 明示”策略（`SolverEditor` 常驻提示、保存/重开时重复说明），schema v3 持久化列入 Backlog；`fvSchemes`/`fvSolution` 以导入原文透传，无结构化编辑；导出预览不做逐行 diff。
- 2D 模式（t28 核对）：模型（`DomainMode::TwoD`）、校验（z 单层 `CASE-MESH`、z 向 patch 必须 `empty` 的 `CASE-PATCH-008` BlockingError）与 `DomainEditor` 均已支持；缺的是**渲染侧 2D 平面视图**——VTK 一律显示三维六面，且导入只给 `suggestMode()` 提示、不自动切换模式。2D 的完整可视化体验列入 Backlog。
- t13 技术债（**已关闭**，t25 复核）：7 个源文件曾超过 D-021 的 400 行上限（`src/ui/MainWindow.cpp` 633、`SolverEditor.cpp` 601、`FieldEditor.cpp` 520、`FieldBoundaryTable.cpp` 456、`CaseDocumentController.cpp` 439、`ExportPreviewDialog.cpp` 409、`src/core/CaseValidator.cpp` 402）；已由 t19（UI 6 个）与 t20（`CaseValidator`）全部拆分，拆分结果见 `docs/CASE_IMPORT_SPEC.md` §10.9.1。2026-10-28 t25 逐文件复核 `src/` + `tests/`：无文件超过 400 行（最长 395 行 `tests/io/ProjectJsonRepositoryErrorsTest.cpp`），t26 复核全量 45/45 通过。
- t25 文档口径（F-04）：规格 §10.2–§10.6 是 t1 设计时的**计划稿**，不是当前源码清单；实际文件登记册以 `docs/ARCHITECTURE.md` §2.1/§5.x 与各模块 `CMakeLists.txt` 为准，计划 → 实际的偏差见规格 §10.2.1。评审不要再把 §10.2–§10.6 当逐文件比对依据。
- t21 边界（D-039）：core/io 有 **147 行**中文诊断文本（`src/core` 7 个文件 51 行、`src/io/ImportIssueFactory.cpp` 96 行；t28 实测，t21 记录时为 126 行，t22 把码表扩充到 71 个错误码后增加），它们 Qt-free 且是**数据文本**，按规则不走 `tr()`；当前 `ValidationPanel`/`NodeDetailHtml`/`ExportPreviewDialog` 直接显示 `issue.message()`，尚无“码 → 文案”映射表，因此在补映射之前不能切换英文界面（`docs/CASE_IMPORT_SPEC.md` §8.7）。

## 最近变更

> 各条为当时快照（测试数为其时点值）；当前最新全量结果为 Debug 0 warning + `45/45`。

- 2026-10-26：初始化 Git 仓库并建立 M0 基线。
- 2026-10-26：建立旧 main 归档分支和最新 main 更新流程。
- 2026-10-26：配置 GitHub origin 并首次推送 main。
- 2026-10-26：新增“提交到github”自动发布规则和一键发布脚本。
- 2026-10-26：一键发布脚本通过临时裸仓库验证。
- 2026-10-27：建立 AgentTeams 团队与 t1–t15 任务图；t1 规格、t2 模块骨架、t3 领域模型、t4 解析层、t15 决策落档完成。
- 2026-10-27：D-017…D-030 记录；`.gitignore` 增加 `/.agent-teams/`、`/.dsh-meow/`。
- 2026-10-27：t5 工程 JSON 持久化完成；新增 t16（CaseSourceInfo）与 t17（schemaVersion 2 + source 迁移）补 D-025。
- 2026-10-27：t6 算例导入完成（44 个文件，30/30），cavity 全链路通过、pitzDaily 正确报多块 Blocking。
- 2026-10-27：t7 字典生成器完成，cavity 6/7 文件逐字节复现（fvSolution 按规格展开 `$p;`），io 15/15。
- 2026-10-28：t8 工程树/算例加载/稳定 ID 选中完成；新增 `ofgui_ui_smoke_tests`，全量 36/36 通过，cavity 教程截图确认树结构与文件一致。
- 2026-10-28：t17 `.ofgproj` schemaVersion 2（source 段 + v1 内存迁移）完成，D-034；全量 36/36。
- 2026-10-28：t9 计算域/网格编辑器与 VTK 场景联动完成；新增 `DomainBoundsAdapter`、`DomainEditor`、`ofgui_render_domain_tests`、`ofgui_ui_domain_tests`，全量 40/40；cavity 截图确认视口显示真实扁平计算域（0.1×0.1×0.01）与编辑页回填 20/20/1。
- 2026-10-28：t18 导出补全（polyMesh 镜像、其它场/constant 保真复制、skip 名单、导出清单）完成，D-035；全量 40/40，io 19/19。
- 2026-10-28：t10 场编辑器完成：dimensions 只读、internalField uniform 编辑、逐 patch BC 表格（未知类型只读）、0/ 预览；新增 `ofgui_ui_field_tests` 与 D-036，全量 41/41；cavity 的 U/p 与 movingWall fixedValue 截图核对通过。
- 2026-10-28：t11 求解设置编辑器完成：`SolverEditor`（controlDict 可编辑键、transportModel/nu、只读原文、导出预览）；非法输入回滚不写模型；新增 `ofgui_ui_solver_tests` 与 D-037，全量 42/42；cavity 的 icoFoam/0.5/0.005/20 与 nu 0.01 截图核对通过。
- 2026-10-28：t12 校验/导出纵向流程完成：`ExportController`/`ExportPreviewDialog`/`ValidationPanel`/`ValidationNavigator`/`ExportDictionaryCheck`/`DefaultCaseTemplate`；新建工程即 cavity 模板，导出计划 9 个文件（新写 7/复制 1/镜像 1）；阻断问题在预览阶段即禁用导出；新增 `ofgui_ui_workflow_tests` 与 D-038，全量 43/43；`build/t12-export-preview.png`、`build/t12-export-blocked.png` 截图核对。
- 2026-10-28：t13 独立集成验证完成：clean Debug/Release 0 warning + 43/43；7 张 smoke 截图；cavity 导入→编辑→导出→再导入 PASS；修复 6 处 `-Wunused-result`（`main.cpp`、`ExportWorkflowTest.cpp`）并复核；记录 §10.9 超 400 行文件拆分方案，`docs/VERIFICATION.md` 落档。
- 2026-10-28：t13 后新增 t19（UI 6 文件拆分）、t20（CaseValidator 拆分）、t21（D-039 Qt-free 诊断例外）并派发；t14 评审与拆分并行。
- 2026-10-28：t21 记录 D-039（Qt-free 层诊断文本是数据、不走 `tr()`，本地化由 UI 按 `code` 映射），同步规格 §6.6/§7/§8.7/§10.1/§10.8 与本节已知问题；仅文档变更。
- 2026-10-28：t19 UI 侧 6 个超 400 行文件拆分完成（MainWindow/SolverEditor/FieldEditor/FieldBoundaryTable/CaseDocumentController/ExportPreviewDialog），拆为 22 个实现文件与 3 个内部头文件，公共 API 与行为不变；新文件显式登记根 `CMakeLists.txt`，规格 §10.9.1 记录结果；Debug clean 构建 0 warning、全量 43/43。`src/core/CaseValidator.cpp` 留待 t20。
- 2026-10-28：t20 `CaseValidator.cpp` 402 行拆为 16 行入口 + 5 个规则族 + 内部支持头，规则/消息/严重度逐字保留，ARCHITECTURE §4.7 与 spec §10.2 同步。
- 2026-10-28：t19+t20 后全量复核：`Build-Project.ps1 -Configuration Debug` 退出码 0、0 warning、ctest 43/43 通过；F1 关闭（VERIFICATION.md 已更新）。
- 2026-10-28：t25 修复 t14 评审文档问题：规格 §10.2–§10.6 标注为历史计划并新增 §10.2.1「计划 → 实际偏差」表（登记册改为 `ARCHITECTURE` §2.1/§5.x + 模块 `CMakeLists.txt`），§10.9/§10.9.1 回填 t20 拆分结果与 t25 复核数据，§10.7 第 5 条与 §10.8 第 6 项改指新登记册；STATUS 规格行数与 field 测试槽数按实测修正、t13 技术债标记关闭；ARCHITECTURE §2.1 按当前 `CMakeLists.txt` 更新（core/io 子目录 + 根文件登记 render/ui/tests）；新增 D-040（transport 不持久化 + UI 明示，schema v3 进 Backlog）。仅文档变更。
- 2026-10-28：t23 修复 t14 评审 UI 项。F-05（策略 B，D-040）：`SolverEditor` constant 页常驻提示「transportModel/nu 不随 .ofgproj 保存，重开按源算例重读」，并在保存/另存为/重开工程的状态栏消息重复说明，不实现持久化。F-06（ui）：`FieldBoundaryTableTypes.cpp` 按域更名为 `FieldBoundaryTableTypeRules.cpp` 并更新根 `CMakeLists.txt`。F-12：`CaseDocumentController` 构造改用类上下文 `tr()`，`ExportController` 自由函数统一 `QCoreApplication::translate("ExportController", …)`；渲染场景测试目标 `ofgui_scene_tests` → `ofgui_render_scene_tests`（同步规格与本节）。新增 `SolverEditorTest::transportPersistenceLimitIsExplained` 回归槽；Debug 0 warning，全量 45/45 通过；截图 `build/t23-main.png`、`build/t23-export-preview.png`、`build/t23-solver-transport.png`。
- 2026-10-28：t22 修复 t14 评审 io 正确性（F-01/F-02/F-03/F-07/F-08/F-09）：`ImportCodes.h` 增 `kCodeCount=71` + `kAllCodes` 全码表，`ImportIssueFactory` 补齐 71 行码表并用 `static_assert(everyCodeHasExactlyOneRow())` 编译期校验，severity 回归规格（`TURB-001`/`EXP-005`→Info 等）；`ExportPlanBuilder` 对调用方 `fvSchemes`/`fvSolution` 解析失败时注入诊断并原样保留文本，`CaseExporter` 改为**任何 Error/BlockingError 即拒绝写盘**；`AtomicCaseDirectory`/`AtomicFileWriter` 的 rename 失败回滚（backup→target，回滚失败保留备份路径，旧 `.ofgproj` 不再可能丢失）；删除 `replaceExisting` 死分支；`FoamParseOptions::maxNestingDepth=64` + `IMPORT-PARSE-009` 防栈溢出；新增 `ImportCodeCatalogTest`（5 槽）与 `ExportSafetyTest`（9 槽）。同轮修掉 ui 侧默认字典缺 FoamFile 头的回归。io 21/21、全量 45/45、0 warning。
- 2026-10-28：t24 修复 F-06（core 命名）：`src/core/CaseValidatorSupport.h` → `CaseValidatorMessages.h`，5 个实现 include、`src/core/CMakeLists.txt`、ARCHITECTURE §4.7、规格 §10.2 同步；`src/core` 中 `Support`/`Helpers`/`Types` 后缀文件数为 0；校验规则、severity、code、文案零改动；构建 0 warning、45/45。
- 2026-10-28：t27 修复 F-06（io 命名）：`CaseScannerHelpers` 拆为 `CaseDirectoryProbe` + `CaseNameRules`，`CaseImporterHelpers` 拆为 `ImportDictionaryLoader` + `ImportReportBookkeeping`（均独立文件、CMake 显式登记、≤400 行）；新增 D-041 固化“按域命名、`Helpers`/`Support`/`Types` 零容忍”，测试辅助头 `tests/io/*TestSupport.h` 例外；仅搬文件与 include，诊断码/文案零变更。
- 2026-10-28：t26 t14 第二轮复核 **pass**：12 项 findings（F-01…F-12）全部关闭，独立 Debug 构建 0 warning、ctest 45/45，cavity 导入→编辑→导出→再导入行为抽查与 AGENTS 硬边界复核通过，无 must-fix 残留；仅剩“rename 失败回滚分支无注入测试”这一低风险残余，已记入 Backlog。
- 2026-10-28：t28 收尾文档同步：头部当前阶段/当前任务改为“首版可交付”，构建与测试结果统一为 Debug 0 warning + 45/45（并注明 Release 需发版前重跑），下一步改为发布与 Backlog 清单，已知问题补 2D 渲染缺口、修正 D-039 中文行数为实测 147 行；Backlog 新增“导出目录最终 rename 失败回滚缺注入测试”与“2D 平面视图”。仅文档变更。
