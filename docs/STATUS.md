# 项目状态

> 最后更新：2026-10-26  
> 当前阶段：M0 技术基线已完成  
> 当前任务：准备进入 M1 工程模型与持久化

## 当前结论

- 项目定位：独立 Qt 6 + VTK 桌面 GUI。
- 不编译、不链接、不嵌入 OpenFOAM。
- M0-01、M0-02、M0-03、M0-04 已完成。
- Debug/Release 构建、测试、部署和最小 PATH 启动均已验证。
- Git 仓库已建立，`main` 保存最新版本。
- 后续更新使用自动归档分支流程。

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

## Git 分支规则

- `main`：最新版本。
- `archive/<时间>-<旧main短SHA>`：每次更新前的旧版本。
- 归档分支只读。
- 每次更新运行 `scripts\Commit-Update.ps1`。
- 固定触发词 `提交到github` 会自动执行归档、提交、推送和远程校验。

## 构建与部署结果

- Debug 构建：通过。
- Debug CTest：`1/1` 通过。
- Release 构建：通过。
- Release CTest：`1/1` 通过。
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

进入 M1-01：

1. 实现 `CaseProject` 和 `DomainBox` 模型。
2. 支持 min/max、单位、2D/3D 模式。
3. 实现有效性校验。
4. 添加模型单元测试。

## 已知问题

- Vulkan headers 未安装，但当前不使用 Vulkan。
- windeployqt 提示缺少 DXC DLL，但当前不使用 Direct3D 12。

## 最近变更

- 2026-10-26：初始化 Git 仓库并建立 M0 基线。
- 2026-10-26：建立旧 main 归档分支和最新 main 更新流程。
- 2026-10-26：配置 GitHub origin 并首次推送 main。
- 2026-10-26：新增“提交到github”自动发布规则和一键发布脚本。