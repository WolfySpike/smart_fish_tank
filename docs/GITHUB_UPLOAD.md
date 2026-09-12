# 上传到 GitHub

[返回 README](../README.md)

仓库根目录是包含 `README.md`、`PCB/` 和 `smart_system/` 的这一层。Keil 工程位于下一层 `smart_system/MDK-ARM/`。本次整理已在项目根目录初始化独立的 Git 仓库，分支为 `main`；GitHub 远程仓库由上传者创建和绑定。

## 创建空仓库

在 GitHub 点击 **New repository**。可参考以下设置：

| 项目 | 建议内容 |
| --- | --- |
| Repository name | `stm32-aquarium-controller` |
| Description | 基于 STM32F103C8T6 的鱼缸自动喂食与照明课设，包含 DS1302 定时、SG90 喂食、水位检测、OLED 菜单及 PCB 设计。 |
| Visibility | 根据分享范围选择 Public 或 Private |
| 初始化选项 | 保持为空；本地已包含 README、`.gitignore` 和 MIT LICENSE |
| Topics | `stm32`、`stm32f103`、`embedded-c`、`aquarium`、`fish-feeder`、`oled`、`ds1302`、`keil`、`course-project` |

## 首次提交

在 PowerShell 中切换到项目根目录。下面使用当前电脑上的路径；在其他电脑上需替换为实际位置：

```powershell
Set-Location 'C:\Users\admin\Desktop\smart_system'
git rev-parse --show-toplevel
git status --short
```

`git rev-parse` 应返回这个项目目录。如果是从不含 `.git` 的文件副本开始，请先在项目根目录运行 `git init -b main`，再检查根目录。

确认目录后添加文件并查看本次提交的范围：

```powershell
git add .
git diff --cached --stat
git diff --cached --name-only
```

应包含固件源码、Keil `.uvprojx`、CubeMX `.ioc`、说明文档和硬件资料。忽略规则会排除编译目录、`.bak`、Keil 个人选项、VS Code 本机设置与构建日志；第三方驱动的许可文件和预编译库会保留。

然后创建首次提交：

```powershell
git commit -m "Initial commit: aquarium feeder and lighting controller"
git branch -M main
```

如果 Git 提示未设置提交者身份，可以仅为此仓库设置署名和自己的 GitHub 提交邮箱，然后重新执行提交命令：

```powershell
git config user.name "你的署名"
git config user.email "你的 GitHub 提交邮箱"
```

## 绑定远程并推送

把下面示例 URL 替换为你刚创建的仓库地址，`YOUR_USERNAME` 是占位符：

```powershell
git remote add origin https://github.com/YOUR_USERNAME/stm32-aquarium-controller.git
git remote -v
git push -u origin main
```

按 Git 的认证提示登录自己的 GitHub 账号。推送成功后打开仓库页面，README 会自动展示项目说明与 PCB 预览。

如果提示 `origin already exists`，先执行 `git remote -v` 核对地址。需要更换时，使用 `git remote set-url origin` 加上正确的仓库 URL，再推送。

## 后续更新

修改代码或文档后，在同一项目根目录执行：

```powershell
git status --short
git add .
git diff --cached --stat
git commit -m "Describe the changes"
git push
```

提交消息应改为这次修改的实际内容。重新编译产生的中间文件会继续被忽略。

## 上传方式说明

推荐使用 Git 命令或 GitHub Desktop 添加这个本地仓库并发布，它们会使用 `.gitignore`。网页拖拽上传不会替你应用本地忽略规则，且本项目含大量驱动文件，容易漏选文件或混入本机输出。

`README.md` 应直接出现在仓库根目录，图片和文档需要保持当前相对路径。Gerber ZIP 是硬件制造资料，可以正常作为文件提交；将整个项目打包成一个 ZIP 则不会让 GitHub 自动展开项目首页。

如需分享编译好的 HEX，可在验证对应固件后，将它作为 GitHub Release 附件，并在说明中记录源码提交和构建环境。
