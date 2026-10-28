# 开发任务队列

状态标记：

- `[ ]` 未开始
- `[~]` 进行中
- `[x]` 已完成
- `[!]` 阻塞

每个任务必须在开始前补充或确认验收标准。任务完成后同步更新本文件和 `docs/STATUS.md`。

---

## M0 技术基线

### [x] M0-01 Qt/VTK 最小应用

目标：建立可运行的 Qt 6 + VTK 9.6 CMake 工程。

范围：

- 根 `CMakeLists.txt`。
- `CMakePresets.json`。
- `src/app/main.cpp`。
- `src/ui/MainWindow.h/.cpp`。
- `src/render/VtkViewportWidget.h/.cpp`。
- 一个基础 `QVTKOpenGLNativeWidget`。
- 菜单栏、工具栏、状态栏、左侧占位面板。

非目标：

- 工程保存。
- OpenFOAM writer。
- 真实网格。
- 复杂菜单。

验收：

- Debug 构建通过。
- 应用启动。
- 三维视口可旋转、平移、缩放。
- 关闭应用无崩溃。
- 不依赖 OpenFOAM。

### [x] M0-02 六面长方体预览

目标：在 VTK 视口中显示六个可独立着色的长方体面。

验收：

- 六个面颜色不同。
- 有清晰边线。
- 有坐标轴。
- 有适配视图入口。
- 面 actor 可通过稳定 FaceId 识别。

### [x] M0-03 构建与部署说明

目标：确认开发构建和运行时依赖可复现。

验收：

- 记录 CMake、Ninja、MinGW、Qt、VTK 的完整命令。
- 记录 PATH 要求。
- 使用干净环境或最小 PATH 验证启动。
- 记录 Release 和 `windeployqt` 初步流程。

### [x] M0-04 初始化版本控制

目标：为持续开发提供可回滚基线。

验收：

- 初始化 Git 仓库。
- `.gitignore` 排除 `build/`、IDE 用户文件和运行生成物。
- 明确 `OpenFOAM-v2206/` 的处理策略。
- 首次提交包含文档、源码骨架和构建配置。

---

## M1 工程模型与持久化

### [ ] M1-01 CaseProject 与 DomainBox 模型

验收：

- 支持 min/max、单位、2D/3D 模式。
- 支持有效性校验。
- 包含单元测试。

### [ ] M1-02 MeshSettings 模型

验收：

- 支持 nx/ny/nz。
- 支持总网格数估算。
- 包含非法网格数测试。

### [ ] M1-03 BoundaryPatch 与 FaceId 模型

验收：

- 六个面有稳定 FaceId。
- patch 支持稳定 ID、名称、类型、颜色和面集合。
- 支持面唯一归属校验。
- 包含名称重复和面冲突测试。

### [ ] M1-04 `.ofgproj` JSON 保存/加载

验收：

- 工程数据可保存并重新加载。
- schemaVersion 存在。
- 损坏 JSON 有结构化错误。
- 包含 JSON 往返测试。

### [ ] M1-05 DocumentController 与 dirty 状态

验收：

- 新建、加载、保存后状态正确。
- 模型变更后进入 Modified。
- 保存成功后清除 dirty。
- 关闭修改工程时提示。

### [ ] M1-06 新建/打开/保存 UI

验收：

- 菜单和工具栏动作可用。
- 使用 Qt 文件对话框。
- 错误通过状态栏或校验面板显示。
- 不把业务逻辑写入动作槽函数。

---

## M2 计算域编辑与 VTK 联动

### [~] M2-01 DomainInspector

t9 已交付：minX/maxX 等六值与 2D/3D 模式编辑、非法值提示且不写模型、修改进入模型并标记 dirty。
未含：`DomainBox::unit()` 的单位编辑（核心模型已支持，UI 排入 Backlog）。

验收：

- 可编辑 minX/minY/minZ、maxX/maxY/maxZ。
- 可编辑单位和 2D/3D 模式。
- 非法值显示错误。
- 修改进入模型并标记 dirty。

### [x] M2-02 MeshInspector

t9 交付 `DomainEditor` 的网格数行：编辑 nx/ny/nz、显示尺寸/体积/单元总数、2D 模式自动约束厚度方向为 1、非法值阻止写模型（因此也无法导出）。

验收：

- 可编辑 nx/ny/nz。
- 显示总网格数。
- 2D 模式自动约束厚度方向为 1。
- 非法值阻止导出。

### [x] M2-03 计算域场景同步

t9 交付：`VtkViewportWidget::setDocumentDomain()` + `render::DomainBoundsAdapter`；模型变化重建六面 actor，网格数变化不重建、bounds 与相机保持不变；`ofgui_render_domain_tests` 与 `ofgui_ui_domain_tests` 覆盖。

验收：

- 模型尺寸变化后六个面几何更新。
- 视图适配不会改变模型。
- 无内存泄漏或重复 actor 累积。
- 有场景更新 smoke test。

### [ ] M2-04 视图控制和坐标轴

验收：

- 等轴测、前、后、左、右、上、下视图。
- 透视/正交切换。
- 适配视图。
- 方向标记正确。

### [~] M2-05 2D 模式规则

t9 已交付：2D 强制厚度方向 1 层、z 控件禁用；前后 patch 自动建议 `empty` 未做。

验收：

- 厚度方向可明确指定。
- 网格数为 1。
- 对应前后面自动建议为 `empty`。
- 规则可通过测试验证。

---

## M3 边界编辑与拾取

### [~] M3-01 工程树边界节点

t8 已交付：树中“边界（六面）”下六个面节点显示 `FaceId` 与所属 patch（`xMin → inlet (patch)` / “未分配”），节点身份为稳定 `FaceId`，选中经 `ProjectNodeRef` 驱动 Inspector，重建后按 ID 恢复选择。
未含：独立的 patch 列表节点与重命名入口（随 M3-02）。

验收：

- 显示所有 patch。
- 显示面归属。
- 选中节点更新属性面板。
- 节点刷新不丢失选择。

### [ ] M3-02 BoundaryInspector

验收：

- 支持编辑名称。
- 支持 patch 类型。
- 支持颜色和可见性。
- 校验通过后才提交模型。
- 重名和不安全名称显示错误。

### [ ] M3-03 六面 actor 到 patch ID 映射

验收：

- FaceId 到 patch ID 的映射稳定。
- 重命名 patch 不影响映射。
- 有映射测试。

### [ ] M3-04 树到三维高亮

验收：

- 点击树节点后面高亮。
- 切换选择时清除旧高亮。
- 多面 patch 的所有面同时高亮。

### [ ] M3-05 三维拾取到树

验收：

- 点击面后树选中对应 patch。
- 点击空白处清除选择或保持明确策略。
- 拾取不会修改业务模型。

### [ ] M3-06 patch 视觉样式

验收：

- 类型变化反映在颜色/标签/图例中。
- 可见性切换生效。
- `empty` 与其他类型有可区分表达。

---

## M4 场、边界条件与文件生成

### [x] M4-01 FieldDefinition 模型

t3 交付 `core::FieldDefinition`（名称、`FieldClass`、7 维 dimensions、internalField、按 patchId 的 BC 表）与 `CaseValidator` 的 FIELD 规则；t10 交付 GUI 回填与新增/删除 p、U 模板。

验收：

- 支持 p、U。
- 支持 dimensions、internalField。
- 支持每个 patch 的边界条件。
- 包含缺失 BC 的校验测试。

### [~] M4-02 BoundaryCondition 编辑器

t10 已交付：`FieldBoundaryTable` 按 patch 逐行编辑类型与 `value` 参数；类型白名单按场类别（标量/向量）提供，白名单外导入类型只读保留；`fixedValue` 缺值、非 uniform 标量/向量被拒绝；内部 `parameters` 除 `value` 外的键原样保留。未含：`inletOutlet` 等类型的多参数按类型展开编辑（当前只读保留）。

验收：

- p 支持 `zeroGradient`、`fixedValue`。
- U 支持 `noSlip`、`fixedValue`、`zeroGradient`。
- 条件参数按类型显示。
- 切换类型时清理不适用参数。

### [ ] M4-03 OpenFOAM FoamFile 头写入器

> 由 IMP-05 覆盖/合并（见「算例识别与前处理（IMP）」小节）。

验收：

- 支持版本、format、class、object。
- 输出 LF。
- 有黄金文件测试。

### [ ] M4-04 blockMeshDict writer

> 由 IMP-05 覆盖/合并。

验收：

- 输出 vertices、blocks、edges、boundary。
- 六个面顶点顺序和 patch 名称正确。
- 有单层 2D 和 3D 黄金文件测试。

### [ ] M4-05 controlDict writer

> 由 IMP-05 覆盖/合并。

验收：

- 支持 application、start/end time、deltaT、write control。
- 有黄金文件测试。

### [ ] M4-06 fvSchemes / fvSolution writer

> 由 IMP-05 覆盖/合并。

验收：

- 生成 icoFoam 所需的基础字典。
- 配置项有合理默认值。
- 有黄金文件测试。

### [ ] M4-07 transportProperties writer

> 由 IMP-05 覆盖/合并。

验收：

- 输出 `nu`。
- 使用正确 FoamFile 头。
- 有黄金文件测试。

### [ ] M4-08 p/U field writers

> 由 IMP-05 覆盖/合并；保真回写规则见 D-019。

验收：

- 输出 dimensions、internalField、boundaryField。
- patch 名称与边界模型一致。
- BC 类型和参数正确。
- 有黄金文件测试。

### [ ] M4-09 导出预览对话框

验收：

- 显示将生成的相对路径列表。
- 可查看每个文件内容。
- 预览与最终导出使用同一 builder。
- 校验错误可定位。

---

## M5 安全导出与首版完成

### [ ] M5-01 OpenFoamCaseExporter

> 由 IMP-05 覆盖/合并。

验收：

- 生成 `0.orig/`、`constant/`、`system/`。
- 输出全部必需文件。
- 失败返回结构化错误。
- 不依赖 OpenFOAM 安装。

### [ ] M5-02 临时目录与原子替换

> 由 IMP-05 覆盖/合并；目标目录保护见 D-022。

验收：

- 先在临时目录生成。
- 成功后替换目标。
- 失败清理临时目录。
- 不破坏已有目标。

### [ ] M5-03 覆盖确认与备份

> 由 IMP-05 覆盖/合并；强制规则见 D-022、D-023。

验收：

- 目标已存在时明确提示。
- 用户确认后才覆盖。
- 自动创建可恢复备份。
- 路径非法或不可写时提前报错。

### [ ] M5-04 示例工程

验收：

- 首次启动可以打开一个长方体示例。
- 示例包含六个 patch、p/U 和基础求解器配置。
- 示例不需要 OpenFOAM 环境。

### [ ] M5-05 设置页

验收：

- 默认导出目录。
- 默认单位。
- 最近打开列表长度等基础配置。
- 设置持久化到用户配置目录。

### [ ] M5-06 UI 完善和 smoke test

验收：

- 高 DPI 布局正常。
- 主要错误有可读提示。
- 所有按钮都有实际行为或明确禁用。
- 完成一次从新建到导出的手动验收。

---

## M6 后续增强

### [ ] M6-01 Undo/Redo

### [ ] M6-02 原始字典高级编辑

### [ ] M6-03 导入已有 case 的基础信息

> 由 IMP-01…IMP-09 覆盖/合并（提前到首版，见下方 IMP 小节）。

### [ ] M6-04 用 VTK 读取并显示 `polyMesh`

### [ ] M6-05 读取时间目录并显示结果场

### [ ] M6-06 增加 simpleFoam 模板

### [ ] M6-07 增加 turbulence 字段和边界条件

### [ ] M6-08 增加 patch 分组

### [ ] M6-09 增加多块网格

### [ ] M6-10 增加深色主题和翻译

---

## 算例识别与前处理（IMP）

> 来源：`docs/CASE_IMPORT_SPEC.md` §13（原样落入），决策依据 `docs/DECISIONS.md` D-017…D-025。
> 本小节把“识别 v2206 算例 → GUI 前处理 → 安全导出”提前为首版能力；与 M1–M6 重叠处已在对应条目下标注“由 IMP-xx 覆盖/合并”。

| ID | 任务 | 依赖 | 产出 | 验收要点 |
| --- | --- | --- | --- | --- |
| IMP-01 | 本规格（t1） | — | `docs/CASE_IMPORT_SPEC.md` | 评审通过 |
| IMP-02 | `core` 模型 + `CaseValidator` | IMP-01 | 规格 §10.2.1 列出的实际文件 + 单测 | 无 Qt 依赖；单测绿；D-021（一文件一职责、≤400 行） |
| IMP-03 | `foam` AST + `FoamLexer`/`FoamParser` | IMP-02 | 实际文件清单（`src/io/Foam*`，规格 §10.2.1）+ 表驱动测试 | 规格 §5.3 全场景通过；错误定位到行+列 |
| IMP-04 | 投影器 + `CaseImportService` + `ImportReport` | IMP-03 | `core/import/*` | 规格 §11.1/§11.2/§11.3 断言通过；D-020（不展开 include/polyMesh） |
| IMP-05 | writers + `ExportPlan` + `AtomicDirectoryWriter` + `OpenFoamCaseExporter` | IMP-04 | `io/*Writer*` | 规格 §11.4/§11.5 通过；D-019/D-022/D-023 |
| IMP-06 | 导入向导 + 报告面板 | IMP-04 | `CaseImportDialog`、`CaseImportReportPanel`、`IssueListView` | 手动 smoke + 截图 |
| IMP-07 | Domain/Mesh/Patch/Field/Solver Inspector | IMP-04、IMP-06 | 规格 §10.4 面板 | 编辑写回模型并触发校验；D-024。t9 已交付计算域/网格页（`DomainEditor`），t10 已交付场页（`FieldEditor`/`FieldBoundaryTable`），t11 已交付求解页（`SolverEditor`）；独立 patch 编辑页仍未做（现有边界页为只读详情） |
| IMP-08 | 校验面板 + 导出预览 | IMP-05、IMP-07 | `ValidationPanel`、`ExportPreviewDialog` | t12 已完成：阻断态置灰并给出中文原因；双击定位；覆盖确认 + `.bak` 备份；transport 接线（D-037）已验证 |
| IMP-09 | 端到端 + 文档收尾 | IMP-08 | fixtures、`docs/STATUS.md`/`TASKS.md` 更新 | G1–G5 全绿；无绝对路径 |

### 与 AgentTeams 任务（t1–t14）的映射

| IMP | AgentTeams 任务 | 说明 |
| --- | --- | --- |
| IMP-01 | t1（已完成） | 规格本身 |
| IMP-02 | t2（骨架）+ t3（`CaseProject` 模型/不变量/FaceId 迁移） | t2 提供 `ofgui_core`/`ofgui_io` 构建边界，t3 落入模型与校验 |
| IMP-03 | t4（`FoamLexer`/`FoamParser` + 位置信息） | 实际落在 `src/io/Foam*`；规格 §10.2 的 `core/foam/*` 是历史计划，偏差见 §10.2.1 |
| IMP-04 | t6（扫描与导入 `0/`、`constant/`、`system/`） | 投影器 + `CaseImportService` + `ImportReport`；t5（`.ofgproj`）为同层前置 |
| IMP-05 | t7（writers + 黄金文件测试） | 导出计划/原子写/导出器在同一纵切；t12 做端到端 |
| IMP-06 | t8（工程树加载算例，已完成：树/打开/保存/dirty/稳定 ID 选中）+ t12 的导入结果摘要 | 树加载已落地；导入报告目前只以工程节点“导入结果”摘要 + 状态栏计数呈现，独立导入向导与报告面板未实现（t13 核查，列入 Backlog） |
| IMP-07 | t9（计算域+VTK）、t10（初始条件）、t11（求解设置，已完成：controlDict 可编辑键 + transport nu + 只读原文 + 导出预览） | 三个 Inspector 按面板拆分 |
| IMP-08 | t12（校验→导出纵向流程，已完成：`ExportController` + 预览对话框 + 校验 dock + 默认 cavity 模板 + 9 个 workflow 测试） | 校验面板与导出预览在 t12 合流 |
| IMP-09 | t13（独立验证，已完成：`docs/VERIFICATION.md`，7 项硬边界/端到端/截图证据；超 400 行技术债见 §10.9）、t14（评审） | 端到端验收与文档收尾 |

依赖自洽检查：`IMP-01 → IMP-02 → IMP-03 → IMP-04 → {IMP-05, IMP-06} → IMP-07 → IMP-08 → IMP-09`，与上表逐行一致；t5（`.ofgproj`）不阻塞 IMP-04 的解析与投影，但阻塞“导入后保存”的完整闭环（`IMP-04 → t5`，见依赖图）。

---

## Backlog

以下功能只记录，不排入首版：

- ~~拆分 t13 发现的 7 个超 400 行文件~~ → **已完成销项**：t19 拆 UI 6 个、t20 拆 `CaseValidator`；2026-10-28 t25 复核 `src/` + `tests/` 无文件超限（最长 395 行），结果见 `docs/CASE_IMPORT_SPEC.md` §10.9.1。
- `.ofgproj` schemaVersion 3：持久化 transport 编辑值（`transportModel`/`nu`）。当前策略（D-040）是不写入 + UI 明示，schemaVersion 2 结构保持不动；升级时沿用既有的“未知段/未知键保留”规则。
- 导出目录最终 `rename` 失败回滚分支缺少自动化注入测试：`src/io/writers/AtomicCaseDirectory.cpp` 的“rename 失败 → 删 temp → backup 回滚（失败时保留备份路径）”路径目前只靠代码评审覆盖，staging 失败路径已有测试（t26 F-03 判定为可接受低风险）。实现建议：在 `AtomicCaseDirectory` 内留一个仅测试可见的 rename 失败注入钩子，或测试时用只读/占用目标制造失败，断言回滚后旧目录内容完整、备份路径出现在错误消息里。
- 2D 平面视图：`DomainMode::TwoD` 已有模型/校验/编辑器支持，但 VTK 视口仍按三维六面显示，缺正交平面视图与“2D 模式下隐藏厚度方向边线”的显示语义。
- 导入向导 `CaseImportDialog` 与独立导入报告面板（`CaseImportReportPanel`/`IssueListView`）；当前仅工程节点摘要 + 状态栏计数。
- STL/STEP 导入。
- snappyHexMesh。
- 任意面拾取。
- 多区域 case。
- 并行求解配置。
- 求解器进程调用。
- 残差监控。
- ParaView 完整过滤器体系。
- 计算域单位（`DomainBox::unit()`）的 UI 编辑：核心模型已支持，t9 编辑器未暴露。
- 面高亮：`VtkViewportWidget::setSelectedFaces()` 尚未实现（M3-04）。
- 云计算和远程作业。
- 账号和项目服务器。

---

## 依赖关系

```text
M0-01
  ├─ M0-02
  ├─ M0-03
  └─ M0-04
M0-01 → M1-01 → M1-02
M1-01 → M1-03
M1-01 + M1-03 → M1-04 → M1-05 → M1-06
M1-01 + M2-01 → M2-03
M1-02 + M2-02 → M2-03
M1-03 + M0-02 → M3-03
M3-03 → M3-04 → M3-05
M1-04 + M4-01 → M4-02
M1-03 + M4-01 → M4-04 → M4-08
M4-03 → M4-04/M4-05/M4-06/M4-07/M4-08
M4-09 + M5-01 → M5-02 → M5-03 → M5-06

IMP-01 → IMP-02 → IMP-03 → IMP-04 → IMP-05
IMP-04 → IMP-06
IMP-05 + IMP-07 → IMP-08 → IMP-09
IMP-02 → IMP-07
IMP-04 → t5（导入后保存闭环）
IMP-05 ⊂ t7；IMP-07 ⊂ t9/t10/t11；IMP-08 ⊂ t12；IMP-09 ⊂ t13/t14
```

---

## Agent 任务派发模板

```text
任务 ID：
目标：
范围：
非目标：
验收标准：
必须运行的测试：
允许修改的目录：
依赖任务：
```

Agent 开始任务前必须检查依赖是否完成。如果依赖未完成，应只实现最小必要前置条件，或者停止并说明阻塞，不得跳过大范围依赖。