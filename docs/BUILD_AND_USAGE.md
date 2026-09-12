# 编译、烧录与使用

[返回 README](../README.md) · [硬件接线](HARDWARE.md)

## 工程与环境

| 项目 | 工程记录 |
| --- | --- |
| MCU | STM32F103C8T6，Cortex-M3，标称 64 KB Flash / 20 KB RAM |
| Keil 工程 | [`smart_system/MDK-ARM/smart_fisher.uvprojx`](../smart_system/MDK-ARM/smart_fisher.uvprojx) |
| 构建目标 | `smart_fisher` |
| C 编译器 | ARM Compiler 5.06 update 7，build 960；工程 `uAC6=0` |
| STM32 器件包 | `Keil.STM32F1xx_DFP.2.4.1` |
| RTE 组件 | 工程引用 `ARM.CMSIS.4.5.0` 中的 CMSIS CORE 4.3.0 |
| CubeMX 配置记录 | STM32CubeMX 6.8.1，STM32Cube FW_F1 V1.8.7 |
| CubeMX 工程 | [`smart_system/smart_fisher.ioc`](../smart_system/smart_fisher.ioc) |

日常编译可直接打开 Keil 工程。仓库随附 HAL 与 CMSIS 源码；若 Keil 提示缺少器件包或 RTE 组件，按上表通过 Pack Installer 补齐。CubeMX 用于修改外设配置，直接构建现有固件不要求先重新生成代码。

工程目前使用 ARM Compiler 5，并需要可用的编译器许可。只安装 ARM Compiler 6 的环境需要补装并选择 Compiler 5；迁移编译器后应重新检查启动文件、库与编译选项，不能直接视为等价环境。

## 编译

1. 打开 `smart_system/MDK-ARM/smart_fisher.uvprojx`。
2. 在目标下拉框选择 `smart_fisher`。
3. 打开 **Options for Target → Target**，确认器件为 STM32F103C8、编译器为 ARM Compiler 5。
4. 执行 **Project → Rebuild all target files**，检查构建结果。
5. 在 `smart_system/MDK-ARM/smart_fisher/` 查看输出的 `smart_fisher.axf` 和 `smart_fisher.hex`。

该输出目录、个人调试选项和日志已由 `.gitignore` 排除。首次打开克隆后的工程时，Keil 会重新生成本机选项。

Windows PowerShell 中也可以调用 Keil 命令行。将示例中的 Keil 安装路径改为自己的路径，在仓库根目录执行：

```powershell
& 'C:\Keil_v5\UV4\UV4.exe' -r '.\smart_system\MDK-ARM\smart_fisher.uvprojx' -t 'smart_fisher' -o '.\smart_system\MDK-ARM\rebuild.log'
```

## 烧录

1. ST-Link 连接 `SWDIO → PA13`、`SWCLK → PA14`、GND，并按下载器要求连接目标电压参考；需要时连接 NRST。
2. 在 **Options for Target → Debug** 选择 **ST-Link Debugger**，进入 **Settings** 并选择 **SW** 接口。
3. 在 **Utilities** 中使用对应调试器进行 Flash 下载，检查已添加适合 STM32F103C8 的 Flash 算法。
4. 确认目标板供电和 BOOT0 启动设置，执行 **Flash → Download**。
5. 复位进入用户程序，检查 OLED 显示，再进行按键和执行机构联调。

重新烧录时是否保留参数取决于下载器的擦除范围；整片擦除会清除最后一页中的配置。

## RTC 时间初始化

[`app_internal.h`](../smart_system/User/app_internal.h) 中的 `APP_RTC_SYNC_FROM_BUILD_AT_BOOT` 默认为 `0U`：

| 值 | 启动行为 |
| --- | --- |
| `0U` | DS1302 已正常计时时保留时间；检测到停振或非法时间时尝试写入编译时间 |
| `1U` | 每次启动都尝试将 DS1302 设置为该固件的编译时间 |

编译时间由 `app_time.c` 中的 `__DATE__` 和 `__TIME__` 提供，是编译时电脑的日期和时间，不会自动随烧录日期更新。

需要用电脑时间近似校时时，将宏临时设为 `1U`，完整重新编译、烧录并启动一次；随后改回 `0U`，再次编译烧录，使后续启动保留 DS1302 计时。编译与烧录耗时会形成时间偏差。需要精确设定时，可在开发调试中调用 `DS1302_SetDateTime()`。

DS1302 应安装并正确连接备用电池，以维持断电计时。当前按键菜单设置的是照明和喂食时刻，没有 RTC 日期／时间编辑页面。

## 菜单与参数

主界面显示日期、星期、时分、两路灯状态、当日计划触发计数和水位百分比。`喂食 x/y` 统计的是计划已标记触发的时点数，手动喂食不计入该数字。

按 K0 依次进入以下页面，再次按 K0 切换页面，从最后一页退出时保存配置：

| 页面 | 可设置内容 |
| --- | --- |
| 鱼缸灯 | `ON` 开灯时刻、`OFF` 关灯时刻 |
| 补光灯 | `ON` 开灯时刻、`OFF` 关灯时刻 |
| 喂食 | `Feed Times` 每日次数、`Rotate` 每次摆动次数、`Time1～Time3` 喂食时刻 |

K3 / K4 向前／向后选择字段，时间字段按小时、分钟分别选择。K1 加、K2 减；消抖时间为 20 ms，长按约 500 ms 后开始连发，间隔约 120 ms。未启用的喂食时点显示 `--:--`，不会参与调度。

设置会立即修改 RAM 中的运行参数，菜单打开时定时调度仍继续执行。退出菜单前断电，尚未保存的修改会丢失；当前没有取消修改的操作。

## 调度规则

- 照明时段包含开始分钟，不包含结束分钟。`08:00 → 22:00` 在 22:00 关闭，`22:00 → 06:00` 跨午夜运行，起止时间相同则关闭。
- 喂食匹配小时和分钟，在匹配分钟内尝试触发。每个启用的时点通过 RAM 标志位防止同日重复触发，日期变化时清零。
- 错过时点不会补执行；重启后触发标记清空，如果仍处于匹配分钟内，可能再次触发。
- 舵机忙碌时 `App_FeedStart()` 不接受新的请求，而调度层仍会标记该时点已触发。设置相同或过近的时点，或在定时喂食前手动喂食，可能使该次计划没有执行动作。
- 摆动次数决定动作循环数，具体投喂量取决于机械结构与饲料，需要实物标定。

## Flash 参数存储

[`app_storage.c`](../smart_system/User/app_storage.c) 读取芯片容量寄存器，将配置写入 **Flash 最后一个 1 KB 页面**。数据包含标识、版本和校验值；加载失败时保留程序内默认配置。

对于容量寄存器报告 64 KB 的芯片，配置页为 `0x0800FC00～0x0800FFFF`。当前 Keil 工程仍按完整的 `0x10000` 字节配置 IROM1，未在链接设置中显式扣除该页。扩展固件时，应预留配置页并检查 MAP 文件；64 KB 目标可将 IROM1 大小限制为 `0xFC00`。其他容量应按实际存储地址调整。

修改源码中的默认参数不会覆盖已保存的有效配置。可通过菜单重新设置，或在明确需要恢复默认值时擦除配置页；整片擦除还会同时移除固件，需要重新烧录。

## 复现检查

以下为上板检查步骤，不代表已经完成的实物测试记录。

| 检查 | 操作与预期 |
| --- | --- |
| 基础显示 | 上电后 OLED 显示日期与状态；正常计时并跨分钟更新 |
| 手动喂食 | 主界面短按 K1，舵机按设定次数摆动并回中位 |
| 设置保存 | 修改时段并从喂食页按 K0 退出，断电重启后参数保持 |
| 定时灯光 | 设置临近的开关时刻，观察两路灯分别开关；另验证跨午夜时段 |
| 自动喂食 | 设置下一分钟为喂食时点，确认触发及同一分钟内的去重 |
| 水位反馈 | 观察不同水位下 ADC、指示灯与百分比，按实际读数标定 |
| RTC 断电计时 | 使用有效备用电池，断开主电源后重启，检查时间是否连续 |

## 常见问题

| 现象 | 排查方向 |
| --- | --- |
| Keil 找不到 Compiler 5 或器件包 | 安装工程记录的编译器／软件包，在目标选项中重新选择 |
| `C9555E` / `A9555E`，`Failed to check out a license` | 在 Keil 的 License Management 中检查本机授权，恢复有效的 Compiler 5 许可后重新构建；此错误表示编译器未通过授权校验 |
| OLED 不显示或偏移 | 核对 PB8/PB9、供电、I²C 上拉和屏幕型号；驱动当前使用写地址 `0x78`（7 位地址 `0x3C`），并在 `OLED_SetCursor()` 中启用 `X += 2` |
| OLED 显示 `1302 时间 ?` | 核对 PB12/PB13/PB14、模块电源与 RTC 接线；此界面表示重试后仍未读到有效时间 |
| 每次重启回到旧时间 | 检查强制校时宏是否仍为 `1U`，以及备用电池和停振状态 |
| 修改默认值没有生效 | Flash 中已保存有效配置，启动时会覆盖源码默认值 |
| 舵机动作导致复位 | 检查 5 V 电源电流能力、共地及电源去耦 |
| 水位百分比不符合实际 | 查看原始采样与输出方向，重新设置阈值及百分比映射区间 |

重新使用 CubeMX 生成代码后，应检查 `User/`、`Hardware/` 是否仍包含在 Keil 工程中，以及 `main.c` 的 `USER CODE` 区域是否保留应用初始化和任务调用。
