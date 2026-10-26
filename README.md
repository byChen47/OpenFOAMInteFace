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
以后只需对 Agent 说 **“提交到github”**，即可自动执行归档、提交和推送。详细规则见 [Git 分支与更新流程](docs/GIT_WORKFLOW.md)。