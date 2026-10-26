# OpenFOAM GUI Project Instructions

本文件是后续所有人类开发者和 Codex/Agent 在本项目中工作时必须优先阅读的项目级规则。

## 1. 项目定位

本项目开发一个独立的 Windows 桌面 GUI：

- 技术栈：C++20、Qt 6 Widgets、VTK、CMake、Ninja、MinGW。
- 产品用途：创建、编辑、预览和导出 OpenFOAM case 的可视化前处理界面。
- UI 风格：左侧配置区类似 StarCCM+，右侧三维视图类似轻量级 ParaView。
- 首版核心工作流：
  `新建工程 -> 定义长方体计算域 -> 设置网格 -> 编辑边界面 -> 设置场和边界条件 -> 三维检查 -> 预览并导出 OpenFOAM 文件`。

## 2. 明确的硬边界

### 必须遵守

- 本项目的应用代码是独立 GUI 程序。
- 不编译 OpenFOAM。
- 不链接 OpenFOAM 的 C++ 库。
- 不包含 OpenFOAM 头文件。
- 不使用 `wmake` 或 OpenFOAM 的构建系统。
- 不把 `OpenFOAM-v2206/` 加入 CMake 的 `add_subdirectory()` 或编译源文件列表。
- `OpenFOAM-v2206/` 仅作为 v2206 文件格式、示例字典和测试参照。
- GUI 必须能够在不需要本机安装 OpenFOAM 的情况下启动、编辑、浏览和导出。
- 首版不要求调用 `blockMesh`、`checkMesh`、求解器或 ParaView。
- 如未来增加外部进程调用，必须作为独立适配层实现，不能破坏上述独立运行能力。

### 首版不做

- OpenFOAM 源码编译或嵌入式求解。
- 任意 CAD/STEP/STL 几何导入。
- 任意曲面拾取和拓扑重建。
- 多块结构网格。
- snappyHexMesh 配置。
- 完整 ParaView 后处理。
- 多相流、燃烧、电磁等复杂物理模板。
- 云端协作、账号系统、数据库服务。
- 自动求解和残差监控。

## 3. 固定环境路径

这些路径已确认，后续不要重复询问用户。

- Qt SDK：`D:\3.wpsAndOther\Qt\setting\6.11.2\mingw_64`
- qmake：`D:\3.wpsAndOther\Qt\setting\6.11.2\mingw_64\bin\qmake.exe`
- Qt Creator：`D:\3.wpsAndOther\Qt\setting\Tools\QtCreator\bin\qtcreator.exe`
- CMake：`D:\3.wpsAndOther\Qt\setting\Tools\CMake_64\bin\cmake.exe`
- Ninja：`D:\3.wpsAndOther\Qt\setting\Tools\Ninja\ninja.exe`
- MinGW g++：`D:\3.wpsAndOther\Qt\setting\Tools\mingw1310_64\bin\g++.exe`
- VTK：`D:\3.wpsAndOther\VTK\install`

已核对版本：

- Qt `6.11.2`
- CMake `3.30.5`
- VTK `9.6.2`，启用 Qt 6 支持
- VTK 为动态库构建，存在 `QVTKOpenGLNativeWidget` 和 `vtkGUISupportQt`

不要使用 README 中旧的 `D:\3.wpsAndOther\Cmake\setting\bin` CMake 路径作为本项目默认工具链。

## 4. 计划中的代码结构

```text
D:\6.developOpenFOAM\
├── AGENTS.md
├── README.md
├── CMakeLists.txt                 # M0 创建
├── CMakePresets.json              # M0 创建
├── cmake/
├── src/
│   ├── app/                       # main、应用启动、依赖装配
│   ├── core/                      # 与 Qt Widgets/VTK 无关的业务模型
│   ├── io/                        # 工程 JSON、OpenFOAM 字典写入
│   ├── render/                    # VTK 场景、拾取、高亮
│   └── ui/                        # 窗口、面板、对话框、Model/View
├── resources/
│   ├── templates/openfoam-v2206/
│   └── icons/
├── tests/                         # Qt Test 单元测试和黄金文件测试
├── docs/
│   ├── DEVELOPMENT_PLAN.md
│   ├── ARCHITECTURE.md
│   ├── STATUS.md
│   ├── TASKS.md
│   └── DECISIONS.md
└── OpenFOAM-v2206/                # 只读参考，不参与编译
```

不要移动、删除或改写 `OpenFOAM-v2206/`。

## 5. 架构约束

- `core` 不得依赖 Qt Widgets、VTK 或文件系统 UI 对话框。
- `render` 可以依赖 VTK 和 Qt GUI，但不得成为业务数据源。
- `io` 负责工程文件和 OpenFOAM 文件写入，不负责界面交互。
- `ui` 只负责显示、输入和调用应用服务，不能把 OpenFOAM 字典字符串直接拼在窗口类中。
- VTK actor 和界面控件都由领域模型驱动。
- 业务对象使用稳定 ID，界面显示名称可修改；VTK 拾取映射必须使用 ID，不能使用显示名称。
- 工程 JSON 是 GUI 状态的持久化来源；OpenFOAM case 是导出产物。
- 所有文件输出必须经过校验，并使用临时目录/临时文件后原子替换。
- 生成 OpenFOAM 字典时使用 LF 换行，字典文本优先使用 ASCII。
- 不允许在源码中散落绝对路径；路径通过 CMake、设置页或运行时配置提供。
- 新增第三方依赖前必须写入 `docs/DECISIONS.md`。

## 5.1 Git 更新规则

- `main` 始终保存最新版本。
- 每次版本更新前，将旧的 `main` 归档为 `archive/<时间>-<旧main短SHA>`。
- 新增提交必须落在 `main`。
- 归档分支只读，不在 `archive/*` 上继续开发。
- 每个开发任务优先产生一个完整的 `main` 提交。
- 更新提交使用：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File '.\scripts\Commit-Update.ps1' -Message '<message>'
```

- 配置远程仓库后，可增加 `-Push` 同时推送归档分支和 `main`。
- `OpenFOAM-v2206/` 是本地只读参考，不进入 Git。
- 详细规则见 `docs/GIT_WORKFLOW.md`。
## 5.2 GitHub 发布触发词

当用户只发送以下任意表达时：

- `提交到github`
- `提交到 GitHub`
- `上传到github`
- `上传到 GitHub`
- `发布到github`
- `发布到 GitHub`

Agent 必须将其视为立即执行 GitHub 发布流程，不再询问分支、提交信息或推送步骤。

自动执行规则：

1. 检查当前分支为 `main`，工作区存在待提交修改。
2. 如本次任务修改了代码，先完成构建和测试。
3. 根据实际修改自动生成清晰的提交信息。
4. 运行 `scripts/Publish-ToGitHub.ps1 -Message '<自动生成的提交信息>'`。
5. 验证远程 `main` 与本地 `main` 的提交一致。
6. 验证本次归档分支已推送到远程。
7. 向用户报告提交 SHA 和归档分支名。

只有以下情况需要停下来询问：

- 没有可提交的修改。
- Git 认证失败。
- 远程仓库不存在或无法访问。
- 远程 `main` 已分叉，不能快进推送。
- 合并冲突或工作区状态不安全。

禁止强制推送 `main`，禁止跳过归档直接推送。
## 6. 标准构建与测试命令

以下命令在 M0 建立 CMake 工程后生效。

```powershell
$env:Path = 'D:\3.wpsAndOther\Qt\setting\6.11.2\mingw_64\bin;D:\3.wpsAndOther\Qt\setting\Tools\mingw1310_64\bin;D:\3.wpsAndOther\Qt\setting\Tools\Ninja;D:\3.wpsAndOther\VTK\install\bin;' + $env:Path

& 'D:\3.wpsAndOther\Qt\setting\Tools\CMake_64\bin\cmake.exe' `
  -S . -B build -G Ninja `
  -DCMAKE_BUILD_TYPE=Debug `
  -DCMAKE_PREFIX_PATH='D:\3.wpsAndOther\Qt\setting\6.11.2\mingw_64;D:\3.wpsAndOther\VTK\install' `
  -DCMAKE_C_COMPILER='D:\3.wpsAndOther\Qt\setting\Tools\mingw1310_64\bin\gcc.exe' `
  -DCMAKE_CXX_COMPILER='D:\3.wpsAndOther\Qt\setting\Tools\mingw1310_64\bin\g++.exe'

& 'D:\3.wpsAndOther\Qt\setting\Tools\Ninja\ninja.exe' -C build
& 'D:\3.wpsAndOther\Qt\setting\Tools\CMake_64\bin\ctest.exe' --test-dir build --output-on-failure
```

优先补充 `CMakePresets.json`，之后使用：

```powershell
& 'D:\3.wpsAndOther\Qt\setting\Tools\CMake_64\bin\cmake.exe' --preset windows-mingw-debug
& 'D:\3.wpsAndOther\Qt\setting\Tools\CMake_64\bin\cmake.exe' --build --preset windows-mingw-debug
& 'D:\3.wpsAndOther\Qt\setting\Tools\CMake_64\bin\ctest.exe' --preset windows-mingw-debug
```

### 6.1 项目脚本

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File '.\scripts\Build-Project.ps1' -Configuration Debug
powershell.exe -NoProfile -ExecutionPolicy Bypass -File '.\scripts\Build-Project.ps1' -Configuration Release
powershell.exe -NoProfile -ExecutionPolicy Bypass -File '.\scripts\Run-Application.ps1' -Configuration Debug
powershell.exe -NoProfile -ExecutionPolicy Bypass -File '.\scripts\Deploy-Release.ps1' -BuildFirst
powershell.exe -NoProfile -ExecutionPolicy Bypass -File '.\scripts\Publish-ToGitHub.ps1' -Message 'chore: publish update'
```

构建、部署和 DLL 收集细节见 `docs/BUILD_AND_DEPLOY.md`。
## 7. 每个开发任务的固定流程

Agent 开始任务前必须：

1. 阅读 `AGENTS.md`、`docs/STATUS.md`、`docs/TASKS.md`。
2. 阅读和本任务相关的 `docs/ARCHITECTURE.md`、`docs/DECISIONS.md`。
3. 选定一个明确的任务 ID，不接受“顺手把所有功能都做了”。
4. 检查当前工作区状态，不覆盖用户未提交的修改。
5. 用简短文字说明：目标、假设、非目标、验收标准、预计修改范围。

开发过程中必须：

1. 以可运行的纵向切片为单位实现，而不是只增加空类或纯 UI 占位。
2. 业务逻辑放在 `core`/`io`，UI 通过信号槽调用服务。
3. 修改行为时同步增加或更新测试。
4. 保持改动范围收敛，不进行无关格式化或大规模重构。
5. 遇到范围外需求，写入 `docs/TASKS.md` 的 Backlog，不在当前任务中实现。
6. 遇到真实阻塞时在 `docs/STATUS.md` 记录阻塞、复现步骤和需要的决策。

任务完成前必须：

1. 构建成功。
2. 相关测试通过。
3. 手动验证受影响 UI，至少覆盖正常流程和一个错误流程。
4. 更新 `docs/STATUS.md` 和 `docs/TASKS.md`。
5. 如改变架构或依赖，更新 `docs/DECISIONS.md`。
6. 给出交接摘要：完成内容、修改文件、测试结果、已知问题、下一任务。

## 8. Agent 任务合同模板

每次向 Agent 派发任务时使用以下格式：

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

任务描述不满足以上信息时，Agent 应先补全假设并在回复中说明，而不是扩大实现范围。

## 9. Definition of Done

一个任务只有同时满足以下条件才算完成：

- 验收标准逐项满足。
- Debug 构建成功。
- 相关自动化测试通过。
- 新增用户可见功能有手动 smoke test。
- 没有引入 OpenFOAM 编译/链接依赖。
- 没有硬编码本机绝对路径到业务源码。
- 文件写入具有错误处理和覆盖保护。
- 文档、状态和任务清单已更新。
- 没有把未完成功能伪装成可用功能。

## 10. UI 与代码规范

- 使用 Qt Widgets，不使用 QML，首版不混用两套 UI 技术。
- 所有用户可见文本通过 `tr()`，为中文界面和后续翻译保留能力。
- 界面术语：技术名词保留 OpenFOAM 原文，例如 `patch`、`fixedValue`、`zeroGradient`。
- VTK 渲染必须在 GUI 主线程进行。
- 禁止在渲染循环中执行文件 I/O。
- 公共类名、文件名和目录名使用英文；文档和界面可以使用中文。
- 头文件使用 `#pragma once`。
- 尽量使用值语义和小型服务类，避免全局单例和 God Object。
- 先写清楚不变量，再实现编辑逻辑。

## 11. 测试规则

- 使用 Qt Test，避免额外测试框架依赖。
- 纯模型测试：坐标、网格参数、边界归属、命名和校验。
- 黄金文件测试：给定工程模型，比较生成的 OpenFOAM 字典文本。
- UI smoke test：启动、新建、编辑、切换视图、导出预览。
- VTK smoke test：创建场景、更新域尺寸、切换边界颜色、拾取面，不要求像素级截图一致。
- 每个 bug 修复必须尽量增加一个能复现该 bug 的测试。

## 12. 交接格式

每次任务结束必须使用以下格式：

```text
状态：完成 / 部分完成 / 阻塞
任务 ID：
完成内容：
修改文件：
验证结果：
已知问题：
下一步建议：
```

## 13. 当前状态

- 当前阶段：M0 技术基线，已完成。
- 已完成：M0-01、M0-02、M0-03、M0-04。
- 当前任务：准备进入 M1 工程模型与持久化。
- Git 分支策略：旧 `main` 自动归档，新提交进入 `main`。
- GitHub 触发词：`提交到github` 即自动归档、提交并推送。
- 远程仓库：`https://github.com/byChen47/OpenFOAMInteFace.git`。
- Git 更新规则：见 `docs/GIT_WORKFLOW.md`。
- 构建和部署脚本位于 `scripts/`。
- 构建与部署说明：见 `docs/BUILD_AND_DEPLOY.md`。
- 详细计划：见 `docs/DEVELOPMENT_PLAN.md`。
- 实时状态：见 `docs/STATUS.md`。
- 任务队列：见 `docs/TASKS.md`。