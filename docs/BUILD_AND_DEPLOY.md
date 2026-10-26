# 构建与部署说明

> 对应任务：M0-03  
> 应用：独立 Qt 6 + VTK Windows 桌面程序  
> 非目标：不编译、不链接、不调用 OpenFOAM

## 1. 固定工具链

- Qt：`D:\3.wpsAndOther\Qt\setting\6.11.2\mingw_64`
- Qt bin：`D:\3.wpsAndOther\Qt\setting\6.11.2\mingw_64\bin`
- CMake：`D:\3.wpsAndOther\Qt\setting\Tools\CMake_64\bin\cmake.exe`
- CTest：`D:\3.wpsAndOther\Qt\setting\Tools\CMake_64\bin\ctest.exe`
- Ninja：`D:\3.wpsAndOther\Qt\setting\Tools\Ninja\ninja.exe`
- MinGW bin：`D:\3.wpsAndOther\Qt\setting\Tools\mingw1310_64\bin`
- VTK：`D:\3.wpsAndOther\VTK\install`
- VTK bin：`D:\3.wpsAndOther\VTK\install\bin`
- windeployqt：`D:\3.wpsAndOther\Qt\setting\6.11.2\mingw_64\bin\windeployqt.exe`

已确认版本：

- Qt `6.11.2`
- CMake `3.30.5`
- MinGW `13.1.0`
- VTK `9.6.2`
- Ninja `1.12.1`

---

## 2. 开发构建脚本

### 2.1 Debug 构建

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass `
  -File '.\scripts\Build-Project.ps1' -Configuration Debug
```

执行内容：

1. 设置 Qt、MinGW、Ninja、VTK 的 PATH。
2. 使用 `windows-mingw-debug` preset 配置。
3. 使用 Ninja 构建。
4. 运行 CTest。

输出：

`build\windows-mingw-debug\OpenFOAMGUI.exe`

### 2.2 Release 构建

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass `
  -File '.\scripts\Build-Project.ps1' -Configuration Release
```

输出：

`build\windows-mingw-release\OpenFOAMGUI.exe`

### 2.3 运行构建目录中的程序

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass `
  -File '.\scripts\Run-Application.ps1' -Configuration Debug
```

Release：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass `
  -File '.\scripts\Run-Application.ps1' -Configuration Release
```

Smoke test：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass `
  -File '.\scripts\Run-Application.ps1' -Configuration Release -SmokeTest
```

---

## 3. 手动 CMake 命令

脚本内部使用以下等价命令。需要先在当前 PowerShell 中设置 PATH。

```powershell
$env:Path = 'D:\3.wpsAndOther\Qt\setting\6.11.2\mingw_64\bin;D:\3.wpsAndOther\Qt\setting\Tools\mingw1310_64\bin;D:\3.wpsAndOther\Qt\setting\Tools\Ninja;D:\3.wpsAndOther\VTK\install\bin;' + $env:Path
```

Debug：

```powershell
& 'D:\3.wpsAndOther\Qt\setting\Tools\CMake_64\bin\cmake.exe' --preset windows-mingw-debug
& 'D:\3.wpsAndOther\Qt\setting\Tools\CMake_64\bin\cmake.exe' --build --preset windows-mingw-debug
& 'D:\3.wpsAndOther\Qt\setting\Tools\CMake_64\bin\ctest.exe' --preset windows-mingw-debug --output-on-failure
```

Release：

```powershell
& 'D:\3.wpsAndOther\Qt\setting\Tools\CMake_64\bin\cmake.exe' --preset windows-mingw-release
& 'D:\3.wpsAndOther\Qt\setting\Tools\CMake_64\bin\cmake.exe' --build --preset windows-mingw-release
& 'D:\3.wpsAndOther\Qt\setting\Tools\CMake_64\bin\ctest.exe' --preset windows-mingw-release --output-on-failure
```

---

## 4. 发布部署

### 4.1 一键部署

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass `
  -File '.\scripts\Deploy-Release.ps1' -BuildFirst
```

仅重新部署已经构建好的 Release：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass `
  -File '.\scripts\Deploy-Release.ps1'
```

输出目录：

`dist\OpenFOAMGUI\`

### 4.2 部署流程

`Deploy-Release.ps1` 执行以下步骤：

1. 将 Release 可执行文件复制到 `dist\OpenFOAMGUI`。
2. 调用 Qt `windeployqt`：
   - 复制 Qt DLL。
   - 复制 Qt 插件。
   - 复制 MinGW 编译器和 C++ 运行库。
   - 复制 OpenGL 软件回退库。
3. 扫描 PE 导入表，递归收集 VTK/MinGW 传递依赖。
4. 在只包含 Windows 系统目录的最小 PATH 下运行部署版。
5. 生成部署 smoke test 截图。

### 4.3 依赖收集策略

`scripts\Collect-PeDependencies.ps1` 使用 MinGW `objdump.exe -p` 读取 PE 导入表：

- 从 `OpenFOAMGUI.exe` 开始。
- 递归扫描 Qt、VTK 和 MinGW DLL。
- 仅在目标目录、Qt bin、MinGW bin、VTK bin 中查找。
- 忽略找不到的 Windows 系统 DLL。
- 将找到的依赖复制到部署目录。

这种方式避免人工维护 VTK DLL 清单。新增 VTK 模块后，只要重新运行部署脚本即可。

### 4.4 最小 PATH 验证

部署脚本内部临时使用：

```powershell
$env:Path = "$env:SystemRoot\System32;$env:SystemRoot"
```

在此 PATH 下启动：

`dist\OpenFOAMGUI\OpenFOAMGUI.exe --smoke-test --screenshot <path>`

如果启动成功，说明程序不依赖开发机中的 Qt、VTK 或 MinGW PATH。

---

## 5. M0-03 实际验证结果

- Debug 构建：通过。
- Debug CTest：`1/1` 通过。
- Release 构建：通过。
- Release CTest：`1/1` 通过。
- `windeployqt`：通过。
- 传递依赖收集：额外复制 62 个 DLL。
- 部署文件总数：94。
- 部署 DLL 总数：92。
- 部署大小：约 297.2 MB。
- 最小 PATH 启动：通过。
- 部署截图：`dist\OpenFOAMGUI\deployment-smoke.png`。

### 已知非阻塞警告

- CMake 未找到 `WrapVulkanHeaders`：当前未使用 Vulkan 后端，不影响构建和运行。
- `windeployqt` 未找到 `dxcompiler.dll` / `dxil.dll`：当前没有使用 Direct3D 12 特性，不影响运行。

---

## 6. PATH 说明

### 开发构建和运行

需要：

- Qt bin
- MinGW bin
- Ninja 目录
- VTK bin

脚本 `Set-ProjectEnvironment.ps1` 已集中处理这些路径。

### 部署版运行

不需要上述开发路径。部署目录中的：

- `OpenFOAMGUI.exe`
- Qt DLL
- VTK DLL
- MinGW runtime DLL
- `platforms\qwindows.dll`
- 其他 Qt 插件

共同构成独立运行环境。

---

## 7. 常见问题

### 程序提示缺少 Qt6Core.dll

说明没有通过脚本运行，或者部署目录不完整。重新执行：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass `
  -File '.\scripts\Deploy-Release.ps1' -BuildFirst
```

### 程序提示找不到平台插件 windows

检查部署目录中是否存在：

`platforms\qwindows.dll`

### VTK 视口为空白或 OpenGL 初始化失败

检查：

- 部署目录中是否存在需要的 `libvtk*.dll`。
- 显卡驱动是否支持当前 OpenGL。
- 是否复制了 `opengl32sw.dll`。

### 新加 VTK 模块后缺少 DLL

重新运行部署脚本。依赖闭包会从新的可执行文件重新扫描。