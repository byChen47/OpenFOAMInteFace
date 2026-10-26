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

首版允许全量重建，但接口应允许增量更新。

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

### 10.4 UI 测试

- 主窗口启动。
- 新建工程。
- 修改 min/max 后模型变化。
- 树节点选中联动。
- 导出预览对话框内容。
- 关闭未保存工程时出现提示。

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