在当前项目中需要记住这些地址：

cmake安装地址：
```
D:\3.wpsAndOther\Qt\setting\Tools\CMake_64\bin
```

Qt安装地址
```
：D:\3.wpsAndOther\Qt\setting
```

VTK安装地址：
```
D:\3.wpsAndOther\VTK\install
```

我现在需要开发一款OpenFOAM的可视化操作的GUI界面软件：
基本的要求是这样的。

* 在这个GUI界面中实现自定义计算域的区域，通过设置计算域的最小点（minX minY minZ）和最大的点（maxX maxY maxZ）实现设置整个计算域。其设置类似StarCCM+的功能。可以选择某一些面实现自定义边界名称和属性。
* 整体界面左侧区域类似StarCCM+，可以选择不同设置实现定义OpenFOAM的文件，当定义完成后写入到OpenFOAM文件中。即创建0.orig/ constant/ system/ 文件夹。
* 在右侧区域为整体的可视化区域，即可以观察到整个计算域的结构，其可视化结构类似paraview。

在当前文件中我放置了OpenFOAM-v2206的源代码。我现在的需求是基于Qt cmake和VTK开发一款OpenFOAM的界面操作软件。
帮我分析应都包含哪些功能和哪些事项，先分析，不着急开展开发
---

## 开发文档入口

本项目当前只开发独立的 Qt 6 + VTK 可视化界面，不编译、不链接、不嵌入 OpenFOAM 源码。`OpenFOAM-v2206/` 仅作为字典格式参考，不参与应用构建。

- [开发计划与 Agent 路径](docs/DEVELOPMENT_PLAN.md)
- [架构设计](docs/ARCHITECTURE.md)
- [当前状态](docs/STATUS.md)
- [任务队列](docs/TASKS.md)
- [构建与部署说明](docs/BUILD_AND_DEPLOY.md)
- [Git 分支与更新流程](docs/GIT_WORKFLOW.md)
- [架构决策记录](docs/DECISIONS.md)
- [Agent 项目规则](AGENTS.md)

## 快速构建、运行与部署

```powershell
# Debug 构建并测试
powershell.exe -NoProfile -ExecutionPolicy Bypass -File '.\scripts\Build-Project.ps1' -Configuration Debug

# Release 构建并测试
powershell.exe -NoProfile -ExecutionPolicy Bypass -File '.\scripts\Build-Project.ps1' -Configuration Release

# 运行 Debug
powershell.exe -NoProfile -ExecutionPolicy Bypass -File '.\scripts\Run-Application.ps1' -Configuration Debug

# Release 部署，并在最小 PATH 下自动验证
powershell.exe -NoProfile -ExecutionPolicy Bypass -File '.\scripts\Deploy-Release.ps1' -BuildFirst

# 归档旧 main、提交新 main，并推送到 GitHub
powershell.exe -NoProfile -ExecutionPolicy Bypass -File '.\scripts\Publish-ToGitHub.ps1' -Message 'chore: publish update'
```

部署目录：`dist\OpenFOAMGUI\`

完整的 PATH、手动 CMake 命令、DLL 收集和故障排查见 [构建与部署说明](docs/BUILD_AND_DEPLOY.md)。

## 使用说明（当前可用流程）

1. 启动 `OpenFOAMGUI.exe`：默认加载内置 cavity 模板（2D、20×20×1、icoFoam），无需手工补值即可直接导出。
2. **打开算例**（`Ctrl+O`）选择 v2206 算例根目录（例如 `OpenFOAM-v2206/tutorials/incompressible/icoFoam/cavity/cavity`），左侧工程树显示计算域、网格、边界（六面）、`0/` 场与求解设置。
3. 在工程树选择节点后在下方/右侧编辑：计算域与网格（min/max、cells、2D/3D）、场（internalField、逐 patch BC）、求解设置（application、时间控制、transportModel/nu）。非法值会就地提示且不写入模型。
4. 底部“校验问题”面板按级别列出问题；双击问题可跳到对应节点。存在错误或阻断问题时导出动作不可用。
5. **导出算例**（`Ctrl+E`）：选择目标目录，预览文件清单与生成文本，确认后写入。目标目录已有内容时需勾选覆盖并二次确认，旧目录会备份为 `<目标>.bak-<时间戳>`；导出失败不会留下半成品目录。
6. 工程本身可保存为 `.ofgproj`（`Ctrl+S` / `Ctrl+Shift+S`），与导出的 OpenFOAM 算例相互独立。

导出结果包含 `0/<field>`、`system/blockMeshDict`、`system/controlDict`、`system/fvSchemes`、`system/fvSolution`、`constant/transportProperties`，以及镜像/复制自源算例的其余文件；目标目录内的 `.ofgport-export.json` 记录每个文件的动作与大小。

以后只需对 Agent 说 **“提交到github”**，即可自动执行归档、提交和推送。详细规则见 [Git 分支与更新流程](docs/GIT_WORKFLOW.md)。