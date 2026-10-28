# 架构决策记录

本文件记录会影响长期开发的决策。后续 Agent 不应在没有新证据的情况下反复推翻这些决定；如需修改，新增一条 ADR 并说明迁移方案。

---

## D-001：GUI 独立于 OpenFOAM 构建系统

- 日期：2026-10-25
- 状态：接受
- 决策：应用使用独立 CMake 工程，不编译、不链接、不嵌入 OpenFOAM。
- 原因：
  - 目标是开发可视化前处理界面。
  - Windows 上编译和链接完整 OpenFOAM 工具链成本高。
  - OpenFOAM v2206 在 Windows 原生环境并非本项目的运行前提。
  - GUI 必须能在没有 OpenFOAM 安装的机器上启动和导出文件。
- 后果：
  - 字典格式通过 writer 自行生成。
  - OpenFOAM-v2206 只作为格式参考。
  - 未来如需运行求解器，使用独立外部进程适配层，不影响主程序。

---

## D-002：使用 Qt 6 Widgets

- 日期：2026-10-25
- 状态：接受
- 决策：首版使用 Qt Widgets，不使用 QML。
- 原因：
  - 工程树、属性面板、Dock、菜单和表格等桌面交互成熟。
  - 与 VTK 的 `QVTKOpenGLNativeWidget` 集成直接。
  - 便于后续 Agent 维护和测试。
- 后果：
  - UI 主要使用 QWidget、QLayout、QAction、QDialog。
  - 不在首版混用 Qt Quick。

---

## D-003：VTK 使用 Qt6 原生控件

- 日期：2026-10-25
- 状态：接受
- 决策：使用 `QVTKOpenGLNativeWidget` 作为三维视口。
- 原因：
  - 本机 VTK 9.6.2 已启用 Qt6 支持。
  - 与 Qt Widgets 集成路径清晰。
- 后果：
  - CMake 必须正确查找 VTK 的 Qt 组件。
  - 需要验证 VTK/Qt 编译器和运行时 ABI。
  - 需要显式部署 VTK DLL。

---

## D-004：首版只支持单块长方体计算域

- 日期：2026-10-25
- 状态：接受
- 决策：首版计算域由 min/max 定义，六个面可编辑；不做任意几何、STL、多块网格。
- 原因：
  - 与用户当前需求直接对应。
  - 可以在较短周期内打通完整 UI 和导出链路。
  - 为后续几何抽象保留接口。
- 后果：
  - 面选择使用 FaceId 枚举。
  - 不实现通用拓扑和 CAD kernel。
  - 任意几何进入 backlog。

---

## D-005：GUI 工程文件作为持久化主状态

- 日期：2026-10-25
- 状态：接受
- 决策：使用 `.ofgproj` JSON 保存 GUI 工程；OpenFOAM case 是导出产物。
- 原因：
  - 需要保存颜色、可见性、树选择等 GUI 状态。
  - OpenFOAM case 目录结构不足以完整表达 GUI 工程。
  - JSON 便于调试、迁移和测试。
- 后果：
  - 必须维护 schemaVersion。
  - 导出逻辑与工程保存逻辑分离。
  - 后续需要实现旧 schema 到新 schema 的迁移。

---

## D-006：OpenFOAM 字典使用专用 writer 生成

- 日期：2026-10-25
- 状态：接受
- 决策：使用 C++ writer 生成字典，不在 UI 中拼接字符串。
- 原因：
  - 便于单元测试和黄金文件测试。
  - 可复用同一 builder 实现预览和导出。
  - 避免界面控件直接决定文件格式。
- 后果：
  - 每个字典文件有明确的 writer。
  - 写文件前进行模型和字典校验。
  - 预览和实际导出必须共用同一生成路径。

---

## D-007：模型是唯一业务数据源

- 日期：2026-10-25
- 状态：接受
- 决策：VTK actor、树节点和控件均从模型派生。
- 原因：
  - 避免选择状态、名称和边界条件出现多份不一致状态。
  - 支持保存、加载、撤销和测试。
- 后果：
  - VTK 拾取结果必须先映射回模型 ID。
  - 不能直接依赖 actor 名称存储业务状态。
  - Render 层只做显示适配。

---

## D-008：首版不调用 OpenFOAM 进程

- 日期：2026-10-25
- 状态：接受
- 决策：首版不集成 `blockMesh`、`checkMesh` 或求解器调用。
- 原因：
  - 用户当前只要求可视化界面。
  - 避免 WSL、路径、环境变量和进程管理过早增加复杂度。
  - 先验证 GUI、模型和文件生成链路。
- 后果：
  - 首版验收不要求安装 OpenFOAM。
  - 生成文件可交由用户自行检查。
  - 进程适配层作为未来独立模块。

---

## D-009：首版求解器模板为 icoFoam

- 日期：2026-10-25
- 状态：接受
- 决策：首版围绕 p、U 和 icoFoam 基础字典验证完整流程。
- 原因：
  - 场数量少，边界条件可控。
  - 适合验证文件生成和 UI 数据流。
  - 避免同时引入湍流模型和多场耦合。
- 后果：
  - 首版字典和字段 UI 仅承诺 icoFoam 基础模板。
  - simpleFoam、湍流和多相流进入后续任务。

---

## D-010：测试使用 Qt Test

- 日期：2026-10-25
- 状态：接受
- 决策：使用 Qt Test 编写单元测试和 UI smoke test。
- 原因：
  - Qt 已是必选依赖。
  - 不增加额外测试框架依赖。
  - 可直接测试模型、文件和部分控件。
- 后果：
  - 每个测试模块是可独立运行的 Qt Test 目标。
  - 通过 CTest 统一运行。
  - 字典输出使用黄金文件对比。

---

## D-011：首版使用 3D 单块基础模式，并预留 2D

- 日期：2026-10-25
- 状态：接受
- 决策：产品模型支持 2D/3D 标记；首个可交付实现优先验证 3D 单块，再补 2D 约束。
- 原因：
  - min/max 三维域与当前需求一致。
  - 2D 是常见 OpenFOAM 用法，但会增加 patch 规则。
- 后果：
  - DomainBox 从一开始保留 mode。
  - 2D 规则通过校验器和测试实现。
  - 不允许在首版中以隐式方式忽略厚度方向。

---

## D-012：不移动或修改现有 OpenFOAM-v2206 目录

- 日期：2026-10-25
- 状态：接受
- 决策：保持现有源码目录原位，应用构建不引用它。
- 原因：
  - 避免破坏用户已有资料。
  - 降低 Agent 误编译或误修改风险。
- 后果：
  - 不以源码目录作为构建输入。
  - 如需参考字典，只读访问。
  - 测试样例复制到 `resources/templates` 或 `tests/fixtures` 中。

---

## D-013：Windows 部署使用 windeployqt 加 PE 依赖闭包

- 日期：2026-10-25
- 状态：接受
- 决策：Release 部署先调用 `windeployqt` 处理 Qt 和编译器运行库，再通过 `objdump` 递归扫描 PE 导入表收集 VTK/MinGW DLL。
- 原因：
  - VTK 组件较多，手工维护 DLL 列表容易遗漏。
  - 新增 VTK 模块后应自动重新收集依赖。
  - 最小 PATH 自动 smoke test 可以发现部署遗漏。
- 后果：
  - 部署脚本必须位于 `scripts/` 并由文档维护。
  - 每次新增二进制依赖后重新执行部署脚本。
  - `dist/` 是生成物，不进入 Git。
---

## D-014：OpenFOAM-v2206 不进入 Git

- 日期：2026-10-26
- 状态：接受
- 决策：`OpenFOAM-v2206/` 作为本地只读参考，不纳入 GUI 仓库。
- 原因：
  - 应用不编译或链接 OpenFOAM。
  - 源码目录体积大，不属于 GUI 产物。
  - 避免误把第三方工具链纳入本项目版本历史。
- 后果：
  - `.gitignore` 排除 `/OpenFOAM-v2206/`。
  - 文档必须明确该目录不是运行依赖。
  - 如未来需要第三方版本锁定，另建子模块或独立说明。

---

## D-015：旧 main 归档，新版本保留在 main

- 日期：2026-10-26
- 状态：接受
- 决策：每次更新前从旧 `main` 创建 `archive/<时间>-<短SHA>` 分支，新提交继续写入 `main`。
- 原因：
  - 用户要求旧设置保留在独立分支。
  - `main` 始终代表最新版本，便于继续开发。
  - 自动命名避免人工版本分支冲突。
- 后果：
  - 使用 `scripts/Commit-Update.ps1` 提交完整更新。
  - `archive/*` 分支不可继续开发。
  - 配置远程后，归档分支和 `main` 一起推送。
---

## D-016：固定短语触发自动 GitHub 发布

- 日期：2026-10-26
- 状态：接受
- 决策：用户说“提交到github”或其等价表达时，Agent 自动执行验证、归档、提交、推送和远程校验。
- 原因：
  - 用户不希望每次重复说明分支和推送流程。
  - 固定触发词可以写入项目级 Agent 规则并长期复用。
  - 一键脚本减少手工步骤和遗漏归档分支的风险。
- 后果：
  - `AGENTS.md` 和 `docs/GIT_WORKFLOW.md` 必须保留触发词定义。
  - 日常发布使用 `scripts/Publish-ToGitHub.ps1`。
  - Agent 只在无修改、认证失败、远程分叉或冲突时停止询问。

---

## D-017：ValidationIssue 追加定位字段

- 日期：2026-10-27
- 状态：接受
- 决策：`core::ValidationIssue` 在既有字段（`code`、`severity`、`message`、`targetObjectId`、`targetField`、`suggestedFix`）之后**追加** `std::optional<SourceLocation> location`、`std::string nodePath`、`std::string snippet`、`std::string origin`，四个字段全部带默认值（空），既有构造点与聚合初始化保持可编译。
- 原因：
  - 导入/解析类问题必须能定位到“文件 + 行 + 列 + 字典路径”，否则用户无法修复外部算例（规格 §8.2）。
  - 全字段给默认值，避免改动既有构造点与 UI 绑定，迁移风险最小。
  - `origin`（`import`/`model`/`export`）让面板能区分问题来源，导出前只拦截相关来源。
- 影响范围：
  - `src/core/ValidationIssue.h` 与既有 `CaseValidator`/UI 绑定的构造调用。
  - 校验面板（`ValidationPanel`）新增“位置”列与双击跳转。
  - 序列化若涉及 issue（导出清单/报告），只写稳定的 `code`/`severity`/`message`/相对位置，不写绝对路径。

---

## D-018：导入场景导出目录名继承源目录

- 日期：2026-10-27
- 状态：接受
- 决策：导入已有算例时，`CaseProject` 记录源起始时间目录名（`startDirectory` = `0` 或 `0.orig`）；导出时**继承**该名称。新建工程（无源 case）仍按 `docs/ARCHITECTURE.md` §7.2 输出 `0.orig/`。
- 原因：
  - v2206 教程的 `Allrun` 普遍假定 `0.orig` 存在并 `cp -r 0.orig 0`；把源 `0/` 强行改名为 `0.orig/` 会破坏用户既有工作流。
  - 反向（把 `0.orig/` 改名为 `0/`）会让 `Allrun` 的恢复语义失效。
  - 继承是最小惊讶原则：导出目录结构与源目录一致，便于直接对比与回灌。
- 影响范围：
  - `CaseProject.startDirectory` 为必填模型字段；`.ofgproj` 持久化。
  - 导出器与 `ExportPreviewDialog` 按该字段决定输出子目录名。
  - `docs/ARCHITECTURE.md` §7.2 增加“导入场景例外”，新建工程路径不变。

---

## D-019：只读内容“原文保真”回写

- 日期：2026-10-27
- 状态：接受
- 决策：`ReadOnly`/`Recognized` 级别的字典内容在导出时按**源文本原样回写**，不做重排、重算或规范化：典型包括 `nonuniform List<...>` 大列表、`#{ … #}` verbatim 块、未展开的 `#include`/`#includeEtc` 行（连同行位置）、未知键与未知子树。
- 原因：
  - 首版不可能覆盖全部 OpenFOAM 语义（`$` 展开、函数对象、未知 BC），重新生成极易引入语义漂移。
  - 原文保真是唯一可自动验证的策略（逐字节或结构等价）。
  - 与 D-020（不展开 `#include`）配套：不展开 ⇒ 必须原样保留 ⇒ 不能被重排破坏。
- 影响范围：
  - writer 对只读节点必须持有源片段（offset/length 或原文），而非仅 AST 值。
  - 黄金测试须包含 `nonuniform`、`#{…#}`、`#include` 三类保真用例（规格 §9.6 G4/G5）。
  - 字段级 `ReadOnly` 不提供编辑控件。

---

## D-020：首版不展开 `#include` 家族、不解析 `polyMesh`、不执行 `#codeStream`

- 日期：2026-10-27
- 状态：接受
- 决策：
  1. 不展开 `#include`/`#includeEtc`/`#includeIfPresent`：识别为指令节点，标 `Partial` + `IMPORT-DIR-001`（Warning），原文保留。
  2. 不解析 `constant/polyMesh/` 的 `points`/`faces`/`owner`/`neighbour`/`boundary`：只探测存在性（含 `.gz`）与大小，导出镜像复制。
  3. 不执行 `#codeStream`（C++ 代码注入）与 `codedFixedValue` 的 `code` 块：只读原文，绝不编译或求值。
- 原因：
  - `#include` 展开需要 `$FOAM_CASE`/`$FOAM_ETC` 环境语义与包含顺序，首版无环境假设（GUI 不要求安装 OpenFOAM）。
  - `polyMesh` 是二进制/大体积格式，解析成本高且与首版“单块长方体”目标无关。
  - `#codeStream` 是任意代码执行入口，安全红线：桌面程序不得编译执行用户算例内嵌代码。
- 影响范围：
  - 导入报告必须显著列出“含未展开指令的文件”。
  - 若源含 `#include`，可编辑能力降级但导出仍可用（镜像/保真）。
  - 未来解禁需新 ADR，并补充环境变量与安全策略。

---

## D-021：拆出 `ofgui_core`/`ofgui_io` 静态库与“一文件一职责”规则

- 日期：2026-10-27
- 状态：接受
- 决策：
  1. `src/core` 编译为独立静态库 `ofgui_core`（纯 C++20，不链接 Qt/VTK），`src/io` 编译为 `ofgui_io`（`ofgui_core` + `Qt6::Core`），各自 `CMakeLists.txt` 维护。
  2. **一个子功能/一个类一个 `.h`/`.cpp` 文件**，禁止把多个职责塞进一个文件（用户硬性要求）。
  3. CMake 使用**显式源文件列表**，禁止 `file(GLOB ...)`；新增文件必须同步登记到对应 `CMakeLists.txt` 与测试目标。
  4. 单个源文件 ≤ 400 行（含注释）；超出必须在 `docs/CASE_IMPORT_SPEC.md` §10 记录拆分方案后再提交。
- 原因：
  - 用户要求“每个子功能各一个文件，方便后续定位和修改”。
  - 显式源列表让新增文件在评审 diff 中可见，避免 GLOB 导致的构建漂移。
  - 行数上限强制拆分，防止 `CaseProject.cpp` 式上帝文件。
- 影响范围：
  - 根 `CMakeLists.txt` 与 `src/core`、`src/io`、`src/ui` 的子 `CMakeLists.txt`。
  - 评审清单（规格 §10.8）把“文件是否超 400 行/是否混职责”列为必查项。
  - `tests/` 同样一文件一测试类。

---

## D-022：禁止导出到源目录

- 日期：2026-10-27
- 状态：接受
- 决策：导出目标必须与源 case 根不同，且不得位于源 case 目录内（任意层子目录都不允许，避免镜像/重写与源互为输入）。允许的目标形态：不存在的新目录，或已存在的**空目录**。当目标已存在且非空时，必须由用户在 `ExportPreviewDialog` 中显式勾选确认，并把旧目标改名为 `<target>.bak-<ts>` 后再原子替换；未确认则 `IMPORT-EXP-002`（Blocking）。
- 原因：
  - 导出到源目录内会让“读取源 → 写目标”的同一棵树在写入中途被改写，无法保证一致性与可回滚。
  - 源目录是用户唯一副本，任何原地覆盖都必须有备份与显式确认。
  - 备份 + 原子替换让失败可回滚（与 D-012、规格 §9.5 一致）。
- 影响范围：
  - `ExportOptions.targetDirectory` 的前置校验（`IMPORT-EXP-001`/`IMPORT-EXP-002`）。
  - `AtomicDirectoryWriter` 的备份命名与回滚路径。
  - 导出预览对话框的确认开关与警告文案。

---

## D-023：镜像（mirror）默认开启

- 日期：2026-10-27
- 状态：接受
- 决策：导出时，未被 GUI 接管（`Recognized`/`Unsupported`/`skip` 之外的其它未识别文件）默认**逐字节镜像复制**到目标目录，而不是丢弃。`ExportPreviewDialog` 必须列出将镜像的文件数与前若干个清单条目，并在清单非空时提示“本次导出不会修改 N 个文件”。
- 原因：
  - 用户导入的是**已有可用算例**，GUI 首版只接管 10 个左右的字典；丢弃未知文件会让导出的算例无法运行。
  - 保真优先于“干净输出”。
  - 列表可见即可控，避免静默复制带来的空间与安全问题。
- 影响范围：
  - `CaseMirror` 与 `OutputFileAction`（`mirror` 默认）。
  - 导出前显式提示（规格 §9.7）与清单 `.ofgport-export.json`。
  - 符号链接不跟随（避免越界复制），`.gz` 保持压缩。

---

## D-024：首版只读场不可改名/删除

- 日期：2026-10-27
- 状态：接受
- 决策：`ReadOnly`/`Recognized` 级别的场文件在首版**不可改名、不可删除**，只能查看与保真导出；只有 `Editable`（`p`、`U` 及项目新建的场）允许改名与删除。
- 原因：
  - 改名会让同目录其它文件的 `boundaryField` 键、`fvSolution.solvers{}` 键、`controlDict.functions` 引用的场名失配，首版没有跨文件重命名追踪能力。
  - 删除会让 `turbulenceProperties` 声明的模型缺少输入场（运行期才报错）。
  - 与 D-019 保真策略一致：不改的东西就别动。
- 影响范围：
  - 场列表 UI：只读场隐藏“重命名/删除”动作，或置灰并给 tooltip。
  - `CaseValidator` 增加“只读场存在性”提示（删除按钮不可用即无需校验）。
  - 未来支持重命名需新增跨字典引用更新器（`SafeRenamer`），另立 ADR。

---

## D-025：`.ofgproj` 记录源 case 相对路径与每文件 SHA-256

- 日期：2026-10-27
- 状态：接受
- 决策：`.ofgproj` 的 `source` 段记录：源 case 相对路径（相对工程文件所在目录，或用户可重定位的根）、`startDirectory`、导入时间、以及**每个被读取源文件的相对路径 + SHA-256**（不记录绝对路径）。据此实现“源已变化”提示与重导入。
- 原因：
  - 导入后用户可能在外部修改算例；没有哈希就无法判断“GUI 内模型是否已过期”。
  - 哈希是重导入差异对比与增量镜像的基础。
  - 只存相对路径符合 `AGENTS.md` §5 的“源码/工程不散落绝对路径”原则。
- 影响范围：
  - `core::CaseSourceInfo`（`sourceRelativePath`、`startDirectory`、`importedAt`、`files[{relativePath, sha256}]`）。
  - `ProjectJsonWriter`/`ProjectJsonReader` 的 schema（需升 `schemaVersion`）。
  - UI：打开工程时若哈希不一致，提示“源算例已在外部修改”，提供“重新导入/忽略”。
  - 计算哈希需流式读取（大文件不整读），由 `io` 提供 `FileHash` 工具。补充：SHA-256 用系统能力或自实现小工具，不引入第三方密码库（如引入需按 `AGENTS.md` §5 记入本文件）。

---

## D-026：`FaceId` 唯一定义迁入 `core`，`render/FaceId.h` 保留兼容别名

- 日期：2026-10-27
- 状态：接受
- 决策：`FaceId` 六面枚举与 `kAllFaceIds`/`faceIndex`/`faceAxis`/`faceIsMax`/`faceName` 定义迁到 `src/core/FaceId.h`（`namespace ofgui::core`）；`src/render/FaceId.h` 改为兼容头：`#include "core/FaceId.h"` 并在 `namespace ofgui` 中 `using` 这些名字。
- 原因：
  - 六面归属是领域不变量（由 `CaseValidator`/`FaceAssignmentValidator` 判定），属于 core，不属于渲染。
  - 若 core 反向依赖 render，会破坏 ARCHITECTURE §2 的依赖方向（render → core 单向）。
  - 兼容别名让 render/ui 现有 include 与测试无需一次性改动，迁移风险可控。
- 影响范围：
  - 新代码一律 `#include "core/FaceId.h"` 并使用 `ofgui::core::FaceId`。
  - 既有 `src/render/DomainSceneActors.h`、`FaceActorFactory`、`FaceAppearance`、`FacePlaneFactory`、`DomainSceneBuilder.cpp`、`src/ui/ProjectTreeWidget.cpp`、`tests/render/DomainSceneBuilderTest.cpp` 通过 `ofgui::` 别名继续编译；后续任务可逐步移除兼容头。
  - `ofgui_render` 已链接 `ofgui_core`，无新增依赖。

---

## D-027：模型 setter 不做“拒绝式”校验，合法性集中在 `CaseValidator`

- 日期：2026-10-27
- 状态：接受
- 决策：`DomainBox`/`MeshSettings`/`BoundaryPatch`/`FieldDefinition`/`CaseProject` 的 setter 只负责赋值与最小防御（越界索引返回 `false` 或抛 `std::out_of_range`），不拒绝“暂时非法”的业务值；`CaseValidator::validate()` 是唯一不变量判定入口。
- 原因：
  - GUI 编辑必然产生中间态（清空输入、先改名再补关联），setter 抛异常或静默改写会吞掉用户输入，且让错误无法定位到字段。
  - 问题应以 `ValidationIssue`（代码 + 目标字段 + 建议修复）返回，供属性面板内联展示。
  - 导入/导出等需要“硬闸门”的路径可显式调用 validate 并拒绝继续。
- 影响范围：UI 在失焦与导出前调用 validate；缺失 patch 的 BC 等逻辑一致性由 `CaseValidator` 报告（`CASE-FIELD-003`），而不是在 `removePatch` 里猜用户意图。

---

## D-028：校验问题使用稳定 `CASE-*` 代码与四级 `Severity`，仅 `BlockingError` 阻断导出

- 日期：2026-10-27
- 状态：接受
- 决策：问题代码按域分段：`CASE-DOM-00x`（域）、`CASE-MESH-00x`（网格）、`CASE-PATCH-00x`（面归属、命名、2D 空隙边界）、`CASE-FIELD-00x`（场与边界条件）、`CASE-SOLV-00x`（时间控制）。`Severity = Info < Warning < Error < BlockingError`；导出与“标记为可运行”只检查 `hasBlockingErrors() == false`。
- 原因：
  - UI 需要按代码做内联提示、跳转与“一键修复”入口，代码是稳定契约。
  - 导入类问题（`IMPORT-*`）与模型类问题（`CASE-*`）必须可区分，便于统计、测试断言与用户报告。
  - 只有阻断级问题才应阻止导出，警告（如单元数偏大、application 未填）不应挡住用户。
- 影响范围：`ValidationIssue::code()` 是对外契约，`tests/core` 以 `countCode()` 断言；新增规则必须登记代码并补测试。
---

## D-029：OpenFOAM 字典解析器使用 `io` 内部有序 AST + 稳定 `IMPORT-*` 诊断码

- 日期：2026-10-27
- 状态：接受
- 决策：
  - AST 放在 `ofgui::io`（`FoamNode`/`FoamNodeKind` + 值视图 `FoamValue`），不复用 `core::foam`。`FoamDictionary` 是一次解析的结果对象：`parse()` 解析字符串、`parseFile()` 读取文件（BOM/编码/大小守卫）、`sourceText()` 返回逐字节原文、`toText()` 输出规范化文本、`find()`/`findAll()`/`visit()` 提供查询与遍历。
  - 词法严格对齐 v2206 `ISstream`：`grad(p)`、`div(phi,U)` 是单个 word；`/` 不进入 word；`#` 分为 `#{ … #}` verbatim、`#word` 指令与裸 `#`；`$` 支持 `$name`/`${name}`/`$.name`/`${{…}}`，词法边界为 `;`、空白、`()`、`{}`、`[]`、`,`、引号、`#`；`${…}`/`${{…}}` 用花括号深度计数，因此 `${_${FOAM_EXECUTABLE}}` 是**一个** Variable token。字符串只解 `\"` 一种转义，其余反斜杠逐字保留（规格 §5.2）。
  - AST 保持文档顺序与原始值文本，每个节点带 `FoamSourceLocation{file, byteOffset, line, column}`；重复 keyword 不去重、不合并；引号键记 `keywordQuoted_`（回写时保留 `".*"`、`"(free|hole)"`）；`$p;` 这类合并在 AST 中是**无 keyword 的 Variable 子节点**；`key ( … );`、`key [ … ];` 在 `;` 处解包，`find("vertices")` 直接得到 List、`find("dimensions")` 直接得到 DimensionSet；`boundary ( movingWall { … } )` 这类列表内字典保留 keyword。
  - 指令不展开也不执行：`#include`/`#includeIfPresent`/`#sinclude`/`#includeEtc`/`#includeFunc` 报 `IMPORT-DIR-001`，`#inputMode`/`#remove`/`#calc`/`#eval`/`#codeStream`/`#if`/`#ifeq`/`#message` 报 `IMPORT-DIR-002`，未知 `#word` 报 `IMPORT-DIR-003`，全部为 Warning 且条目原文保留；“字典不完整（Partial）”由是否存在 `IMPORT-DIR-*` 派生，不另设标志位。
  - 诊断码稳定且分段：`IMPORT-LEX-001`…`007`（未闭合字符串/块注释/verbatim、非法字符、word 内括号不配对、坏数字、`$` 未闭合）、`IMPORT-PARSE-001`…`006`（缺 `;`、缺闭合括号、缺 `FoamFile` 头、缺 keyword、多余 `;`、多余闭合括号）、`IMPORT-SYN-003`（重复 keyword）、`IMPORT-ENC-001/003`（编码不支持/BOM 已剥离）、`IMPORT-IO-001/002`（读失败/空文件）、`IMPORT-REPORT-001`（诊断截断）。缺 `;` 的位置指向**最后一个值 token** 所在行，而不是 entry 起始行。
  - 解析器不使用异常做控制流；单条 entry 出错不丢后续 entry；诊断按 `byteOffset` 稳定排序，超过 `FoamParseOptions::maxDiagnostics`（默认 200）截断并追加 `IMPORT-REPORT-001`。
  - 路径查询 `find("a/b/c")`、`find("a.b.c")` 等价：先尝试整段精确 keyword（覆盖带 `.`/`/` 的引号键），再剥离前导分隔符，最后从左到右尝试每个切分点；`findAll(dictPath, keyword)` 只返回该字典的**直接**子条目。
  - 每个子功能单独 `.cpp`，共享的纯内联小工具放独立头文件，以满足 D-021 的 400 行上限：`FoamLexer.cpp`（遍历/位置/诊断）+ `FoamLexerTokens.cpp`（各 token 读取器）+ `FoamLexerChars.h`（字符分类）；`FoamParser.cpp`（文档/entry/字典体）+ `FoamParserValues.cpp`（值序列/列表/量纲/指令）+ `FoamParserTokens.h`（token→节点映射）；`FoamValue.cpp`（视图/文本）+ `FoamValueConversions.cpp`（数值/向量/量纲转换）+ `FoamValueText.h`（单行渲染）；`FoamDictionary.cpp`（结果对象/规范化输出）+ `FoamDictionaryLookup.cpp`（路径解析）+ `FoamDictionaryFileReader.cpp`（文件读取与编码守卫）。
- 原因：
  - 解析器是“读别人的文件”的组件，必须对未知/降级语法**保守**：保留原文与位置信息，把“无法理解”变成可展示的诊断，而不是丢弃或猜测。
  - `io` 不反向依赖 `core` 的领域模型（`CaseProject`/`DomainBox`），导入器把 AST 单向投影到 core 模型即可，避免 core 演进时解析器随之破裂。
  - 诊断码与位置是 UI 跳转、报告页与回归测试的稳定契约；缺 `;` 报在值行，用户才能直接看到出错的那一行。
  - 单文件 400 行上限可读性优先；按子功能拆分后每个文件仍只承担一种职责，共享工具以 header-only 内联函数提供，不新增 .cpp。
- 影响范围：
  - `src/io/Foam*`（12 个 `.cpp` + 12 个头文件，另加模块自带的 `Version.h/.cpp`）与 `tests/io/FoamLexerTest.cpp`、`FoamParserTest.cpp`、`FoamParserRecoveryTest.cpp`（共享 `FoamParserTestSupport.h`）、`FoamDictionaryTest.cpp`、`FoamReferenceCorpusTest.cpp` 是解析层契约；`tests/fixtures/foam/` 存 v2206 快照与坏输入样本，运行时不得读 `OpenFOAM-v2206/`（引用语料测试在目录缺失时 `QSKIP`）。
  - 后续 writer（`FoamDictionaryWriter` 与各分字典 writer）应基于同一 AST 输出，不得另建字符串拼接路径；`toText()` 只是规范化兜底格式，逐字典 writer 落地后由 `ExportPreviewDialog` 使用 writer 结果。
  - `IMPORT-*` 与 `CASE-*` 的区分沿用 D-028；引入新的 `IMPORT-*` 码必须同步更新本 ADR 与 `docs/CASE_IMPORT_SPEC.md` §5。
---

## D-030：`.ofgproj` 使用 schemaVersion 1 显式键集、未知字段保留与原子替换保存

- 日期：2026-10-27
- 状态：接受
- 决策：
  - 工程文件固定为 UTF-8（无 BOM）+ LF，经 `QJsonDocument::Indented`（4 空格、Qt 按键名排序）输出；`ProjectJsonRepository::toJsonDocument()` 是唯一文本出口，`save()` 与导出预览共用，禁止在 UI 里另拼 JSON。
  - schemaVersion 1 键集（`ProjectJsonSchema.h` 唯一定义）：`schemaVersion`、`project{name, caseDirectory}`、`domain{min,max,mode,unit}`、`mesh{cells,grading,scale}`、`solver{application,startTime,endTime,deltaT,writeInterval}`、`patches[]{id,name,type,faces,color,visible}`、`fields[]{id,name,class,dimensions,internalField,boundaryField[]}`。枚举按字面量写出：`twoD`/`threeD`、`patch`/`wall`/`empty`/`symmetry`/`symmetryPlane`、`volScalarField`/`volVectorField`、`uniformScalar`/`uniformVector`/`nonUniformRaw`。
  - `internalField.kind = "unset"` 表示 `core::FieldValue::isValid() == false`：模型没有“未设置”枚举值，写入该占位符、读取时保留默认值，从而“尚未配置的场”也能无损往返。
  - 必填与可选：`schemaVersion`、`domain.min/max`、`mesh.cells`、`solver` 五项、每个 patch 的 `id/name/type`、每个 field 的 `id/name/class/dimensions/internalField` 必填（缺 → `PROJ-MODEL-001`）；`project`、`domain.mode/unit`、`mesh.grading/scale` 以及 `color`/`visible`/`parameters` 可选，缺省取模型默认值。
  - 未知字段保留：读取时把本版本不认识的键收进 `ProjectJsonExtras`（根与四个子字典按键名、数组元素按 `id`、边界条件按 `"<fieldId>/<patchId>"` 键控），保存时先写 extras 再覆盖模型键，并记一条 `PROJ-MODEL-007` Info。没有可读 `id` 的数组元素无法匹配，其未知键显式丢弃（不猜测 id）；`internalField` 与 `parameters` 内部属于模型，不保留未知键。
  - 版本策略：只接受 `schemaVersion == core::kCurrentSchemaVersion`。旧版本不做隐式迁移，新版本报 `PROJ-SCHEMA-002` 并明确“文件未被修改”。**加载过程从不写文件**，旧版本 GUI 因此不可能毁坏新版本数据；迁移必须作为显式动作另立 ADR 实现。
  - 保存走 `AtomicFileWriter`：同目录临时文件 `<name>.tmp-<16 hex>` → 写入并 flush → 继承目标权限 → `rename` 覆盖；rename 失败时删除目标后重试一次。任何失败都清理临时文件，目标要么是旧内容、要么不存在，不产生半写文件。路径为空、目标是目录、目录不可创建都归为 `PROJ-IO-003`。
  - `caseDirectory` 为绝对路径或盘符开头时保存只给 `PROJ-PATH-001` Warning 并保留原值：该字段当前是用户显式输入，改成相对路径是导入器的职责。D-025 规定的 `source` 段（相对 case 路径 + 每文件 SHA-256）尚无 `core` 模型支撑，延后到 schemaVersion 2，v1 不写该段。
  - 错误码分段：`PROJ-IO-001..004`（打开/读取/目标不可写/写入失败）、`PROJ-ENC-001`（剥离 UTF-8 BOM）、`PROJ-JSON-001/002`（JSON 语法错误带字节偏移 / 根不是对象）、`PROJ-SCHEMA-001..003`（缺版本 / 不支持版本 / 版本类型错）、`PROJ-MODEL-001..007`（缺字段 / 类型错 / 值非法 / id 重复 / 未知枚举字面量 / 引用不存在的 patch / 保留未知字段）、`PROJ-PATH-001`、`PROJ-REPORT-001`（诊断超 200 条截断）。未知枚举按 `threeD`/`patch`/`volScalarField` 回退并记 `PROJ-MODEL-005`；引用不存在的 patch 保留边界条件并记 `PROJ-MODEL-006`，交给校验器修复。
  - 加载失败时 `ProjectLoadResult::ok == false`，`project` 与 `extras` 复位为空：半填充模型绝不交给 UI，也不回写文件。
  - 文件组织（D-021）：`ProjectJsonSchema.h`、`ProjectJsonDiagnostic.h/.cpp`、`ProjectJsonExtras.h`、`ProjectJsonValueReader.h/.cpp`、`ProjectJsonMapping.h`、`ProjectJsonWrite.cpp`、`ProjectJsonRead.cpp`、`ProjectJsonReadPatches.cpp`、`ProjectJsonReadFields.cpp`、`ProjectJsonReadExtras.cpp`、`ProjectJsonReadSections.h`（内部跨文件声明）、`ProjectJsonRepository.h/.cpp`、`AtomicFileWriter.h/.cpp`；每个 `.cpp` 只承担一个子功能，且均在 400 行以内。
- 原因：
  - 工程 JSON 是 GUI 状态的唯一持久化来源（AGENTS.md §5），必须能承受“换机器”“旧版本打开新文件”两类场景：保留未知字段比整体丢弃安全，比猜测语义诚实。
  - 原子替换是 AGENTS.md 的硬要求；临时文件必须与目标同目录，跨卷 rename 不是原子操作。
  - 显式键集加单一 schema 头文件避免读写两处拼字符串漂移；枚举回退保证一份文件不会因为出现一个新词就整体打不开。
- 影响范围：
  - `src/io/ProjectJson*` 与 `src/io/AtomicFileWriter.*` 是工程持久化契约；新增字段必须同时更新 `ProjectJsonSchema.h`、读写两侧与 `tests/io/ProjectJsonRepositoryTest.cpp` 的逐字段比较，并在本 ADR 记录。
  - 后续导入器应通过 `ProjectJsonExtras` 承载 `source` 段，而不是修改 v1 读取逻辑；升级 schema 时必须同时补 `PROJ-SCHEMA-*` 回归测试。
  - `ofgui_io` 仍只依赖 `ofgui_core` + `Qt6::Core`（`QJson*` 属 Qt Core），未引入新的第三方依赖。

---

## D-031：`CaseProject.source`（D-025 来源模型）先行补齐，持久化在 t17 落地

- 日期：2026-10-27
- 状态：接受
- 决策：
  - `src/core/CaseSourceInfo.h/.cpp` 定义 D-025 要求的来源模型：
    - `SourceFileHash{relativePath, sha256[32]}`：`sha256` 存 32 字节原始值，十六进制只是边界表示；`sha256Hex()` / `setSha256Hex()` 与自由函数 `sha256ToHex()` / `sha256FromHex()` 负责转换。解析严格要求 64 位十六进制（大小写均可），长度或字符非法时返回 `false` 且不改写目标 digest。
    - `CaseSourceInfo{sourceRelativePath, startDirectory, importedAt, files}`：`sourceRelativePath` 相对工程文件所在目录、以 `/` 分隔，绝不写绝对路径；`startDirectory` 记录导入使用的时间目录（如 `0` 或 `0.orig`）；`importedAt` 为 ISO-8601 UTC 文本；`files` 通过 `setFileHash/removeFile/findFile/fileCount/isEmpty` 维护，同一路径重复设置时覆盖而非追加。
    - `CaseProject` 增加 `source()` / `setSource()` / `hasSource()`；空值表示“新建工程、无来源”，不影响既有构造、校验与序列化调用方。
  - `CaseValidator` 不读取 `source`：来源是事实记录，不是可修复的模型不变量；缺少来源不应阻断导出（导出目标由 `caseDirectory` 决定）。
  - 持久化在 t17 落地：优先作为 schemaVersion 2 的正式键 `source{...}` 写入，并保持 v1 读取兼容；在 t17 之前，`ProjectJsonExtras` 仍按 D-030 承载 `source` 段。本 ADR 取代 D-030 中“`source` 段尚无 `core` 模型支撑”的表述。
- 原因：
  - D-025 要求导入后能判断“源算例是否已被用户或求解器改动”（逐文件 SHA-256）。没有 core 模型时，t6 导入器只能把来源塞进 UI 状态，违背“`CaseProject` 是唯一业务数据源”。
  - SHA-256 以 32 字节存储、仅在契约边界转 hex，避免 JSON 文本成为唯一真相源，也避免大小写或分隔符差异被误判为“文件已变化”。
- 影响范围：
  - `src/io/ProjectJson*`（t17）按本 ADR 的字段名落盘，并补逐字段往返测试；升级 `kCurrentSchemaVersion` 时同步 `ProjectJsonSchema.h` 与 `PROJ-SCHEMA-*` 回归测试。
  - `tests/core/CaseSourceInfoTest.cpp` 是 hex 往返、非法输入与文件表语义的回归基线；导入器（t6）只通过 `CaseSourceInfo` 的公开方法填充来源。

---

## D-032：算例导入拆成“文件系统扫描 + 字典投影”，来源哈希写入 `CaseSourceInfo`

- 状态：已接受（IMP-03 扫描器 / IMP-04 导入器落地）
- 背景：
  - 导入必须只读源算例（用户硬性要求与 `AGENTS.md` §5），并且要在不依赖 OpenFOAM 安装的前提下识别 `0/`、`constant/`、`system/`。
  - 教程算例的结构差异很大（`0/` vs `0.orig/`、缺 `turbulenceProperties`、`blocks` 多块、`#includeEtc`），逐文件规则需要可测、可复用。
- 决策：
  1. **两段式**：`CaseScanner` 只做文件系统探测（无解析、不跟随符号链接、`0/` 优先于 `0.orig/`、polyMesh 只探测五件套存在性），产出 `CaseDirectoryScan`；`CaseImporter` 再按白名单解析字典并投影。扫描与解析分离，使“空目录/缺文件”类错误不需要启动解析器。
  2. **投影器不碰 IO**：`BlockMeshProjector`、`BlockMeshPatchProjector`、`FieldProjector`、`BoundaryConditionFactory`、`ControlDictProjector`、`TransportPropertiesProjector`、`ReadOnlyDictionaryProjector` 只接收 `FoamDictionary` + `CaseProjectionContext`，返回结构化结果并把 `ValidationIssue` 追加到上下文，便于单测与后续导出复用。
  3. **支持级别是 io 局部类型**：`SupportLevel{Editable,ReadOnly,Recognized,Unsupported}` 与 `ImportReport`/`ImportedFile` 留在 `src/io`，不污染 `core`；`core::CaseProject` 只保存真正可编辑的业务数据。
  4. **解析器形态适配**：`blocks ( hex (…) (20 20 1) simpleGrading (1 1 1) )` 的列表子项是逐 token 的，投影器按“块类型单词分组”重建块描述；单 token 值（`type wall;`、`scale 0.1;`）在解析层解开 `Sequence` 包装，保证叶子语义稳定。
  5. **无法解析的 `system/controlDict` 视为 `BlockingError`**（`IMPORT-CASE-006`）：没有 `application`/时间参数就无法导出可运行算例，宁可在导入阶段阻断。
  6. **来源记录**：`CaseSourceRecorder` 递归遍历算例根目录，对每个普通文件算 SHA-256（超过 32 MiB 或不可读的文件跳过并计数），结果写入 `core::CaseSourceInfo`；`importedAt` 用 ISO-8601 UTC 文本。目录路径一律 case 相对 POSIX 形式。
  7. **用户可见文案走码表**：`ImportIssueFactory` 用 `IMPORT-*` 码查中文 `message`/`suggestedFix`，解析器的英文诊断只作为 `detail` 不直接展示；码表与 `docs/CASE_IMPORT_SPEC.md` 附录 C 必须同源维护。
  8. **规模与顺序**：issue 超过 200 条截断（`IMPORT-REPORT-001`），文件超过 5000 个停止列举（`IMPORT-REPORT-002`）；报告按 severity → 文件 → 行 → 码稳定排序，`BlockingError` 决定 `ok=false`。
- 影响范围：
  - 新增文件均 ≤400 行（D-021）；`BlockMeshValueReader`、`CaseScannerHelpers`、`BlockMeshPatchProjector` 是为满足该约束拆出的子功能单元。
  - 后续导出器（t8/t9）必须复用 `ImportReport` 的支持级别与 issue 码，不得另建一套；`ProjectJsonRepository`（t17）负责把 `CaseSourceInfo` 落盘成 schemaVersion 2 的 `source` 段。
- 替代方案与放弃原因：
  - 用 `core::ValidationIssue` 之外再建一套 issue 类型：放弃，统一用 core 类型以复用排序与四级语义。
  - 扫描时顺便解析：放弃，扫描要能对损坏字典给出“文件存在但不可解析”的分级报告，且在部分损坏时继续工作。
---

## D-033：导出侧的 writer 分文件 + “值列 = 缩进 + 16”的定宽布局，cavity 六个字典逐字节复现

- 状态：已接受（IMP-05 写出器落地，t7）
- 背景：
  - `docs/CASE_IMPORT_SPEC.md` §9.4 要求 `0/p`、`0/U` 在未编辑往返后**逐字节相同**，同时 §9.1/§9.2 要求 `blockMeshDict`、`controlDict`、`fvSchemes`、`fvSolution`、`transportProperties` 由模型重新生成（禁止回拷源文件）。
  - 早期草案的“`keyword` + 4 空格 + 值，不做列对齐”与 v2206 教程文件不符，逐字等价必然失败。
- 实测基准（逐字节取自 `tests/fixtures/foam/cavity/`）：banner/分隔行/尾行均 79 字符定宽；`FoamFile` 块值列固定第 16 列；普通条目值列 = 缩进 + 16（顶层 16、一级嵌套 20、BC 体 24）；`keyword` 抵到列位时退化为单个空格。
- 决策：
  1. **一个 writer 一个文件**：`BlockMeshDictWriter`、`ControlDictWriter`、`FvSchemesWriter`、`FvSolutionWriter`、`TransportPropertiesWriter`、`FieldFileWriter`，共享 `FoamLayout`（缩进/对齐/79 字符 banner）与 `FoamNumberFormat`（`std::to_chars` 最短往返）。
  2. **值列 = 缩进 + 16**，`FoamFile` 块例外地用绝对第 16 列；`blockMeshDict` 的 `scale` 用第 8 列、patch 内 `type`/`faces` 用单空格——这是教程文件的实际形态，不追求内部统一。
  3. **数字**：`std::to_chars` 最短往返（`1e-06`、`0.5`、`20`）；**网格顶点额外做 15 位有效数字整形**（`formatFoamPoint`），否则 `vertices × scale ÷ scale` 的二进制噪声会把 `0.1` 写成 `0.10000000000000002`，破坏逐字等价。
  4. **hex 面顶点序**采用教程写法（xMin `0 4 7 3`、xMax `2 6 5 1`、yMin `1 5 4 0`、yMax `3 7 6 2`、zMin `0 3 2 1`、zMax `4 5 6 7`），它们与 `core::FaceId` 一一对应，循环等价但与 `blockMesh` 自身输出同形。
  5. **`fvSolution` 的 `$p;` 按语义展开**：先写被引用字典的条目（跳过合并方自己重定义的键），再写合并方自己的条目；解析顺序为“先同级、后父级”（与 OpenFOAM 变量解析一致），因此 `pFinal` 得到 `solver`/`preconditioner`/`tolerance` + 自己的 `relTol`。
  6. **`scheme`/`solution` 字典走 AST 重排**（`FoamDictionaryEmitter`），保持源条目顺序、字典内条目相邻不插空行；顶层条目之间留一个空行（与教程一致）。
  7. **导出先校验后写盘**：`CaseExporter` 先跑 `core::CaseValidator`，存在 `Error`/`BlockingError` 时拒绝并追加 `IMPORT-EXP-006`，一个字节都不写。
  8. **原子写**：全部文件先写入 `<target>.tmp-<seq>-<timestamp>/`，成功后目录级 rename；已存在的非空目标必须显式确认（`allowExistingTarget`），确认后改名为 `<target>.bak-<ts>`；已存在但**空**的目标直接替换且不留备份。
  9. **目标保护**：目标位于源算例内 → `IMPORT-EXP-001`（Blocking）；目标非空未确认 → `IMPORT-EXP-002`；写失败 → `IMPORT-EXP-003`；目录不可写 → `IMPORT-EXP-004`。
- 影响范围：
  - 新增 `src/io/writers/` 目录（18 个文件），全部登记在 `src/io/CMakeLists.txt`；`ofgui_io` 仍只链接 `ofgui_core` + `Qt6::Core`。
  - 黄金测试 `tests/io/CaseWriterGoldenTest.cpp`：以 `tests/fixtures/foam/case` 导入 → 导出 → 与 7 个快照逐字节比较，其中 6 个与 `tests/fixtures/foam/cavity/` 相同，`fvSolution` 与 `tests/fixtures/golden/cavity/system/fvSolution` 相同。
  - `docs/CASE_IMPORT_SPEC.md` §9.4 的布局与等价判定已按实测更新；附录 C 补 `IMPORT-EXP-001/002/003/004/006`。
- 替代方案与放弃原因：
  - 直接复制源文件以通过逐字等价：放弃，规格 §9.4 明文禁止，且编辑后的模型无法反映。
  - 用 `printf("%.6g")` 统一浮点格式：放弃，会改写用户原始精度（`1e-06` → `1e-06` 尚可，`0.005` → `0.005` 但是 `1e-10` 会变成 `1e-10`/`0`），且与 `§9.4` 的最短往返要求冲突。
  - 把 `ScaleSolutionWriter` 合成一个类：放弃，用户硬性要求“每个 writer 一个文件”，且 §10.8 评审清单要求文件名 = 主类名。

---

## D-034：`.ofgproj` 升到 schemaVersion 2，`source` 段常写、v1 内存迁移、绝对路径与哈希严格校验

- 状态：已接受（t17 落地）
- 背景：
  - D-030 记录 schemaVersion 1 时明确写着“D-025 的 `source` 段延后到 schemaVersion 2”；D-031 补齐了 `core::CaseSourceInfo`，并约定持久化在 t17 落地。本 ADR 就是这次落地，同时取代 D-030 中“延后到 v2”与 D-031 中“在 t17 之前 `source` 由 `ProjectJsonExtras` 承载”的表述。
  - `core::kCurrentSchemaVersion` 仍为 1（模型修订号，`CaseProject::schemaVersion()` 返回它）；工程**文件**的版本由 `io` 自己决定，不再读 core 常量。
- 决策：
  1. **版本常量归 `io`**：`projectjson::kCurrentProjectSchemaVersion = 2`、`kOldestSupportedProjectSchemaVersion = 1`，写在 `src/io/ProjectJsonSchema.h`。写出永远写 2，读取接受 1..2；`N < 1` 或 `N > 2` 一律 `PROJ-SCHEMA-002`，且“文件未被修改”。
  2. **v1 → v2 只在内存里迁移**：读到版本 1 时照常解析模型，`source` 视为空（v1 没有该段，即使手写出现了也忽略），在 `ProjectLoadResult` 里置 `migrated = true` 并追加 Info `PROJ-SCHEMA-004`（消息同时给出旧版本号与当前版本号）。**加载从不回写文件**；用户下一次保存时才写成 2。
  3. **`source` 段永远写出**（含新建工程）：`{caseRelativePath, startDirectory, importedAt, files[]}` 四个键总在，空工程写成空串 + `[]`。理由：读者不需要区分“缺段”和“空段”，保存后的字节布局也不依赖工程是否有来源。
  4. **`sha256` 文本形态严格**：必须 64 位**小写**十六进制（`PROJ-MODEL-008`），解析后存入 `core::Sha256Digest` 的 32 字节原始值；大写被拒是有意的（`core::sha256ToHex` 只生产小写，出现大写只可能是手改）。同一路径重复出现 → Warning `PROJ-MODEL-009`（读时保留第一条、写时去重）。
  5. **路径只允许相对形式**：`source.caseRelativePath` 与 `files[].relativePath` 绝对即 `PROJ-PATH-002`——**读为 Error**（D-025 禁止绝对路径），**写为 Warning 且原值照写**（不静默丢数据，用户能看到问题）。判定用 `std::filesystem::path::is_absolute()/has_root_directory()/has_root_name()`，因此 `D:/x`、`/x`、`\\server\x` 都命中。
  6. **空 `caseRelativePath` = 无来源**：此时整段按空处理（`CaseSourceInfo{}`），`startDirectory`/`importedAt`/`files` 不再生效；若 `files` 非空则追加 Warning `PROJ-MODEL-010`，避免“看起来有来源却被当成新建工程”而无人知晓。
  7. **未知键继续保留**：`source` 段与每个 `files[]` 元素的未知键都进 `ProjectJsonExtras`（`extras.source`、`extras.sourceFiles[relativePath]`），保存时原样写回，并保留 D-030 的 `PROJ-MODEL-007` 提示。
  8. **失败即不改文件**：解析/校验任一处 Error 时 `ProjectLoadResult::ok = false`，`project`/`extras` 复位为空、`migrated = false`，磁盘文件保持原样（保存仍走 `AtomicFileWriter` 的临时文件 + rename）。
- 影响范围：
  - 新增 `src/io/ProjectJsonSource.h/.cpp`（段 ↔ `core::CaseSourceInfo` 双向映射 + 两个校验谓词），登记在 `src/io/CMakeLists.txt`；`ProjectJsonSchema/Diagnostic/Extras/Mapping/Read/Write/ReadExtras/Repository` 相应修改，所有文件仍 ≤400 行（D-021）。
  - 新测试：`tests/io/ProjectJsonSourceTest.cpp`（往返、常写、二次保存字节一致、未知键保留、绝对路径与重复路径告警、hex 小写）、`tests/io/ProjectJsonMigrationTest.cpp`（v1 迁移、v1 中的 `source` 被忽略、保存写 2 且再读不再迁移、空 `caseRelativePath`、短哈希/大写哈希/绝对路径/重复/缺 `sha256` 的错误用例，且每个失败用例都断言文件字节未变）。
  - 既有测试同步：`ProjectJsonRepositoryTest` 的稳定文本断言改为 `"schemaVersion": 2`；“不支持版本”用例改用 3；其余 v1 fixture 继续留着，正好覆盖迁移路径。
- 替代方案与放弃原因：
  - 让 reader 直接写回迁移结果（“打开即升级文件”）：放弃，D-030/§9 明确“加载不写文件”，且只读介质或权限不足时会让“打开工程”变成写操作。
  - 只写 v2、拒绝 v1：放弃，现存的工程文件会全部打不开，与 D-031 “保持 v1 读取兼容”冲突。
  - 让 `core::kCurrentSchemaVersion` 跟着升到 2：放弃，那是模型修订号，升它会让 `CaseProject::schemaVersion()` 与文件版本语义混在一起，且 t17 不允许改 `src/core`。
  - 绝对路径在写出时直接丢弃/改写为空串：放弃，属于静默丢数据；告警 + 原样保留更容易被用户发现并修正。---

## D-035：导出补全为“文件系统镜像 + 显式动作清单”，`Written`/`Copied`/`Mirrored` 三分与 skip 单列

- 背景：D-033 之后导出只写 7 个模型驱动的字典，导出的算例没有 `constant/polyMesh/`、没有其它场，无法直接跑求解器；而 spec §9.2 的动作表（`rewrite`/`mirror`/`skip`）尚未落地。t18 补全这一段，并保持“不解析来源、只搬运字节”的原则。
- 决策：
  1. **来源只走文件系统，不解析**：`SourceCaseWalker` 用 `recursive_directory_iterator` 列出源算例的普通文件，`postProcessing/`、`processor*/` 在遍历阶段就剪枝；符号链接只记录不跟随（§4.1）；超过 `kMaxFiles = 20000` 置 `truncated`。起始目录 `0/` 优先、`0.orig/` 回退，与导入一致（D-018）。
  2. **动作三分 + skip 单列**：`ExportAction::Written`（writer 生成）、`Copied`（保真复制：起始目录里模型没接管的场、`constant/turbulenceProperties`、`system/decomposeParDict`）、`Mirrored`（其余全部源文件，含 `constant/polyMesh/**`）。被排除的源文件不进 `plan.files`，而是进 `plan.skipped` 并带 `reason`（`runtime-artifact`/`other-time-directory`/`symbolic-link`/`unreadable`）。理由：`files[]` 只回答“导出了什么”，把 skip 混进去会让清单语义变成“见过的文件”，用户反而看不出跳过了什么。
  3. **接管集合按路径判定**：`CaseMirrorPlanner` 以“计划里已存在的相对路径 + 起始目录内被模型接管的场文件名”为保护区，因此 writer 生成的 `0/p` 永远不会被源文件覆盖；源为 `0.orig/` 时映射到导出起始目录（`0.orig/T` → `0/T`），不会把 `0.orig/` 整目录搬过去。
  4. **镜像 = 逐字节流复制**：64 KiB 缓冲、二进制模式，`.gz` 保持 `.gz`（不解压、不重算哈希）；复制前后都不改写权限位之外的元数据。
  5. **写入顺序与失败原子性**：`AtomicCaseDirectory` 在 `<target>.tmp-<seq>-<stamp>/` 里先写字典、再执行镜像、最后写 `.ofgport-export.json`，然后目录级 rename。新增两个失败态 `MirrorFailed`（源缺失/不可读）与 `ManifestFailed`（清单写入失败），分别映射 `IMPORT-EXP-008`/`IMPORT-EXP-009`；任何失败都会 `remove_all` 临时目录，旧目标保持原样。镜像不完整（符号链接、触顶）只记 `IMPORT-EXP-010` Warning，导出继续，但清单里能看出缺了什么。
  6. **清单格式（schemaVersion 1）**：`{schemaVersion, generatedAt, counts{written,copied,mirrored,skipped}, files[{relativePath,action,bytes}], skipped[{relativePath,reason}]}`；`files[]`/`skipped[]` 均按路径排序，`toJson(Indented)` + LF + 单一结尾换行，不含绝对路径、不列自身。`generatedAt` 由调用方注入（`currentUtcTimestamp()`），测试因此可断言稳定文本。
  7. **空目标无备份**：目标存在但为空时直接替换、不留 `<target>.bak-*`；只有非空目标（已被用户确认）才备份（延续 D-033，测试 `refusesSecondExportWithoutConfirmation` 覆盖二次导出必须先确认）。
- 影响范围：
  - 新增 `src/io/writers/`：`ExportAction`、`ExportSkipRules`、`SourceCaseWalker`、`CaseMirrorPlanner`、`CaseMirror`、`ExportManifestSchema`、`ExportManifest`（各 `.h/.cpp`，`ExportManifestSchema.h` 仅头文件），共 8 个 `.cpp` + 7 个头文件，全部登记 `src/io/CMakeLists.txt`；`CaseExportPlan` 扩为 `files`+`skipped`+`manifestText`，`CaseExportOptions` 增 `mirrorSourceFiles`/`keepRunScripts`/`mirrorOtherTimeDirectories`，`AtomicCaseDirectory`/`CaseExporter` 相应扩展；`ImportCodes.h` 补 `EXP-008/009/010`，`ImportIssueFactory` 表扩到 60 行。所有文件 ≤400 行（D-021）。
  - 新测试：`tests/io/CaseMirrorTest.cpp`（walker 只列普通文件、skip 规则与策略开关、planner 的 Copied/Mirrored 分类、`0.orig` 映射、逐字节复制含 NUL、缺源报错）、`tests/io/ExportManifestTest.cpp`（JSON 合法、排序、动作与字节数、skip 理由、counts、LF/结尾换行、不列自身）；`tests/io/CaseExporterTest.cpp` 增 4 槽（polyMesh 五个文件逐字节一致、清单与实际计划一致、skip 名单、二次导出拒绝）。
  - 文档：spec §9.2 实现进度、§9.5 第 3/4/6 条、附录 C 的 `EXP-008` 更正与 `EXP-009`/`EXP-010` 新增、附录 E 1.4。
- 替代方案与放弃原因：
  - 解析源字典后重新生成“其它场/constant 文件”：放弃——非接管场与 `polyMesh` 的保真要求就是逐字节一致（§9.4），重新生成必然引入格式漂移。
  - 用硬链接代替复制：放弃——Windows 上需要额外权限，且目标与源共享 inode 会让“导出后修改导出算例”污染源算例。
  - 把 skip 也写进 `files[]`（`action: "skip"`）：放弃——清单会被运行时产物淹没，且 `bytes` 对跳过的文件没有意义。
  - 由 UI 组装清单文本：放弃——清单是导出产物的一部分，必须与写入同源，否则预览与落盘会不一致。

## D-036：场编辑器只改“能无损表达”的部分，预览复用导出 writer

- 背景：t10 要在 GUI 编辑 `0/` 下的场（dimensions、internalField、逐 patch BC），但导入的字典里存在 `nonuniform` 列表、`$internalField` 引用、`inletOutlet` 等首版 UI 表达不了的构造。如果一律改写，会把原文压成模型能表达的最简形式，造成静默格式漂移。
- 决策：
  1. **可编辑范围显式白名单**：`FieldBoundaryTable::scalarTypeNames()` / `vectorTypeNames()` 定义首版可编辑 BC 类型（`zeroGradient`、`fixedValue`、`noSlip`、`empty`、`symmetry`、`inletOutlet`、`fixedFluxPressure`、`calculated` 等）；白名单外的导入类型只读展示原文，`BoundaryCondition::parameters()` 原样保留。理由：D-024 已确立“未知类型原文保留”，UI 不应成为绕过它的后门。
  2. **`internalField` 只编辑 uniform 标量/向量**：`UniformValueEditor` 解析 `uniform <scalar>` 或 `uniform (x y z)`；`nonuniform`、`$...`、函数对象值走只读 `QLineEdit`（`showRaw()`）。标量/向量页面按 `FieldClass` 切换，不猜类型。
  3. **dimensions 只读**：7 个指数完整显示但禁用编辑。量纲变化会改变物理含义且无法从 UI 判断用户意图，留待专门任务（与 spec §7 的字段清单一致）。
  4. **预览即导出**：`FieldEditor::updatePreview()` 与 `ExportPreviewPanel` 都调用 `io::FieldFileWriter::write()`，禁止 UI 自行拼字典文本（AGENTS.md §5）。预览与保存使用同一 writer，避免“预览好看、导出不同”。
  5. **新增/删除场限定模板**：只允许 `p`（`volScalarField`，`[0 2 -2 0 0 0 0]`）与 `U`（`volVectorField`，`[0 1 -1 0 0 0 0]`），删除同样只对这两个名字开放；其它场一律保留，避免误删求解器必需场或用户自定义场。新建场按 patch 建默认 BC：`p → zeroGradient`，`U → noSlip`。
  6. **一文件一职责**：`FieldEditor`（页面/写模型）、`FieldBoundaryTable`（逐 patch 表格）、`UniformValueEditor`（uniform 值控件）、`UniformValueText`（纯文本解析/格式化）、`ExportPreviewPanel`（0/ 预览页）各自独立 `.h/.cpp`，全部登记根 `CMakeLists.txt`。
- 影响范围：新增 `src/ui/{FieldEditor,FieldBoundaryTable,UniformValueEditor,UniformValueText,ExportPreviewPanel}.{h,cpp}`；`InspectorPanel` 增加 Field/FieldBoundary 路由与第三页；`tests/ui/FieldEditorTest.cpp`（7 槽，含 cavity 全字段回填、非法值拒绝、未知类型只读、预览与 writer 一致）；`CMakeLists.txt` 新增 `ofgui_ui_field_tests`。
- 替代方案与放弃原因：
  - 允许 `nonuniform` 列表在表格里逐项编辑：放弃——首版没有列表编辑器，强行解析会把嵌套列表/函数对象压平，违反“降级不得静默丢数据”。
  - 用正则直接改原文文本：放弃——绕开 `CaseProject` 与 writer 的单一事实源，diff 不可控，也破坏稳定 ID 映射。
  - dimensions 可编辑：放弃——见第 3 条；`CaseValidator` 目前也不校验量纲语义，开放编辑会制造无法验证的状态。

## D-037：求解设置只暴露模型能承载的键；transport 走 controller 的单一对象

- 背景：t11 要覆盖 `controlDict`、`fvSchemes`、`fvSolution`、`constant/transportProperties` 的常见设置。但 `core::SolverSettings` 只有 `application/startTime/endTime/deltaT/writeInterval`，`constant` 侧只有 `io::TransportModelSettings`（`transportModel` + `nu`）被 exporter 消费；`startFrom`、`stopAt`、`writeControl`、`fvSchemes`、`fvSolution`、`simulationType` 都没有模型支撑。若在 UI 提供这些输入框，用户改了界面但导出不变，属于“把未完成功能伪装成可用”。
- 决策：
  1. **可编辑集合 = 模型集合**：只对 `application`（可编辑下拉）、`startTime`、`endTime`、`deltaT`、`writeInterval` 提供控件；`transportModel`/`nu` 写 `CaseDocumentController::transportSettings()`（exporter 的 `CaseExportOptions.transport` 来源），不另建 UI 侧副本。
  2. **无模型键降级为只读说明**：`startFrom/stopAt/writeControl` 从 `readOnlySettings()` 按 `system/controlDict.<key>` 精确取值，显示“导入原值 + 导出固定写 `startTime`/`endTime`/`timeStep`”；`fvSchemes`/`fvSolution`/`turbulenceProperties` 走“只读原文”页签，逐字来自导入缓存，不重新解析。后续若要开放编辑，先补领域模型与 writer，再改 UI。
  3. **nu 的安全边界**：只有纯数值可编辑；带量纲（`nu [0 2 -1 0 0 0 0] 1e-05`）或 `$...` 表达式时控件置灰、显示原文、保存逐字回写（`isKinematicViscosityEditable()`），避免 UI 把有量纲值压成裸数字改变物理含义。`transportModel` 仅在空或 `Newtonian` 时可改，其他模型只读（当前 writer 只写这两个取值）。
  4. **非法输入不进模型**：`endTime > startTime`、`deltaT > 0`、`writeInterval >= 1`、`application` 非空在控件信号里先行检查；失败时红色错误行 + 控件回滚到模型值，不调用 `notifyProjectEdited()`。与 t9 的“非法值不进入模型、也不静默夹取”一致。
  5. **预览复用 writer**：`导出预览` 页签直接调用 `io::ControlDictWriter::write(project)` 与 `io::TransportPropertiesWriter::write(transportSettings)`；测试对两页签逐字比较 writer 输出，预览与导出不会漂移。
  6. **一文件一职责**：`SolverEditor` 一个 `.h/.cpp`（页签、控件、写模型、校验），transport 模型投影仍在 `io`，controller 只增加 `transportSettings()`/`notifyTransportEdited()` 与源字典缓存访问器。
- 影响范围：新增 `src/ui/SolverEditor.{h,cpp}`；`InspectorPanel` 增加 Solver 路由与第四页；`CaseDocumentController` 增加 transport/只读设置与源字典访问器（t11 编辑）；`tests/ui/SolverEditorTest.cpp`（8 槽：cavity 回填、控件写模型与预览同步、非法输入拒绝、nu 与 transport 预览、writer 逐字一致、只读原文、新建默认、Inspector 路由）；`CMakeLists.txt` 新增 `ofgui_ui_solver_tests`。
- 已知边界：UI 尚无“实际导出”入口（t13 负责）；届时构造 `io::CaseExportOptions` 必须取 `document.transportSettings()`，否则 nu 的编辑只影响预览。`startFrom` 等键的编辑能力留待补模型任务。

## D-038：导出只走 `ExportController`，预览不写盘，阻断在校验页先行可见

- 背景：t12 要打通“导入 → 编辑 → 校验 → 导出”纵向流程。风险有三类：UI 直接拼字典文本绕过 writer；预览与导出行为漂移；阻断问题直到写盘后才暴露，甚至留下半成品目录。
- 决策：
  1. **单一导出服务**：`ExportController` 是 UI 侧唯一导出入口，内部只调用 `io::ExportPlanBuilder`/`io::CaseExporter`，UI 不生成 OpenFOAM 文本。`ExportPreviewDialog` 只读 `preview()` 的计划，写盘发生在对话框 `accept()` 之后的 `exportTo()`，因此整个写盘路径可无对话框测试。
  2. **计划也校验**：`ExportDictionaryCheck` 对计划中每个生成文件（`system/blockMeshDict`、`controlDict`、`fvSchemes`、`fvSolution`、`0/<field>`）检查存在且非空，并用 `io::FoamDictionary::parse` 重新解析，损坏文本报 `EXPORT-DICT-001`；缺文件报 `EXPORT-DICT-002`。两类都是 `BlockingError`。
  3. **模型校验先进预览**：`preview()` 合并 `document.validation()` 的问题，存在 `Error`/`BlockingError` 时 `ok=false`、对话框禁用导出并显示原因；主窗口导出动作同样按校验结果禁用。阻断必须在按“导出”之前可见（D-028 的 `BlockingError` 语义延续到 UI）。
  4. **transport 接线固定点**：`ExportController::makeOptions()` 把 `document.transportSettings()` 填入 `io::CaseExportOptions.transport`，`fvSchemesText`/`fvSolutionText` 取导入缓存原文；t11 的 D-037 边界在此闭合并由测试断言（nu = 0.02 出现在导出文件）。
  5. **默认算例即 cavity 模板**：首次启动/新建使用 `DefaultCaseTemplate`（2D 20×20×1、icoFoam、p/U 与三 patch BC、nu 0.01），保证“新建 → 导出”开箱可用；模板只构造模型，不写文件。
  6. **失败不破坏已有算例**：目标已存在时必须勾选覆盖并在确认框二次确认；覆盖由 `AtomicCaseDirectory` 生成 `.bak-<stamp>` 备份；失败路径删除临时目录，测试断言目标不存在且无 `.tmp-*` 残留。
  7. **校验问题可定位**：`ValidationNavigator` 把 `targetField` 前缀（`domain`/`mesh`/`solver`/`patches`/`fields`）与对象 ID 映射为 `ProjectNodeRef`，`ValidationPanel` 双击后经树选中联动到对应编辑器页，不使用显示文本匹配。
- 影响范围：新增 `src/ui/{ExportController,ExportPreviewDialog,ExportDictionaryCheck,ValidationPanel,ValidationNavigator,DefaultCaseTemplate}.{h,cpp}`；`CaseDocumentController` 的 `newProject()` 改用默认模板；`MainWindow` 增加导出动作、校验 dock 与导出状态；`src/app/main.cpp` 增加 `--export-preview`/`--export-blocked` 截图钩子；`tests/ui/ExportWorkflowTest.cpp`（10 槽）；`CMakeLists.txt` 新增 `ofgui_ui_workflow_tests`。
- 已知边界：`fvSchemes`/`fvSolution` 仍以导入原文透传，不提供结构化编辑；`.ofgproj` 仍不持久化 transport（D-037 的模型缺口），transport 编辑随会话存在，导出立即生效；VTK 面高亮（M3-04）仍未接入。

---

## D-039：Qt-free 层的诊断文本是数据，不走 `tr()`；本地化由 UI 层按错误码完成

- 背景：`src/core` 与 `src/io` 按 D-021 与 `docs/ARCHITECTURE.md` §2.1 是 Qt-free 层（`ofgui_core` 不链接任何 Qt 目标，`ofgui_io` 只链接 `Qt6::Core`），而 `tr()` 是 `QObject`/`QCoreApplication` 的翻译机制。但导入与校验的诊断文案天然是中文：实测 `src/core` 7 个文件共 51 行、`src/io/ImportIssueFactory.cpp` 75 行含中日韩字符，合计 126 行（对 `src/core`/`src/io` 全部 `.h`/`.cpp` 逐行按 `[\u4e00-\u9fff]` 匹配，含注释行）。若为这些文本在 core/io 引入 `tr()`，要么把 Qt 依赖塞进纯 C++ 层、破坏 §2.1 的边界表，要么自造翻译框架；两者都不划算。评审中“core/io 有中文却没有 `tr()`”曾被记为 F2 低风险缺口（`docs/VERIFICATION.md` §6.4 与 F2 行，“待文档决策”），本条即为该决策。
- 决策：
  1. **定性**：core/io 的诊断文本是**数据**（随 `ValidationIssue`/`ImportReport` 流动的载荷），不是 UI 文本。`ValidationIssue::message`、`suggestedFix`、`ImportIssueFactory` 的 `IMPORT-*` 码表条目、`ValidationResult::formatCounts()` 的计数摘要，一律以 UTF-8 `std::string` 直出，**不使用 `tr()`**，也不进入 `.ts` 翻译流程。
  2. **本地化归属 UI 层**：需要外语或改写文案时，由 `src/ui` 依据稳定 `ValidationIssue::code`（`IMPORT-*`/`CASE-*`/`EXPORT-*`，见 `docs/CASE_IMPORT_SPEC.md` 附录 C）做“码 → 文案”映射（映射表放在 UI 侧，一个码表一个文件），core/io 的原文本作为回退显示与日志内容。
  3. **契约是码不是文案**：稳定标识为 `code` + `location`/`nodePath`。`ValidationNavigator`、`ExportDictionaryCheck`、测试断言一律匹配 `code`（或 `targetField`/`targetObjectId`），**禁止**匹配中文文案；规格 §8.3 的排序键已按 `severity → file → line → code`。
  4. **UI 自有文案仍必须 `tr()`**：severity 标签（`ValidationPanel`、`NodeDetailHtml`、`ExportController` 的“提示/警告/错误/阻断”）、按钮、标题、工具提示、占位符继续走 `QObject::tr()`/`QCoreApplication::translate()`。`AGENTS.md` §10“所有用户可见文本通过 `tr()`”约束的是 **UI 层**，不约束 core/io 的数据文本。
  5. **新增 core/io 文案的写法**：直接写 UTF-8 中文字面量（源文件必须 UTF-8 无 BOM，见 `docs/STATUS.md` 已知问题），每条必须配稳定 `code`；不得为此新增 Qt 依赖、不得 `#include <QObject>`、不得引入 `QString`。
- 影响范围：`docs/CASE_IMPORT_SPEC.md` §6.6 `message` 注释、§7 通用规则、新增 §8.7、§10.1 第 9 条与 §10.8 检查项 10；`docs/STATUS.md` 已知问题。代码侧无行为变化——本决策是把既成事实写成规则，防止后续评审与重构误判。
- 已知边界：当前 `ValidationPanel`、`NodeDetailHtml`、`ExportPreviewDialog` 直接显示 `issue.message()`，尚无“码 → 文案”映射表；在引入英文/多语言 UI 之前必须先补该映射，否则非中文界面仍会直出中文。`.ts` 覆盖率统计只针对 `src/ui`（以及未来的映射表），core/io 文案不计入翻译统计。

---

## D-040：transport 编辑值本轮不持久化，由 UI 显式告知；schema v3 持久化列入 Backlog

- 背景：t14 评审 F-05 指出 `transportModel`/`nu` 的编辑值只存在于会话内——不写入 `.ofgproj`，关闭工程后丢失，而导入/编辑界面没有明确说明（`docs/STATUS.md` 已知问题 t12 边界）。本轮有两条路线：A 立即把 transport 纳入 `.ofgproj`（升 schemaVersion 3 + 迁移 + 读回），B 保持 v2 结构、在 UI 明示限制。选 B：transport 的 `nu` 支持带量纲写法与表达式（D-037），把它结构化持久化需要先定“原文保真 vs 数值化”的模型语义，属于独立的模型缺口；在评审修复窗口里改 schema 会连带 t17 的迁移测试与 `ProjectJsonSchema` 键集，风险高于收益。UI 明示已在本轮由 `SolverEditor` 落地（`SolverEditorRefresh.cpp` 的 `persistenceNote`）。
- 决策：
  1. **不写 `.ofgproj`**：`schemaVersion` 保持 2，`ProjectJson*` 不改结构、不加 `transport` 段；transport 编辑值仅存在于会话内的单一对象（`CaseDocumentController` 持有），由 `ExportController::makeOptions()` 带进 `io::CaseExportOptions.transport`（D-037）。
  2. **UI 必须明示**：`SolverEditor` 的 constant 页常驻提示「`transportModel`/`nu` 不随 `.ofgproj` 保存；重新打开工程时会按源算例的 `constant/transportProperties` 重读」。保存、另存为、重开工程时不得静默回退；提示文案属于 UI 自有文案，走 `tr()`（D-039）。
  3. **只在可写时承诺回写**：`nu` 为可解析的 kinematic 标量时，提示补充“会写入 `constant/transportProperties`（导出预览可核对）”；带量纲/表达式写法保持只读并原样回写（D-037）。
  4. **v3 持久化进 Backlog**（`docs/TASKS.md`）：实现时必须同时升 `schemaVersion` 到 3、补 v2→v3 内存迁移、保持未知键/未知段保留、并新增“保存 → 关闭 → 重开 → transport 值仍在”的往返测试；在此之前不得以“临时写段”的方式半持久化。
- 影响范围：`src/ui/SolverEditor*`、`src/ui/CaseDocumentControllerDictionaries.cpp`、`docs/CASE_IMPORT_SPEC.md` §7.6/§10.9.2、`docs/STATUS.md` 已知问题、`docs/TASKS.md` Backlog、`docs/ARCHITECTURE.md` §5.6。`src/io/ProjectJson*` 与本决策无关（明确不改）。
- 已知边界：关闭工程后 transport 编辑值丢失，重开按源算例重读；源算例没有 `constant/transportProperties` 时按 OpenFOAM 默认（`Newtonian` + 空 `nu`）解释并报 `IMPORT-TRANS-001`（Info）。导出只会改写该文件的 `transportModel`/`nu` 两个键，其余键按 D-019 保真镜像。

---

## D-041：io 子功能文件按域命名，`Helpers`/`Support`/`Types` 杂物桶零容忍

- 背景：t14 评审 F-06 指出 `src/io/CaseImporterHelpers.*`、`src/io/CaseScannerHelpers.*` 属 `docs/CASE_IMPORT_SPEC.md` §10.1 明令禁止的杂物桶命名（文件名必须能直接检索到职责，`Helpers` 只会吸引更多无关函数）。这两组文件各自混了两个不同的域：文件系统探针 + 名字规则；字典装载 + 报告记账。
- 决策：
  1. **按域拆名**：`CaseScannerHelpers` 拆成 `CaseDirectoryProbe.{h,cpp}`（`isDirectory`/`isRegularFile`/`relativePath`/`fileSize`/`polyMeshComplete`）与 `CaseNameRules.{h,cpp}`（`isNumericName`/`kindForSystemFile`）；`CaseImporterHelpers` 拆成 `ImportDictionaryLoader.{h,cpp}`（`parseDictionary`/`hasIncludeDirective`/`hasParseError`/`fileNameOf`）与 `ImportReportBookkeeping.{h,cpp}`（`sortIssues`/`findFile`/`defaultLevel`）。
  2. **零行为变更**：函数体、命名空间 `ofgui::io::detail`、诊断码与消息文本逐字未动，只搬文件、改 include、改 CMake 登记行。四个新文件均 ≤400 行（D-021）。
  3. **命名规则固化**：此后 `src/io` 下任何 `(Support|Helpers|Types)\.(h|cpp)$` 都是违规；跨用例共享的小逻辑按"它服务的域"命名（探针、规则、装载器、记账），不建通用工具库。测试辅助头（`tests/io/*TestSupport.h`）不受此限，它们不是产品代码且只服务单一测试目标。
  4. **消费方同步**：`CaseScanner.cpp`（探针 + 名字规则）、`CaseImporter.cpp`（装载器 + 记账）、`writers/SourceCaseWalker.cpp`（探针）、`writers/ExportSkipRules.cpp`（名字规则）；`docs/CASE_IMPORT_SPEC.md` §10.2.1 映射表的文件名已同步。
- 影响范围：删除 `src/io/CaseImporterHelpers.*`、`src/io/CaseScannerHelpers.*`；新增 `src/io/CaseDirectoryProbe.*`、`src/io/CaseNameRules.*`、`src/io/ImportDictionaryLoader.*`、`src/io/ImportReportBookkeeping.*`；`src/io/CMakeLists.txt` 显式源列表与四个消费方 include。`docs/DECISIONS.md` D-032 里出现的旧名属历史记录，不再改写。
- 已知边界：文件名变更对不经过 CMake 的外部脚本/IDE 项目文件是破坏性的；本仓库没有此类引用（已全仓 grep 确认 0 命中），且 `OpenFOAM-v2206/` 不参与构建，不受影响。