# OpenFOAM GUI 架构设计

## 1. 架构目标

架构的首要目标不是一次性实现所有功能，而是保证后续 Agent 可以在不破坏现有代码的前提下持续增加功能。

必须满足：

- GUI 不依赖 OpenFOAM 编译环境。
- 核心业务逻辑可以脱离 Qt Widgets 和 VTK 测试。
- VTK 只作为渲染与拾取层。
- OpenFOAM 文件生成逻辑与窗口控件解耦。
- 每个功能都有明确的模型、服务和测试入口。
- 数据模型可持久化、可校验、可迁移。
- 后续可以增加求解器模板、更多边界条件和网格读取，而不需要重写主界面。

---

## 2. 总体分层

```text
┌──────────────────────────────────────────────────────────────┐
│ app                                                          │
│ main.cpp、ApplicationContext、依赖装配、启动错误处理        │
└──────────────────────────────┬───────────────────────────────┘
                               │
┌──────────────────────────────▼───────────────────────────────┐
│ ui                                                           │
│ MainWindow、ProjectTree、Inspector、Dialog、ValidationPanel │
└───────────────┬──────────────────────────────┬───────────────┘
                │                              │
┌───────────────▼──────────────┐  ┌────────────▼───────────────┐
│ application                  │  │ render                     │
│ DocumentController           │  │ VtkViewportWidget          │
│ ExportService                │  │ CaseScene                  │
│ SelectionService             │  │ DomainActorBuilder         │
│ Command/Undo（后续）         │  │ PatchSelectionController   │
└───────────────┬──────────────┘  └────────────┬───────────────┘
                │                              │
┌───────────────▼──────────────┐               │
│ core                         │◄──────────────┘
│ CaseProject                  │
│ Domain/Mesh/Boundary/Field   │
│ Validator/Invariants         │
└───────────────┬──────────────┘
                │
┌───────────────▼──────────────────────────────────────────────┐
│ io                                                           │
│ ProjectJsonRepository、OpenFoamCaseExporter、Dict Writers    │
└──────────────────────────────────────────────────────────────┘
```

依赖方向只能自上而下，禁止反向依赖。

### 2.1 目录结构与构建目标

`src/core` 与 `src/io` 各自在自己的 `src/<module>/CMakeLists.txt` 里声明源文件、警告选项和测试登记，根 `CMakeLists.txt` 用 `add_subdirectory()` 引入这两个模块；`ofgui_render`、`ofgui_ui`、`OpenFOAMGUI` 以及 `tests/render`、`tests/ui` 的测试目标仍直接登记在根 `CMakeLists.txt`。新增 core/io 文件只改所属模块的 `CMakeLists.txt`；新增 render/ui/app 文件改根文件。

```text
src/
├── core/                       # ofgui_core：纯 C++20 标准库领域模型
│   ├── CMakeLists.txt
│   ├── Basics.h                # Real / Index / kRealTolerance
│   ├── Vec3d.h                 # 三维值类型（仅头文件）
│   ├── Version.h / .cpp        # 模块骨架锚点（模块名 + 语义版本）
│   └── …                       # CaseProject、DomainBox、CaseValidator 等
├── io/                         # ofgui_io：字典解析/写出与工程持久化
│   ├── CMakeLists.txt
│   ├── Version.h / .cpp        # 模块骨架锚点（复用 core 的版本格式化）
│   └── …                       # FoamLexer、FoamParser、writers、exporter 等
├── app/
├── render/
└── ui/
tests/
├── core/                       # 由 src/core/CMakeLists.txt 登记
├── io/                         # 由 src/io/CMakeLists.txt 登记
├── render/                     # 由根 CMakeLists.txt 登记
└── ui/                         # 由根 CMakeLists.txt 登记
```

| 目标 | 类型 | 直接依赖 | 禁止依赖 |
| --- | --- | --- | --- |
| `ofgui_core` | STATIC | C++20 标准库 | Qt（含 `Qt6::Core`）、VTK |
| `ofgui_io` | STATIC | `ofgui_core`、`Qt6::Core` | Qt Widgets、Qt GUI、VTK |
| `ofgui_render` | STATIC | `ofgui_core`、`Qt6::Widgets`、VTK | `ofgui_io`、`ofgui_ui` |
| `ofgui_ui` | STATIC | `ofgui_render`、`ofgui_io`、`Qt6::Widgets` | 直接写 OpenFOAM 文件 |
| `OpenFOAMGUI` | 可执行 | `ofgui_ui` | — |

模块边界规则：

1. `ofgui_core` 的编译/归档命令中不得出现 Qt 或 VTK 的 include、define 和库（对应 `docs/CASE_IMPORT_SPEC.md` §10.7 与评审清单第 8 项）。
2. `core`/`io` 头文件不得 `#include <QWidget>` 等 UI 类型；`io` 只允许 `Qt6::Core`（`QJson*`、`QFile` 等）。
3. 每个测试文件一个可执行目标，测试源码放 `tests/<module>/`，在每个测试实际所属的 `CMakeLists.txt`（core/io 在模块内，render/ui 在根文件）里 `add_test`，命名 `ofgui_<module>_<subject>_tests`。
4. 源文件列表必须显式登记，禁止 `file(GLOB …)`。

---

## 3. 模块职责

### 3.1 `core`

职责：

- 定义项目数据模型。
- 定义模型不变量。
- 实现校验逻辑。
- 提供不依赖 UI 的枚举、ID、值对象。
- 为 UI、IO、Render 提供只读或受控访问。

禁止：

- 创建 QWidget。
- 包含 VTK 头文件。
- 直接写 OpenFOAM 文件。
- 弹出对话框。
- 保存窗口尺寸等 UI 状态。

建议类：

```text
CaseProject
DomainBox
MeshSettings
BoundaryPatch
FaceId
FieldDefinition
BoundaryCondition
SolverSettings
ValidationIssue
CaseValidator
```

### 3.2 `application`

职责：

- 管理当前打开的工程文档。
- 执行新建、加载、保存、导出流程。
- 在模型发生变化时发出通知。
- 协调 UI、IO 和 Render。
- 管理 dirty 状态。
- 后续管理 Undo/Redo 命令栈。

建议类：

```text
DocumentController
DocumentState
ExportService
ProjectService
SelectionService
```

### 3.3 `io`

职责：

- 保存/加载 `.ofgproj`。
- 将模型转换为 OpenFOAM 字典。
- 管理文件编码、换行和目录结构。
- 实现原子写入和备份。
- 返回结构化错误，不直接弹窗。

建议类：

```text
ProjectJsonRepository
OpenFoamCaseExporter
BlockMeshDictWriter
ControlDictWriter
FvSchemesWriter
FvSolutionWriter
TransportPropertiesWriter
FieldFileWriter
AtomicDirectoryWriter
```

### 3.4 `render`

职责：

- 将 `CaseProject` 中的计算域和 patch 显示为 VTK 场景。
- 处理相机、方向标记、视图适配。
- 将 VTK 拾取结果映射回 patch ID。
- 将选择状态高亮显示。
- 后续加载 `polyMesh` 和结果场。

禁止：

- 修改业务模型作为唯一结果。
- 直接写工程文件。
- 保存 patch 名称或边界条件的主状态。

建议类：

```text
VtkViewportWidget : QVTKOpenGLNativeWidget
CaseScene
DomainActorBuilder
PatchActorRegistry
PatchSelectionController
SceneStyle
```

### 3.5 `ui`

职责：

- 显示工程树和属性编辑器。
- 收集用户输入。
- 调用 DocumentController 的受控接口。
- 显示校验结果和错误消息。
- 管理窗口布局和快捷键。

禁止：

- 直接拼 OpenFOAM 文本。
- 绕过模型直接修改 VTK actor 后认为状态已更新。
- 把 IO 逻辑实现在按钮槽函数中。

---

## 4. 数据模型设计

### 4.1 标识符

- 每个 patch、field 和 boundary condition 使用稳定 ID。
- 显示名称可以修改，ID 不随名称变化。
- 树节点、VTK actor 和模型对象通过 ID 关联。

首版可使用 `QUuid` 或整数 ID。若使用 `QUuid`，JSON 中保存字符串。

### 4.2 计算域

```text
DomainBox
  mode: TwoD | ThreeD
  min: Vec3<double>
  max: Vec3<double>
  unit: string
```

不变量：

- 三个轴分别满足 `max > min`。
- 2D 模式下必须指定厚度方向。
- 尺寸不允许 NaN 或 infinity。

### 4.3 网格

```text
MeshSettings
  nx, ny, nz: int
  gradingX, gradingY, gradingZ: double
```

不变量：

- 网格数大于 0。
- 总网格数不超过配置的安全上限。
- 2D 模式下厚度方向网格数为 1。

### 4.4 边界面和 patch

```text
FaceId = XMin | XMax | YMin | YMax | ZMin | ZMax

BoundaryPatch
  id: stable id
  name: string
  type: patch | wall | empty | symmetry | symmetryPlane
  faces: set<FaceId>
  color: RGB
  visible: bool
```

不变量：

- 六个面全部且只能归属一个 patch。
- patch 名称唯一。
- patch 类型合法。
- 2D 模式前后面的类型必须为 `empty`。
- 至少存在一个 patch。
- patch 名称必须满足安全命名规则。

### 4.5 场和边界条件

```text
FieldDefinition
  id
  name
  fieldClass: volScalarField | volVectorField
  dimensions
  internalField
  boundaryConditions: map<patchId, BoundaryCondition>

BoundaryCondition
  type: noSlip | fixedValue | zeroGradient | ...
  parameters: typed value or JSON object
```

首版建议为 p 和 U 使用明确类型，而不是完全通用的任意字典：

```text
ScalarFieldDefinition
VectorFieldDefinition
```

稳定后再抽象公共基类。

### 4.6 工程

```text
CaseProject
  schemaVersion
  name
  caseDirectory
  domain
  mesh
  patches
  fields
  solver
  exportSettings
```

`CaseProject` 是导出和渲染的业务根对象。

### 4.7 模型实现（t3 已落地）

`src/core` 已按“一个类一个文件”落地首版模型，全部位于 `namespace ofgui::core`，不链接 Qt Widgets/VTK：

| 文件 | 内容 |
| --- | --- |
| `src/core/Basics.h` | `Real`（double）、`Index`、`kRealTolerance` |
| `src/core/Vec3d.h` | 值语义三维向量（分量、点积、长度、归一化、近似比较、分量极值） |
| `src/core/FaceId.h` | `FaceId` 六面枚举、`kAllFaceIds`、`faceIndex/faceAxis/faceIsMax/faceName` |
| `src/core/ObjectId.h/.cpp` | 稳定对象 ID（16 位十六进制字符串，可往返解析） |
| `src/core/DomainBox.h/.cpp` | 计算域（min/max/mode/unit，尺寸、体积、2D 建议） |
| `src/core/MeshSettings.h/.cpp` | 网格（cells[3]、grading[3]、scale，总单元数与预算） |
| `src/core/BoundaryPatch.h/.cpp` | patch（稳定 id、可改显示名、类型、面集合、颜色、可见性） |
| `src/core/FieldValue.h/.cpp` | `internalField` 取值（均匀标量/均匀向量/非均匀原文） |
| `src/core/BoundaryCondition.h/.cpp` | 边界条件（类型原文 + 参数表，附常用类型白名单） |
| `src/core/FieldDefinition.h/.cpp` | 场（名称、类别、dimensions[7]、internalField、按 patchId 的 BC 表） |
| `src/core/SolverSettings.h/.cpp` | 求解设置（application、时间控制；`fvSchemes/fvSolution` 另行扩展） |
| `src/core/SourceLocation.h/.cpp` | 文件/行/列定位 |
| `src/core/ValidationIssue.h/.cpp` | 校验问题（Severity、稳定代码、目标对象、建议修复、定位、nodePath、snippet、origin） |
| `src/core/ValidationResult.h/.cpp` | 问题集合（按级别过滤、首个阻断项、摘要） |
| `src/core/FaceAssignmentValidator.h/.cpp` | 六面归属覆盖统计（未归属/多归属/完整覆盖） |
| `src/core/CaseProject.h/.cpp` | 工程根对象（域、网格、patch 列表、场列表、求解设置、导入来源 `source`） |
| `src/core/CaseSourceInfo.h/.cpp` | 导入来源（源相对路径、`startDirectory`、`importedAt`、逐文件 SHA-256 与 hex 转换） |
| `src/core/CaseValidator.h/.cpp` | 全部模型不变量的唯一校验入口（`validate()` 编排；各规则族见下） |
| `src/core/CaseValidatorDomain.cpp` | `CASE-DOM-*` 计算域几何规则 |
| `src/core/CaseValidatorMesh.cpp` | `CASE-MESH-*` 网格数/渐变/scale/2D 厚度规则 |
| `src/core/CaseValidatorPatches.cpp` | `CASE-PATCH-*` 命名、六面归属与 2D empty 规则 |
| `src/core/CaseValidatorFields.cpp` | `CASE-FIELD-*` 场名、internalField 与 BC 完整性规则 |
| `src/core/CaseValidatorSolver.cpp` | `CASE-SOLV-*` 时间控制与 application 规则 |
| `src/core/CaseValidatorMessages.h` | 上述五个实现文件共享的消息与格式化辅助（`ofgui::core::detail`，模块内部；不含校验逻辑） |
| `src/core/ProjectSchemaVersion.h` | 工程 schema 版本与扩展名 |
| `src/core/Version.h/.cpp` | 模块版本（构建边界自检） |

实现约定：

- **FaceId 唯一定义在 `core`**（`docs/DECISIONS.md` D-026）：`src/render/FaceId.h` 只保留 `namespace ofgui` 的 `using` 别名，render/ui 既有代码无需改动。
- **setter 不拒绝非法值**（D-027）：编辑器需要能暂存“正在输入”的中间状态；合法性统一由 `CaseValidator` 判断并给出可定位问题。
- **问题代码稳定**（D-028）：`CASE-DOM-*`（域）、`CASE-MESH-*`（网格）、`CASE-PATCH-*`（面归属/命名/2D 空隙边界）、`CASE-FIELD-*`（场与 BC）、`CASE-SOLV-*`（时间控制）；只有 `BlockingError` 阻断导出。
- Python/JSON 侧与 GUI 侧对上述字段使用同名 key，见 `docs/CASE_IMPORT_SPEC.md` §6。

---

## 5. 文档与状态流

### 5.1 单向数据流

```text
用户输入
  ↓
UI 控件
  ↓
DocumentController 修改模型
  ↓
模型版本/变更信号
  ↓
工程树刷新 + 属性面板刷新 + VTK 场景刷新
```

VTK 中点击面时：

```text
VTK 拾取
  ↓
得到 patch ID / face ID
  ↓
SelectionService
  ↓
ProjectTree 选中
  ↓
Inspector 显示对应属性
```

VTK actor 永远不能成为 patch 状态的唯一来源。

### 5.2 文档状态

```text
New
Loading
Ready
Modified
Exporting
Error
Closing
```

`DocumentController` 统一维护 dirty 状态。用户保存成功后清除 dirty，导出失败不应错误清除 dirty。

### 5.3 工程树与选中（t8）

左侧配置区由一个 `ProjectTreeView`（`QTreeView` + `ProjectTreeModel`）承载，层级固定为：

```text
工程 · <显示名> *
├── 计算域
├── 网格
├── 边界（六面）
│   ├── xMin → inlet (patch)
│   ├── …（共 6 个面，未归属时显示“未分配”）
├── 场（0/）
│   ├── p [volScalarField]
│   │   ├── movingWall — zeroGradient
│   │   └── …（该场在每个 patch 上的 BC）
│   └── U [volVectorField]
├── 求解器设置
└── 导出预览
```

- 树节点身份是 `ProjectNodeRef`：`NodeKind` + 可选 `FaceId` + 可选 `fieldId` + 可选 `patchId`，由 `ProjectTreeModel::NodeRefRole` 保存；界面显示文本只用于展示，禁止反向解析文本得到身份。
- 选中通过 `ProjectTreeView::nodeSelected(ProjectNodeRef)` 传播：`ConfigurationPanel` 将其转给 `InspectorPanel::showNode()`（按 ID 从 `CaseProject` 取值）。VTK 面高亮 API 尚未实现，选中目前只联动 Inspector（见 Backlog M3-04）。
- 选择“面”节点时，Inspector 通过 `CaseProject::findPatchForFace()` 解析所属 patch，并列出该 patch 上所有场的 BC；选择“场 → patch”节点时显示单个 BC 的类型与参数。
- `ProjectTreeModel` 在 `CaseDocumentController` 的 `projectChanged` / `documentStateChanged` / `importReportChanged` 上重建，重建前后用 `modelAboutToBeReset` / `modelReset` 保存并恢复选中（按 ID，不按行号）。
- 打开算例与打开工程都走 `CaseDocumentController`：前者调用 `io::CaseImporter::import()` 生成 `CaseProject` 并把导入报告保存在 document 上，后者调用 `io::ProjectJsonRepository::load()`；两条路径最终都发出同一组信号，树和 Inspector 不区分来源。
- 新建、打开、关闭前的 dirty 保护由 `MainWindow::maybeSaveChanges()` 统一处理（保存 / 放弃 / 取消），窗口标题用 `[*]` 占位并在保存后清除。

### 5.4 计算域编辑器与场景同步（t9）

`InspectorPanel` 内部是一个 `QStackedWidget`：选中“计算域”或“网格”时切到可编辑的 `DomainEditor`；选中场或“场 → patch”时切到可编辑的 `FieldEditor`；选中“求解器设置”时切到 `SolverEditor`；其他节点切到 `NodeDetailFormatter` 生成的只读 HTML 页。

- `DomainEditor` 只通过 `CaseDocumentController` 写模型。每次提交先由 `readCandidate()` 从控件读出 `DomainBox` + `MeshSettings`，在 UI 侧检查硬不变量：每轴 `max > min`、三个方向 `cells > 0`、二维模式 `z = 1`。任一条不满足就只更新红色错误行并返回，`CaseProject` 与 VTK 场景保持上一次合法状态（非法值不进入模型，也不会被静默夹取）。
- 通过检查后调用 `setDomain()` + `setMesh()` + `notifyProjectEdited()`；document 依次发出 `validationChanged` → `projectChanged` → `documentStateChanged`，工程树、Inspector、主窗口状态栏都从这条信号链刷新，编辑器自身用 `m_updating` 防止回写触发递归。
- 二维/三维：切到二维时先把 z 网格数强制为 1 再提交；三维模式下 z = 1 只给灰色提示（“如果算例实为二维，请切换为二维模式”），不阻断。
- 场景侧：`MainWindow` 监听 `projectChanged` 调 `VtkViewportWidget::setDocumentDomain(domain)`。`render::DomainBoundsAdapter` 把 `DomainBox` 转成 `PreviewDomain`（非有限或非正尺寸返回 `nullopt`），`sameBounds()` 以 `core::kRealTolerance` 的相对容差比较；bounds 未变（例如只改网格数）时保留 actor 与相机，不重建。
- 新建工程默认域为原点处的 1 m 单位立方体，保证首次选中“计算域”就有可画、可编辑的状态；打开算例/工程、新建、关闭后主窗口都会调用 `fitView()`，该调用只改相机，不写模型。
- 已知边界：`DomainBox::unit()` 暂不在编辑器中修改（核心模型已支持，UI 排入 Backlog）；patch 面高亮仍是 M3-04。

### 5.5 场与边界条件编辑器（t10）

选中“场（0/）→ 某场”或“某场 → 某 patch”时，Inspector 显示 `FieldEditor`；`FieldBoundaryTable` 是该编辑器内按 patch 逐行编辑 BC 的表格，`UniformValueEditor` 负责 `uniform <scalar>` / `uniform (x y z)` 的输入。

- 身份仍是稳定 ID：`FieldEditor::showField(fieldId)`、`FieldBoundaryTable::patchId` 都取自 `core::ObjectId`；表格行的 patch 通过 `QComboBox` 的 `userData` 携带 patchId，不解析显示文本。
- 可编辑范围与 D-024 对齐：`scalarTypeNames()` / `vectorTypeNames()` 是白名单；导入的类型不在白名单时只读展示原文（灰色提示“只读保存”），用户不能把它改成别的类型但保存时原文不变。
- 写模型只有一条路径：控件信号 → 校验 → `CaseProject::setInternalField()` / `setBoundaryCondition()` → `CaseDocumentController::notifyProjectEdited()`。校验失败只显示红色错误行，模型和预览保持上一次合法状态。
- `internalField`：标量/向量都用 `UniformValueEditor`；`nonuniform` 列表、`$internalField` 等非 uniform 原文以只读 `QLineEdit` 展示，保存时逐字保留。`dimensions` 首版只读（7 个指数完整显示），避免误改物理量纲。
- 预览：`FieldEditor::updatePreview()` 直接调用 `io::FieldFileWriter::write(field, project)`，因此预览与导出共享同一 writer；`ExportPreviewPanel` 用同一 writer 列出 0/ 下所有场文件。
- 新增/删除场：`FieldEditor::addPredefinedField("p"/"U")` 走 `CaseProject::addField()` 并按 patch 建立默认 BC（p → zeroGradient，U → noSlip），删除只允许首版模板内的 p/U，避免误删求解必需场；两类操作都先写模型再 `notifyProjectEdited()`。
- 表格列宽固定为 patch / 类型 / 值，类型列宽 124 px 以完整显示 `zeroGradient`、`fixedValue` 等；表格放在 `QScrollArea` 中，窄面板下整体纵向滚动而不是横向截断。
- 导入的 cavity：p 的 dimensions `[0 2 -2 0 0 0 0]`、internalField `uniform 0`；U 的 `[0 1 -1 0 0 0 0]`、`uniform (0 0 0)`，movingWall 的 U 为 `fixedValue` + `uniform (1 0 0)`，与 `0/p`、`0/U` 原文一致。

### 5.6 求解设置编辑器（t11）

选中“求解器设置”时 Inspector 显示 `SolverEditor`，用四个页签分开“能改的模型值”“只读的导入原文”“导出预览”：`controlDict`、`constant`、`只读原文`、`导出预览`。

- 可编辑面严格等于领域模型已有的字段（D-037）：`application`、`startTime`、`endTime`、`deltaT`、`writeInterval` 写 `core::SolverSettings`；`transportModel`、`nu` 写 `CaseDocumentController::transportSettings()`（`io::TransportModelSettings`，也就是 exporter 消费的同一对象）。没有模型支撑的键（`startFrom`、`stopAt`、`writeControl`、`fvSchemes`、`fvSolution`、`simulationType`）不提供可编辑控件，避免“改了界面但导出没变”。
- `startFrom`/`stopAt`/`writeControl` 的导入值从 `readOnlySettings()` 按路径精确读取（`system/controlDict.startFrom`），以只读说明文字显示；同时说明导出固定写 `startTime`/`endTime`/`timeStep`。
- `nu` 只在纯数值写法下可编辑；带量纲（`nu [0 2 -1 0 0 0 0] 1e-05`）或表达式时 `isKinematicViscosityEditable()` 为 false，控件置灰并显示原值，保存时逐字回写。`transportModel` 仅在为空或 `Newtonian` 时可改（首版 writer 只支持这两个取值），其他模型只读展示。
- 持久化边界（t23 / F-05，策略 B）：`transportModel`/`nu` 目前只存在于会话内的 `io::TransportModelSettings` 与源算例投影，**不写入 `.ofgproj`**（schema 仍为 v2）。因此 `constant` 页在控件下方固定提示“transportModel/nu 不随 .ofgproj 保存；重新打开工程时会按源算例的 constant/transportProperties 重读”，保存与重开工程的状态栏消息也会重复该说明，避免用户误以为编辑丢失是 bug。要真正持久化需要 schema v3（core/io 侧任务），本轮不做。
- `simulationType` 只读：`turbulenceProperties` 只有 `laminar` 会被 writer 写出，其他值在 UI 中明确标注“导出保持导入值”。
- 写路径：控件信号 → `readCandidate()` 式校验（`endTime > startTime`、`deltaT > 0`、`writeInterval ≥ 1`）→ `CaseProject`/`transportSettings` → `notifyProjectEdited()` / `notifyTransportEdited()`。`m_updating` 防止刷新回写递归；dirty 标记、工程树标题与状态栏都由 document 信号链刷新。
- `只读原文` 页签列出 `CaseDocumentController::availableSourceDictionaries()`（`system/controlDict`、`fvSchemes`、`fvSolution`、`constant/transportProperties` 等），文本来自导入时缓存的逐字节原文 `sourceDictionaryText()`，不重新解析。
- `导出预览` 页签直接调用 `io::ControlDictWriter::write(project)` 与 `io::TransportPropertiesWriter::write(transportSettings)`；预览文本与导出文件同源，测试逐字比较。

### 5.7 校验、导出与默认算例（t12）

导出是唯一写 OpenFOAM 文件的入口，链路固定为“模型校验 → 计划生成 → 计划字典校验 → 原子写盘 → 回读 manifest”：

- `ExportController::preview(target)`：把 `document.validation()` 的问题与 `io::ExportPlanBuilder::build()` 的问题合并，再用 `ExportDictionaryCheck` 重新解析计划中每个生成文件。存在 `Error`/`BlockingError` 或计划为空时 `ok = false`，`errorText` 逐条列出问题；对话框据此禁用“导出”按钮并显示红色说明，因此阻断问题在按下导出之前就可见。
- `ExportController::exportTo(target, allowExistingTarget)`：空目标、目标位于源算例目录内、模型含错误/阻断问题、预览失败都在写盘前返回中文原因；随后调用 `io::CaseExporter::exportProject()`。该函数内部再执行一次 `CaseValidator` 并写入同级临时目录后重命名，失败时不产生目标目录（测试断言无 `.tmp-*` 残留）。
- D-037 接线点：`makeOptions()` 把 `document.transportSettings()` 原样填入 `io::CaseExportOptions.transport`，并把 `sourceDictionaryText("system/fvSchemes")`、`sourceDictionaryText("system/fvSolution")` 填入 `fvSchemesText`/`fvSolutionText`。求解页的 transportModel/nu 编辑与导入的 schemes 原文由此真正进入导出文件（测试断言 `nu 0.02` 出现在导出的 `constant/transportProperties`）。
- 成功回读 `<target>/.ofgport-export.json`，用 written/copied/mirrored/skipped 计数与文件数组装状态栏与结果对话框文本；覆盖已有目录时 `AtomicCaseDirectory` 生成 `<target>.bak-<stamp>` 备份。
- `ExportPreviewDialog` 只展示不写盘：文件树（文件/动作/大小）、动作中文名（新写/复制/镜像/跳过）、生成文件原文预览、计划统计；目标目录已有内容时必须勾选“覆盖”并在确认框二次确认，`allowExistingTarget()` 才为真。
- `ValidationPanel` 作为主窗口底部 dock（`validationDock`）常驻：按级别着色列出问题，双击通过 `ValidationNavigator::nodeRefForIssue()` 把 `targetField`/对象 ID 映射为 `ProjectNodeRef` 后选中树节点；主窗口在有 `Error`/`BlockingError` 时禁用导出动作并在 tooltip 说明原因。
- 首次启动与“新建工程”使用 `DefaultCaseTemplate` 的 cavity 模型（2D、20×20×1、icoFoam 0→0.5、p/U 与三个 patch 的 BC 齐备、nu 0.01），因此“新建 → 导出”无需任何手工补值即可得到可运行的 v2206 算例；避免“界面能开但导出必失败”的假可用状态。
- 打开已有算例后 `importReport` 仍只在工程页展示，导出永远以模型为源；未建模的键（`fvSchemes`/`fvSolution` 等）通过镜像源文件或原文透传保留，不假装可编辑。

---

## 6. VTK 设计

### 6.1 Qt 集成

使用：

- Qt 6 Widgets。
- `QVTKOpenGLNativeWidget`。
- `vtkGenericOpenGLRenderWindow`。
- `vtkRenderer`、`vtkRenderWindow`、`vtkRenderWindowInteractor`。
- `vtkModuleAutoInit` 对应的 CMake 初始化。

不要使用 QML 版 VTK 控件，除非未来重新决策。

### 6.2 面 actor

每个长方体面使用独立 actor：

- 六个 `vtkPlaneSource` 或等价几何源。
- 每个 actor 对应一个 FaceId。
- actor 的名称可以辅助调试，但不参与业务映射。
- 通过 `std::unordered_map<vtkActor*, FaceId>` 或注册表映射。

每个面同时包含：

- 面 actor。
- 边线 actor。
- 选择高亮 actor。

### 6.3 选择

首版使用 `vtkPropPicker` 或适合面 actor 的 picker：

1. 用户点击视口。
2. 判断命中的 actor。
3. 映射为 FaceId。
4. 查找该 FaceId 所属 patch ID。
5. 更新 SelectionService。
6. UI 刷新属性面板。
7. 场景更新高亮。

如果后续改为加载 `polyMesh`，可使用 cell picker，但必须重新设计映射层，不能污染首版模型。

### 6.4 场景重建

- 域尺寸变化：更新六个面的几何。
- patch 名称变化：不重建几何，只更新 UI 标签。
- patch 类型变化：更新颜色/样式和 UI。
- patch 可见性变化：切换 actor visibility。
- 选择变化：切换高亮，不重建所有 actor。

首版允许全量重建，但接口应允许增量更新。t9 的落地方式：`VtkViewportWidget::setDocumentDomain()` 先用 `render::DomainBoundsAdapter` 得到 `PreviewDomain`，`sameBounds()` 判定 bounds 未变时直接返回（不重建 actor、不重置相机）；变化时 `rebuildScene()` 重新生成六个面 + outline，并递增 `sceneRevision()` 供测试断言。

### 6.5 相机

- 初始等轴测视图。
- 自动 fit 计算域。
- 支持正交和透视。
- 保存到 GUI 工程时可选择保存相机状态，但这不是首版必须项。

---

## 7. OpenFOAM 字典生成设计

### 7.1 生成原则

- writer 是独立 C++ 类。
- 输入为 `CaseProject` 或相关子模型。
- 输出为字符串或内存文件集合。
- 不直接操作 Widget。
- 不调用 OpenFOAM。
- 所有输出可进行字符串级黄金文件测试。
- 使用 LF 结尾。
- 明确字符编码，字典文本优先 ASCII。

### 7.2 文件集合

首版输出：

```text
case/
├── 0.orig/
│   ├── p
│   └── U
├── constant/
│   └── transportProperties
└── system/
    ├── blockMeshDict
    ├── controlDict
    ├── fvSchemes
    └── fvSolution
```

**导入场景例外（D-018）**：上述 `0.orig/` 是**新建工程**的固定输出。当工程来自“导入已有算例”时，起始时间目录名**继承源目录**：源用 `0/` 就输出 `0/`，源用 `0.orig/` 就输出 `0.orig/`。

- 原因：v2206 教程的 `Allrun` 普遍假定 `0.orig` 存在并 `cp -r 0.orig 0`；把源 `0/` 强行改名为 `0.orig/`（或反向改名）都会破坏用户既有工作流。
- 实现：`CaseProject.startDirectory`（`0` | `0.orig`）为模型字段，导出器与 `ExportPreviewDialog` 只读该字段决定输出子目录名；新建工程时该字段固定为 `0.orig`。
- 文件集合（`p`、`U`、`constant/transportProperties`、`system/*`）与现有 §7.1 原则不变；导入场景下未接管文件按镜像策略保留，见 `docs/DECISIONS.md` D-023。

### 7.3 生成接口示例

```cpp
struct GeneratedFile
{
    std::filesystem::path relativePath;
    std::string content;
};

class OpenFoamCaseExporter
{
public:
    ExportResult build(const CaseProject& project) const;
    ExportResult write(const CaseProject& project,
                       const std::filesystem::path& targetDirectory) const;
};
```

`build` 用于预览，`write` 用于落盘。二者共享同一生成逻辑，避免预览与实际文件不一致。

### 7.4 导出目录安全

1. 校验目标路径非空。
2. 校验目标不是磁盘根目录。
3. 在目标同级创建临时目录。
4. 写入全部文件。
5. 校验必需文件存在。
6. 如果目标已存在，创建备份或提示用户。
7. 原子替换目标。
8. 失败时清理临时目录并保留原目录。

---

## 8. 校验与错误模型

### 8.1 错误级别

```text
Info
Warning
Error
BlockingError
```

只有 `BlockingError` 阻止导出。

### 8.2 ValidationIssue

```text
ValidationIssue
  severity
  code
  message
  targetObjectId
  targetField
  suggestedFix
```

UI 根据 `targetObjectId` 定位到树节点和属性面板，不让用户只看到一段无上下文的错误文本。

### 8.3 典型错误

- 最小点不小于最大点。
- 网格数为零或负数。
- patch 名称为空。
- patch 名称重复。
- 一个面没有归属。
- 一个面被多个 patch 使用。
- 2D 模式的前后面不是 `empty`。
- 某场缺少 patch 边界条件。
- target 目录不可写。
- 输出文件已存在且未确认覆盖。

---

## 9. 工程文件与版本迁移

`.ofgproj` 必须包含：

```json
{
  "schemaVersion": 1
}
```

加载流程：

1. 读取 JSON。
2. 检查 schemaVersion。
3. 相同版本：反序列化。
4. 较旧版本：执行迁移链。
5. 较新版本：拒绝加载并提示升级 GUI。
6. 损坏 JSON：显示具体路径和解析错误。

任何 schema 修改都必须更新 `docs/DECISIONS.md` 并增加迁移测试。

---

## 10. 测试架构

### 10.1 Core 测试

- DomainBox 边界条件。
- MeshSettings。
- Patch 面覆盖。
- 名称校验。
- field/BC 完整性。
- schema 往返。

### 10.2 IO 测试

- 字典黄金文件。
- 文件目录结构。
- 换行符。
- 覆盖保护。
- 临时目录失败回滚。

### 10.3 Render 测试

- 场景对象数量。
- face 到 actor 的映射。
- actor 颜色和可见性。
- 选择状态变更。
- 相机 fit 不崩溃。
- `ofgui_render_domain_tests`（t9）：`DomainBoundsAdapter` 的 `DomainBox → PreviewDomain` 转换、非法盒拒绝、容差比较，以及从 `DomainBox` 构建六面场景。

### 10.4 UI 测试

- 主窗口启动。
- 新建工程。
- 修改 min/max 后模型变化。
- 树节点选中联动。
- 导出预览对话框内容。
- 关闭未保存工程时出现提示。
- `ofgui_ui_domain_tests`（t9）：选中计算域切到编辑页；改 max/min 后 `CaseProject` 与 `VtkViewportWidget::sceneBounds()` 同步、`sceneRevision()` +1；max ≤ min / cells = 0 被拒绝且场景不重建；只改网格数不重建；2D 强制 z=1 且错误为空；cavity 导入后六个坐标与 20/20/1 回填。
- `ofgui_ui_field_tests`（t10）：cavity 导入后 p/U 的 dimensions、internalField 与逐 patch BC 回填；改 internalField/边界类型/值后模型、脏标记与 0/ 预览同步；类型白名单外的导入 BC 只读；非法值被拒绝且模型不变；新增/删除 p、U 的模板行为；`ExportPreviewPanel` 文本与 `io::FieldFileWriter` 一致；Inspector 对 Field/FieldBoundary 的路由。
- `ofgui_ui_solver_tests`（t11）：cavity 导入后 application/startTime/endTime/deltaT/writeInterval 回填（`0`/`0.5`/`0.005`/`20`）；改 endTime/writeInterval/application 后模型、脏标记与 controlDict 预览同步；改 nu 后 `transportSettings` 与 transportProperties 预览同步；两个预览页签与 writer 输出逐字相等；只读原文页签列出导入字典且文本含 `application`/`divSchemes`；新建工程显示默认求解设置；Inspector 对 Solver 节点的路由。
- `ofgui_ui_workflow_tests`（t12）：新建工程即 cavity 模板且可直接导出；导出计划含 `0/p`、`0/U`、`system/blockMeshDict`、`system/controlDict`、`system/fvSchemes`、`system/fvSolution`、`constant/transportProperties` 且非空；导出成功后 `.ofgport-export.json` 的计数与结果一致；transport 编辑（nu = 0.02）出现在导出的 `constant/transportProperties`；阻断问题阻止导出且目标目录不存在、不留 `.tmp-*` 残留；已存在目标需显式覆盖、覆盖时生成 `.bak-<stamp>` 备份且旧文件不丢；`EXPORT-DICT-001` 捕获损坏的字典文本；校验面板双击问题定位到对应树节点；存在阻断问题时主窗口导出动作禁用；预览对话框在阻断态禁用“导出”并显示 `导出预检失败：…CASE-MESH-001…`；对话框销毁（含目标行仍持有焦点）不再触发悬空信号回调。

---

## 11. 扩展点

### 11.1 求解器模板

不要硬编码无数 if/else。后续定义：

```text
SolverTemplate
  name
  requiredFields
  defaultDictionaries
  supportedBoundaryTypes
  fieldWriters
```

首版只注册 `icoFoam` 模板。

### 11.2 网格来源

后续定义接口：

```text
IMeshSource
  generate()
  load()
  meshType()
```

首版实现 `BlockMeshBoxSource`。后续可以增加 `PolyMeshSource`、`StlSource`。

### 11.3 文件格式升级

OpenFOAM 版本差异通过模板版本处理：

```text
resources/templates/openfoam-v2206/
```

不要把 v2206 的每个细节散落在 UI 中。

### 11.4 可视化扩展

渲染组件通过模型信号更新：

- 计算域预览。
- polyMesh 预览。
- 结果场预览。
- 网格质量预览。

不同可视化模式使用独立 builder，不共享一个巨型 render 函数。

---

## 12. 架构守则

1. 模型变更必须通过 DocumentController。
2. VTK 不保存主业务状态。
3. UI 不生成 OpenFOAM 文本。
4. Writer 不做 UI 弹窗。
5. Renderer 不写文件。
6. 不编译或链接 OpenFOAM。
7. 新功能先写不变量，再写 UI。
8. 新文件格式先写测试样例，再写 writer。
9. 每个扩展点只抽象一层，不做过度设计。
10. 每次架构变化记录到 `docs/DECISIONS.md`。