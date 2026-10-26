# OpenFOAM GUI 开发计划与可持续 Agent 路径

> 文档状态：初始基线  
> 项目定位：独立的 Windows 桌面可视化界面  
> 技术栈：C++20 + Qt 6 Widgets + VTK 9.6 + CMake + Ninja + MinGW  
> 关键边界：不编译、不链接、不嵌入 OpenFOAM 源码

---

## 1. 项目目标

开发一个可独立运行的 OpenFOAM 可视化前处理界面，使用户能够通过图形界面完成以下工作：

1. 新建和打开一个 GUI 工程。
2. 输入长方体计算域的最小点和最大点。
3. 设置网格数量、均匀网格和基础网格选项。
4. 在三维视图中观察计算域。
5. 选择长方体的六个面。
6. 为边界面设置名称和 patch 类型。
7. 为 p、U 等场设置边界条件。
8. 配置一个基础 OpenFOAM 求解案例的常用字典参数。
9. 预览即将生成的 OpenFOAM 文件。
10. 导出 `0.orig/`、`constant/`、`system/`。
11. 保存和重新加载 GUI 工程，继续编辑。

最终产品是一个 **Qt/VTK 桌面程序**，不是 OpenFOAM 的构建产物，也不是 OpenFOAM 的图形前端插件。

---

## 2. 产品范围

### 2.1 本项目包含

- Qt Widgets 桌面应用。
- 类似 StarCCM+ 的左侧工程配置树和属性面板。
- 类似轻量 ParaView 的右侧三维视口。
- 长方体计算域的创建和编辑。
- 六个面的选择、高亮、命名和 patch 类型设置。
- 基础场定义和边界条件编辑。
- OpenFOAM 字典文本生成和预览。
- GUI 工程文件保存/加载。
- 输入校验、导出校验和用户错误提示。
- 必要的单元测试、黄金文件测试和手动 smoke test。
- 后续加载已有 `polyMesh` 或结果文件的可扩展接口。

### 2.2 本项目不包含

- 编译 OpenFOAM v2206。
- 链接 `libOpenFOAM`、`libfiniteVolume` 等 OpenFOAM 库。
- 在程序进程中执行 OpenFOAM 求解。
- 自动调用 `blockMesh`、`checkMesh`、`icoFoam` 或 `simpleFoam`。
- 实现完整 CFD 求解器。
- 完整 ParaView 功能。
- 任意 CAD、STEP、STL 几何建模和布尔运算。
- 任意曲面拾取。
- 云平台、账号系统、数据库和多人协作。
- 首版的多块网格和 snappyHexMesh 配置。

### 2.3 OpenFOAM-v2206 源码的用途

现有 `OpenFOAM-v2206/` 目录只用于：

- 查阅 v2206 的字典格式。
- 参考 `blockMeshDict`、`controlDict`、场文件等示例。
- 复制少量测试所需的最小字典结构到项目测试资源中。
- 作为黄金文件测试的格式依据。

它不进入应用构建，不进入安装包，不被修改。

---

## 3. 首版产品形态

### 3.1 窗口布局

```text
┌─────────────────────────────────────────────────────────────────────┐
│ 菜单栏：文件  编辑  视图  工具  帮助                                  │
├─────────────────────────────────────────────────────────────────────┤
│ 工具栏：新建 打开 保存 导出 适配视图 显示边界 显示网格               │
├───────────────────────┬─────────────────────────────────────────────┤
│ 左侧配置区             │                                             │
│                       │                                             │
│ 工程树                 │             VTK 三维视口                    │
│ ├─ 工程                │                                             │
│ ├─ 计算域              │    坐标轴 / 计算域 / 边界面 / 选择高亮      │
│ ├─ 网格                │                                             │
│ ├─ 边界                │                                             │
│ │  ├─ xMin              │                                             │
│ │  ├─ xMax              │                                             │
│ │  ├─ yMin              │                                             │
│ │  ├─ yMax              │                                             │
│ │  ├─ zMin              │                                             │
│ │  └─ zMax              │                                             │
│ ├─ 场                  │                                             │
│ ├─ 求解器设置          │                                             │
│ └─ 导出预览            │                                             │
│                       │                                             │
│ 属性编辑面板           │                                             │
├───────────────────────┴─────────────────────────────────────────────┤
│ 日志 / 校验结果 / 导出状态                                           │
├─────────────────────────────────────────────────────────────────────┤
│ 状态栏：工程路径、修改状态、渲染状态                                 │
└─────────────────────────────────────────────────────────────────────┘
```

### 3.2 推荐交互流程

1. 用户点击“新建工程”。
2. 输入工程名、保存路径和基础模板。
3. 进入“计算域”页面，输入 min/max、单位、网格数。
4. 右侧三维视图立即显示或更新长方体。
5. 用户点击左侧某个边界面，或直接在三维视图中点击面。
6. 在属性面板中修改边界名称、颜色、可见性和 patch 类型。
7. 进入“场”页面，设置 p、U 的内部场和边界条件。
8. 进入“求解器设置”页面，填写时间控制、离散格式和线性求解器参数。
9. 点击“导出预览”，查看所有将生成的文件和关键字典内容。
10. 校验通过后点击“导出”。
11. GUI 工程保存为 `.ofgproj`，OpenFOAM case 导出到指定目录。

### 3.3 视觉反馈规则

- 未生成网格时，界面显示“计算域预览”，不能标注为“网格”。
- 选中面使用明显的高亮边框或颜色。
- 每个 patch 使用稳定颜色。
- `empty`、`wall`、`patch` 等类型在树和属性面板中显示明确标签。
- 非法参数在输入框旁显示错误，并在底部校验面板汇总。
- 导出失败时必须保留原目录，不允许产生半成品覆盖。

---

## 4. 功能分解

### 4.1 工程管理

- 新建工程。
- 打开 `.ofgproj`。
- 保存、另存为。
- 最近打开列表。
- 修改状态和关闭提示。
- 工程路径显示。
- 工程 schema 版本。
- 默认导出目录。
- 导入/导出失败的错误信息。

### 4.2 计算域

- minX、minY、minZ。
- maxX、maxY、maxZ。
- 单位显示，默认 `m`。
- 2D/3D 模式切换。
- 长方体有效性校验。
- 坐标轴、包围盒、尺寸标注。
- 根据模型即时重建 VTK 场景。
- 后续预留通用几何接口。

### 4.3 网格设置

首版使用结构化单块网格：

- `nx`、`ny`、`nz`。
- 默认均匀网格。
- 可选 simpleGrading 字段预留。
- 2D 模式下 `nz=1`，前后 patch 类型为 `empty`。
- 显示估算总网格数。
- 校验总网格数、每轴网格数和最大尺寸。

### 4.4 边界设置

六个规范面使用稳定枚举：

- `XMin`
- `XMax`
- `YMin`
- `YMax`
- `ZMin`
- `ZMax`

每个 patch 包含：

- 稳定 ID。
- 用户可编辑名称。
- 面集合。
- patch 类型。
- 显示颜色。
- 可见性。
- 可选的备注。

首版支持类型：

- `patch`
- `wall`
- `empty`
- `symmetry`
- `symmetryPlane`

校验：

- 所有六个面必须被一个 patch 覆盖。
- 同一面不能属于多个 patch。
- patch 名称为空、重复或包含不安全字符时报错。
- 2D 模式要求 ZMin/ZMax 为 `empty`。
- 不允许两个 patch 使用同一名称。

### 4.5 场和边界条件

首版仅围绕一个清晰模板：`icoFoam`。

场：

- `p`：`volScalarField`
- `U`：`volVectorField`

每场支持：

- internalField。
- dimensions。
- 每个 patch 的边界条件类型。
- 条件所需参数。

首批可编辑边界条件：

- p：`zeroGradient`、`fixedValue`
- U：`noSlip`、`fixedValue`、`zeroGradient`

后续扩展时通过注册表增加，而不是在 UI 中无限堆叠 if/else。

### 4.6 求解器和字典设置

首版生成：

- `system/blockMeshDict`
- `system/controlDict`
- `system/fvSchemes`
- `system/fvSolution`
- `constant/transportProperties`
- `0.orig/p`
- `0.orig/U`

配置项以“基础模式 + 高级原文编辑（后续）”组织：

- 应用名称。
- 开始时间、结束时间。
- 时间步长。
- 写出控制。
- 写出间隔。
- 运动黏度。
- 默认离散格式。
- 默认线性求解器。

首版不追求覆盖 OpenFOAM 全部字段，只生成一个可读、结构正确、可检查的最小案例。

### 4.7 VTK 三维视图

首版能力：

- 创建 6 个可独立着色的面 actor。
- 显示边线。
- 显示坐标轴和方向标记。
- 旋转、平移、缩放。
- 正交/透视切换。
- 适配视图。
- 单击面进行选择。
- 选择状态与左侧工程树联动。
- 隐藏/显示 patch。
- 根据模型变化增量或完整重建场景。

后续能力：

- 读取已有 `polyMesh`。
- 读取 OpenFOAM 时间目录中的场。
- 颜色映射、切片、等值面。
- 网格质量可视化。

### 4.8 导出

导出流程：

1. 收集当前工程模型。
2. 执行模型校验。
3. 执行字典结构校验。
4. 在目标目录同级创建临时导出目录。
5. 生成目录和字典文件。
6. 校验必需文件和内容。
7. 目标不存在时原子重命名。
8. 目标存在时创建备份或要求用户确认。
9. 更新状态栏和日志。

覆盖策略必须明确、可恢复，不能静默覆盖。

---

## 5. 软件架构

### 5.1 分层

```text
┌─────────────────────────────────────────────────────────────┐
│ UI 层：MainWindow、Dock、Tree、PropertyEditor、Dialogs      │
└──────────────────────────┬──────────────────────────────────┘
                           │ 信号/槽、命令调用
┌──────────────────────────▼──────────────────────────────────┐
│ Application 层：DocumentController、ExportService、Commands  │
└───────────────┬──────────────────────────────┬──────────────┘
                │                              │
┌───────────────▼─────────────┐  ┌─────────────▼─────────────┐
│ Core 领域模型                │  │ IO 适配层                  │
│ Domain/Mesh/Boundary/Field  │  │ JSON Repository            │
│ Validation/Invariants       │  │ OpenFOAM Writers           │
└───────────────┬─────────────┘  └───────────────────────────┘
                │ 只读模型/场景更新事件
┌───────────────▼─────────────────────────────────────────────┐
│ Render 层：VtkViewport、SceneBuilder、Picker、Selection     │
└─────────────────────────────────────────────────────────────┘
```

### 5.2 依赖方向

- `core` 不依赖 Qt Widgets、VTK、文件对话框。
- `io` 依赖 `core`，可依赖 QtCore 做 JSON 和字符串。
- `render` 依赖 `core` 和 VTK，不拥有业务数据。
- `ui` 依赖 `core`、`io`、`render` 和应用服务。
- `app` 负责组装依赖和启动。
- `OpenFOAM-v2206` 不属于任何依赖方向。

### 5.3 主要模块

```text
src/core/
  CaseProject.h/.cpp
  DomainBox.h/.cpp
  MeshSettings.h/.cpp
  BoundaryPatch.h/.cpp
  FieldDefinition.h/.cpp
  BoundaryCondition.h/.cpp
  SolverSettings.h/.cpp
  ValidationIssue.h/.cpp
  CaseValidator.h/.cpp

src/io/
  ProjectJsonRepository.h/.cpp
  OpenFoamCaseExporter.h/.cpp
  BlockMeshDictWriter.h/.cpp
  ControlDictWriter.h/.cpp
  SchemeSolutionWriter.h/.cpp
  FieldFileWriter.h/.cpp
  AtomicDirectoryWriter.h/.cpp

src/render/
  VtkViewportWidget.h/.cpp
  CaseScene.h/.cpp
  DomainActorBuilder.h/.cpp
  PatchSelectionController.h/.cpp
  SceneStyle.h/.cpp

src/ui/
  MainWindow.h/.cpp
  ProjectTreeModel.h/.cpp
  ProjectTreeView.h/.cpp
  DomainInspector.h/.cpp
  BoundaryInspector.h/.cpp
  FieldInspector.h/.cpp
  SolverInspector.h/.cpp
  ExportPreviewDialog.h/.cpp
  ValidationPanel.h/.cpp

src/app/
  main.cpp
  ApplicationContext.h/.cpp
```

### 5.4 关键设计原则

- 模型是唯一业务数据源。
- VTK actor、树节点和控件都从模型派生。
- 通过稳定 ID 关联模型对象、树节点和 VTK actor。
- 字典写入器是纯函数式或服务式的，不依赖当前窗口控件。
- 每个 writer 都能脱离 GUI 进行测试。
- 所有 schema 变化带版本号和迁移路径。
- 不在 UI 类里拼接 OpenFOAM 字典。

---

## 6. 数据模型

### 6.1 工程文件

建议扩展名：`.ofgproj`，内容为 UTF-8 JSON。

示例结构：

```json
{
  "schemaVersion": 1,
  "projectName": "cavity",
  "caseDirectory": "cases/cavity",
  "domain": {
    "mode": "3d",
    "min": [0.0, 0.0, 0.0],
    "max": [0.1, 0.1, 0.01],
    "units": "m"
  },
  "mesh": {
    "cells": [20, 20, 1],
    "grading": [1.0, 1.0, 1.0]
  },
  "patches": [],
  "fields": [],
  "solver": {
    "application": "icoFoam",
    "endTime": 0.5,
    "deltaT": 0.005
  }
}
```

实际字段以实现时的模型定义为准。必须避免保存仅属于某个 UI 控件的临时状态。

### 6.2 核心不变量

- 计算域最大点在每个轴上都必须大于最小点。
- 每个轴网格数必须大于零。
- 2D 模式下必须有一个厚度方向只划分一层。
- 六个面必须被唯一 patch 覆盖。
- patch 名称必须唯一且合法。
- 每个启用的场在每个 patch 上必须有边界条件。
- 导出前所有必填字段必须有值。
- 文件写入必须使用 LF。
- 工程文件中的 schemaVersion 必须可识别。

### 6.3 编辑命令

首版可直接通过 DocumentController 修改模型，但接口不应阻止后续加入撤销/重做。

后续采用命令式编辑：

- `SetDomainCommand`
- `RenamePatchCommand`
- `AssignFaceCommand`
- `SetPatchTypeCommand`
- `SetBoundaryConditionCommand`
- `SetMeshSettingsCommand`

每个命令包含验证、执行和撤销信息。

---

## 7. 分阶段开发过程

### M0：技术基线和应用骨架

目标：证明 Qt 6、VTK 9.6、MinGW 和 CMake 可以在本机稳定编译运行。

任务：

- 创建根 `CMakeLists.txt`。
- 添加 `CMakePresets.json`。
- 创建 `OpenFOAMGUI` 可执行目标。
- 创建 `QMainWindow`。
- 添加菜单栏、工具栏、状态栏。
- 添加左侧 Dock 和右侧 VTK 视口。
- 使用 `QVTKOpenGLNativeWidget` 显示彩色立方体。
- 设置 CMake 自动初始化和 VTK 模块依赖。
- 配置 Debug 构建。
- 编写 VTK 场景 smoke test。
- 确认运行时能找到 Qt 和 VTK DLL。

验收：

- 应用可启动。
- 三维视图可见。
- 鼠标可旋转、平移、缩放。
- 关闭程序无崩溃。
- 构建和测试命令写入文档。
- 不需要 OpenFOAM 环境即可运行。

### M1：工程模型与持久化

目标：建立 GUI 的业务数据源。

任务：

- 实现 `CaseProject`、`DomainBox`、`MeshSettings`。
- 实现 JSON 保存和加载。
- 实现 schemaVersion。
- 实现工程新建、保存、打开、另存为。
- 实现 dirty 状态和关闭提示。
- 实现基础校验问题列表。
- 添加模型和 JSON 往返测试。

验收：

- 修改域尺寸后保存再打开，数据一致。
- 非法 JSON 有可读错误。
- 未知 schemaVersion 不会被静默覆盖。
- 应用重启后可恢复工程。

### M2：计算域编辑器与 VTK 联动

目标：打通“输入参数 -> 模型 -> 三维视图”的完整链路。

任务：

- 实现计算域属性面板。
- 实现 min/max 和网格数编辑。
- 实现 2D/3D 模式。
- 实现长方体面 actor。
- 实现颜色、边线、坐标轴和方向标记。
- 实现适配视图。
- 实现输入校验高亮。
- 编写模型变化触发场景重建的测试。

验收：

- 修改 min/max 后三维尺寸立即更新。
- 修改网格数不改变几何外形。
- 非法尺寸不更新场景且显示错误。
- 2D/3D 模式行为明确。

### M3：边界面编辑与拾取

目标：完成类似 StarCCM+ 的边界设置体验。

任务：

- 建立六个规范面的稳定枚举。
- 实现 patch 模型和面归属。
- 实现工程树边界节点。
- 实现边界属性面板。
- 为六个面创建独立 actor。
- 实现树选中到三维高亮。
- 实现三维拾取回到树选中。
- 实现 patch 名称和类型校验。
- 实现颜色和可见性。
- 添加面覆盖和重复归属测试。

验收：

- 六个面都能独立选中。
- 树和三维视图选中状态双向同步。
- 修改名称后树和属性同步更新。
- 修改 patch 类型后视觉标签更新。
- 同一面不能被重复分配。
- 所有面未覆盖时显示明确错误。

### M4：场、边界条件和字典预览

目标：生成可检查的 OpenFOAM 文件内容。

任务：

- 实现场模型和边界条件模型。
- 实现 p/U 基础编辑界面。
- 实现 dimensions、internalField 和 BC 参数编辑器。
- 实现 `BlockMeshDictWriter`。
- 实现 `ControlDictWriter`。
- 实现 `fvSchemes` 和 `fvSolution` 基础写入。
- 实现 `transportProperties` 写入。
- 实现 p/U 场文件写入。
- 实现导出预览对话框。
- 使用 v2206 教程字典建立黄金文件测试。

验收：

- 修改参数后预览文本发生变化。
- 生成文件结构正确。
- patch 名称、类型和场边界条件一致。
- 文本使用 LF。
- 输出可被人工在 OpenFOAM v2206 教程环境中检查。
- GUI 启动和导出不依赖 OpenFOAM 安装。

### M5：安全导出和完整工作流

目标：完成可交付的首版纵向业务流程。

任务：

- 实现导出目录选择。
- 实现临时目录生成。
- 实现覆盖检测和备份。
- 实现失败清理和原子替换。
- 实现导出成功/失败日志。
- 实现首次启动示例工程。
- 实现最近打开列表。
- 实现基础设置页。
- 完善错误文案。
- 进行跨 DPI 和窗口缩放检查。

验收：

- 从空工程到导出完成无需手工编辑文件。
- 导出中断不会破坏已有 case。
- 文件覆盖有明显警告。
- 所有错误都能在 UI 中定位。
- 示例工程可重复打开和导出。

### M6：可持续性与增强

目标：让项目后续可扩展，而不是变成不可维护的单体界面。

候选任务：

- 撤销/重做。
- 原始字典高级编辑模式。
- 导入已有 case 的基础元数据。
- 读取 `polyMesh` 显示真实网格。
- 读取时间目录显示结果场。
- 增加 `simpleFoam` 模板。
- 增加 turbulence 字段。
- 增加 patch 分组。
- 增加多块网格。
- 增加插件式 solver template。
- 增加深色主题、快捷键、翻译。

M6 的每一项都应作为独立任务派发，不应合并成一次大改。

---

## 8. 开发、构建和部署过程

### 8.1 首次搭建

1. 创建 CMake 工程。
2. 设置 C++20。
3. 查找 Qt6 Widgets 和 Qt6 Test。
4. 查找 VTK 组件。
5. 创建核心静态库、渲染静态库、IO 静态库和可执行程序。
6. 配置 `AUTOMOC`、`AUTOUIC`、`AUTORCC`。
7. 配置 VTK 自动初始化。
8. 添加 `enable_testing()` 和测试目标。
9. 将 Qt、VTK、MinGW 路径集中到 CMake preset。

### 8.2 日常开发

```powershell
cmake --preset windows-mingw-debug
cmake --build --preset windows-mingw-debug
ctest --preset windows-mingw-debug --output-on-failure
```

实际命令使用固定绝对路径的 CMake 和 Ninja，或者将 Qt/MinGW 工具目录加入当前 PowerShell 的 PATH。

### 8.3 运行

- 开发期直接运行 `build/OpenFOAMGUI.exe`。
- 确保 `VTK\install\bin`、Qt `bin` 和 MinGW runtime DLL 在 PATH 中。
- 发布前使用 `windeployqt` 收集 Qt DLL。
- 单独收集需要的 VTK DLL。
- 在干净 Windows 环境验证启动。

### 8.4 发布前检查

- Release 构建成功。
- 测试全部通过。
- 示例工程可以打开和导出。
- 没有绝对开发路径写入二进制资源。
- 应用图标、版本号和关于页面存在。
- 依赖 DLL 完整。
- 覆盖导出测试通过。
- 崩溃日志或错误日志路径明确。

---

## 9. 测试与质量保证

### 9.1 测试金字塔

```text
           手动 UI 验收
         VTK/UI smoke test
      黄金文件 / 集成测试
     模型 / 校验单元测试
```

### 9.2 必需测试

- `DomainBox` 有效性。
- 网格数量校验。
- 六面覆盖唯一性。
- patch 名称规范化。
- patch 类型与 2D/3D 约束。
- 场和 BC 完整性。
- JSON 保存/加载往返。
- 每个字典 writer 的黄金文件。
- 临时目录和原子替换。
- 主窗口启动。
- VTK 场景重建。
- 面拾取和选择同步。

### 9.3 质量门禁

每个 PR/任务至少满足：

- 编译无新增错误。
- 相关测试通过。
- 无新增 OpenFOAM 编译依赖。
- 无无关大范围重构。
- 行为变更伴随测试或明确说明无法自动测试的原因。
- UI 变更手动验证通过。

---

## 10. 可持续 Agent 开发路径

### 10.1 为什么需要专门设计

这个项目包含多个容易失控的方向：

- UI 组件越来越多。
- VTK 场景和业务模型容易互相耦合。
- OpenFOAM 字典格式复杂。
- 功能容易从“长方体界面”扩展成完整 CFD 平台。
- Agent 容易在单次任务中大范围重构。

因此必须把开发过程本身设计成可重复、可交接、可验证的流程。

### 10.2 固定文档体系

- `AGENTS.md`：所有 Agent 的硬规则、构建命令、完成标准。
- `docs/DEVELOPMENT_PLAN.md`：产品范围、阶段和交付物。
- `docs/ARCHITECTURE.md`：代码分层、数据模型和技术决策。
- `docs/STATUS.md`：当前阶段、已完成、进行中、阻塞和下一步。
- `docs/TASKS.md`：可派发的任务队列和验收标准。
- `docs/DECISIONS.md`：架构决策记录，避免后续反复争论。

### 10.3 单次 Agent 工作循环

```text
读取规则
  ↓
选择一个任务 ID
  ↓
检查代码和测试
  ↓
声明目标/非目标/验收标准
  ↓
实现最小纵向切片
  ↓
构建 + 自动测试 + 手动 smoke test
  ↓
更新状态/任务/决策文档
  ↓
按统一格式交接
```

### 10.4 Agent 任务粒度

推荐一个任务满足：

- 1 到 3 个相关文件模块。
- 有明确用户可见结果或可测试行为。
- 可以在一次会话内完成。
- 不跨越两个以上里程碑。
- 不包含“顺便”重构其他模块。

不建议的任务：

- “把所有 OpenFOAM 文件都支持了”。
- “重写整个架构”。
- “实现完整 ParaView”。
- “同时增加 simpleFoam、湍流和 STL”。

推荐的任务：

- “为 DomainBox 增加 min/max 校验并显示到 DomainInspector”。
- “让六个 face actor 支持点击后更新 ProjectTree 选中项”。
- “实现 blockMeshDictWriter 并添加 3 个黄金文件测试”。
- “实现导出前临时目录和失败回滚”。

### 10.5 Agent 禁止事项

- 不得编译或链接 OpenFOAM。
- 不得修改 `OpenFOAM-v2206/`。
- 不得为了跑通界面而引入 Python/PySide 作为替代实现，除非用户重新明确技术栈。
- 不得用硬编码字符串冒充文件生成逻辑。
- 不得把 VTK actor 当作模型状态。
- 不得在没有测试的情况下大改数据模型。
- 不得把未实现按钮做成点击后静默无反应。
- 不得隐藏导出错误。
- 不得删除用户文件或未经确认覆盖 case。

### 10.6 任务交接要求

每个 Agent 完成任务后必须留下以下信息：

```text
状态：完成 / 部分完成 / 阻塞
任务 ID：
完成内容：
修改文件：
验证结果：
已知问题：
下一步建议：
```

同时更新：

- `docs/STATUS.md`：当前真实状态。
- `docs/TASKS.md`：任务状态和后续任务。
- `docs/DECISIONS.md`：只在有架构决策时更新。

### 10.7 每阶段结束的集成检查

- 是否能从空工程走到当前阶段承诺的最终用户动作。
- 是否仍然不依赖 OpenFOAM 安装。
- 是否有任务被错误合并，导致 UI、IO 和模型层交叉依赖。
- 是否有文档与实际代码不一致。
- 是否需要调整架构决策。

---

## 11. 风险与应对

| 风险 | 影响 | 应对 |
|---|---|---|
| Qt/VTK ABI 或运行时 DLL 不匹配 | 程序无法启动或视口崩溃 | M0 先做最小 VTK 程序并验证部署 |
| UI 和 VTK 状态双向同步混乱 | 选择/编辑结果不一致 | 模型为唯一数据源，使用稳定 ID |
| 范围膨胀为完整 StarCCM+ | 长期无法交付首版 | 首版锁定长方体、icoFoam、基础 BC |
| OpenFOAM 字典格式错误 | 导出不可用 | 以 v2206 教程为黄金文件样本 |
| 覆盖导出损坏用户数据 | 数据丢失 | 临时目录、备份、原子替换 |
| 高低 DPI 下布局错乱 | 可用性差 | M5 做跨 DPI smoke test |
| 后续支持多求解器导致 UI 失控 | 维护成本高 | 使用模板注册表和分阶段扩展 |
| Agent 大范围重构 | 回归风险 | 单任务小切片、测试先行、文档约束 |

---

## 12. 推荐默认决策

在用户没有提出相反意见时，后续开发采用：

1. 第一阶段只实现单块长方体。
2. 第一阶段只实现 3D 单块和可选 2D 单层模式。
3. 第一阶段只生成 p、U、transportProperties、controlDict、fvSchemes、fvSolution、blockMeshDict。
4. 第一阶段不做任何 OpenFOAM 进程调用。
5. GUI 工程文件作为持久化主状态。
6. OpenFOAM 文件由独立 writer 生成。
7. VTK 视图只负责显示和拾取，不保存业务状态。
8. 所有后续功能先进入 `docs/TASKS.md`，再按任务实施。

---

## 13. 第一项正式开发任务

任务 ID：`M0-01`

目标：

> 建立可运行的 Qt 6.11.2 + VTK 9.6.2 + MinGW + CMake/Ninja 最小桌面工程，显示一个带六个可独立着色的面的计算域预览，并完成构建/运行说明。

非目标：

- 不实现工程保存。
- 不实现 OpenFOAM 字典。
- 不实现真实网格。
- 不调用 OpenFOAM。
- 不增加复杂主题和菜单业务。

验收标准：

- 应用可以启动。
- 右侧显示三维长方体。
- 六个面颜色不同。
- 鼠标可以旋转、平移、缩放。
- 左侧有工程树占位和属性面板占位。
- Debug 构建成功。
- 启动不依赖 OpenFOAM。