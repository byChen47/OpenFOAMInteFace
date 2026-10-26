# Git 分支与更新流程

## 0. 固定触发词

以后只需对 Agent 说：

```text
提交到github
```

等同于：

1. 检查并验证当前修改。
2. 自动生成提交信息。
3. 归档旧 `main`。
4. 将新修改提交到 `main`。
5. 推送归档分支和最新 `main`。
6. 验证远程分支与本地提交一致。

该规则已写入 `AGENTS.md`，后续 Agent 不需要再次询问提交或推送步骤。

一键脚本：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass `
  -File '.\scripts\Publish-ToGitHub.ps1' `
  -Message 'chore: publish update'
```

如果不提供 `-Message`，脚本会自动生成带时间戳的提交信息。
## 1. 分支规则

- `main`：始终指向最新版本。
- `archive/<时间>-<旧main短SHA>`：保存更新前的 `main` 状态。
- `archive/*` 分支视为只读历史，不再继续开发。
- 不使用强制推送覆盖历史。

示例：

```text
main
archive/20261026-103000-a1b2c3d
archive/20261026-151500-f4e5d6a
```

## 2. 初始状态

首次建立仓库时只有 `main`。下一次发生更新时才会创建第一个归档分支。

## 3. 每次更新

在 `main` 上完成修改后执行：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass `
  -File '.\scripts\Commit-Update.ps1' `
  -Message 'feat: describe the update'
```

脚本执行顺序：

1. 检查当前分支是 `main`。
2. 检查存在未提交修改。
3. 从当前 `main` 创建一个归档分支。
4. 将全部修改加入暂存区并提交到 `main`。
5. 输出归档分支名称。

因此：

- 旧版本保存在新创建的 `archive/*` 分支。
- 新版本进入 `main`。

## 4. 同时推送到远程仓库

当前远程仓库已配置为 `origin`：

```text
https://github.com/byChen47/OpenFOAMInteFace.git
```

提交更新并推送 `main` 和旧版本分支：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass `
  -File '.\scripts\Commit-Update.ps1' `
  -Message 'feat: describe the update' `
  -Push
```

脚本会先推送归档分支，再推送 `main`。日常更新优先使用 `Publish-ToGitHub.ps1`。

## 5. 只创建归档分支，不提交更新

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass `
  -File '.\scripts\Archive-MainBranch.ps1'
```

该命令只创建归档分支，不会修改工作区或提交代码。

## 6. 查看版本

```powershell
git branch --all
git log --oneline --decorate --graph --all
```

## 7. 从归档版本恢复

只查看旧版本：

```powershell
git switch --detach <archive-branch>
```

基于旧版本创建修复分支：

```powershell
git switch -c fix/from-archive <archive-branch>
```

不要直接在 `archive/*` 分支上继续开发。

## 8. 当前远程状态

远程仓库已配置为：

```text
https://github.com/byChen47/OpenFOAMInteFace.git
```

本地分支 `main` 已跟踪 `origin/main`。