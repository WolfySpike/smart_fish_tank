# 硬件与接线

[返回 README](../README.md)

以下信号分配依据当前固件的 `main.h`、`gpio.c`、`tim.c`、`adc.c` 与外设驱动整理。PCB 和原理图保留原始导出文件，制作或改板时需将实际接口与此表对应。

## 模块清单

| 模块 | 数量 | 用途 |
| --- | --- | --- |
| STM32F103C8T6 最小系统板 | 1 | 主控，外部 8 MHz 晶振，运行于 72 MHz |
| DS1302 时钟模块 | 1 | 日期与时间，使用模块备用电池维持断电计时 |
| 128×64 I²C OLED | 1 | 当前代码启用 SH1106 常用的两列偏移，适配 1.3 寸屏 |
| SG90 舵机 | 1 | 喂食执行机构 |
| 模拟输出水位传感器 | 1 | 水位采样 |
| 红、黄、绿指示灯 | 各 1 | 水位状态指示 |
| 鱼缸灯、植物补光灯 | 各 1 | 两路照明输出 |
| 独立按键 | 5 | 菜单和手动喂食 |
| ST-Link 与连接线 | 1 套 | SWD 下载与调试 |

灯的限流／驱动、电源模块与接插件按实际负载和板卡配置选用。

## 引脚分配

| 功能 | STM32 引脚 | 接口／模式 | 说明 |
| --- | --- | --- | --- |
| OLED SCL | PB8 | GPIO 开漏输出 | 软件 I²C 时钟 |
| OLED SDA | PB9 | GPIO 开漏输出 | 软件 I²C 数据 |
| DS1302 CE / RST | PB12 | GPIO 输出 | 片选 |
| DS1302 CLK | PB13 | GPIO 输出 | 串行时钟 |
| DS1302 DAT / IO | PB14 | GPIO 双向切换 | 在 `ds1302.h` 中定义 |
| SG90 信号 | PA8 | TIM1_CH1 PWM | 50 Hz |
| 红色指示灯 | PA6 | TIM3_CH1 PWM | 干燥或高水位 |
| 黄色指示灯 | PA7 | TIM3_CH2 PWM | 中水位 |
| 绿色指示灯 | PB0 | TIM3_CH3 PWM | 低水位 |
| 鱼缸灯 | PB1 | TIM3_CH4 PWM | 开启时默认占空比 90% |
| 补光灯 | PB6 | GPIO 推挽输出 | 高电平开启 |
| 水位传感器 AO / S | PA5 | ADC1_IN5 | 模拟输入 |
| K0 / KEY0 | PA0 | 上拉输入 | 按下接地 |
| K1 / KEY1 | PA2 | 上拉输入 | 按下接地 |
| K2 / KEY2 | PA4 | 上拉输入 | 按下接地 |
| K3 / KEY3 | PB11 | 上拉输入 | 按下接地 |
| K4 / KEY4 | PC14 | 上拉输入 | 已作为按键使用，不接 LSE 晶振 |
| SWDIO | PA13 | SWD | 下载／调试 |
| SWCLK | PA14 | SWD | 下载／调试 |

源码位置：[`main.h`](../smart_system/Core/Inc/main.h)、[`gpio.c`](../smart_system/Core/Src/gpio.c)、[`tim.c`](../smart_system/Core/Src/tim.c)、[`adc.c`](../smart_system/Core/Src/adc.c)、[`ds1302.h`](../smart_system/Hardware/ds1302.h)、[`OLED.c`](../smart_system/Hardware/OLED.c)。

## 供电与接口

- MCU 和信号接口按 3.3 V 逻辑连接，各模块与舵机电源共地。
- SG90 使用满足其负载电流的 5 V 电源，通过 PA8 接收控制信号；不要由 MCU GPIO 为舵机供电。
- PA5 的模拟输入电压应处于 `0～VDDA`，本工程按约 3.3 V 参考电压使用。接入其他水位模块前核对其输出范围。
- OLED 的软件 I²C 引脚配置为开漏，需有上拉到 3.3 V 的电阻；先检查模块是否自带上拉。
- LED 需要限流电阻；大电流灯具需通过合适的三极管或 MOSFET 驱动级连接。固件的引脚电平用于控制驱动级。
- PCB 电源输入与模块电源脚应按原理图区分；最小系统板的 5 V 电源输入不等于 MCU 的 3.3 V 引脚。

## 定时器与 ADC

| 外设 | 当前配置 | 结果 |
| --- | --- | --- |
| 系统时钟 | HSE 8 MHz，PLL ×9 | SYSCLK 72 MHz |
| TIM1 | 定时器时钟 72 MHz，PSC=71，ARR=19999 | 50 Hz，1 个计数对应 1 μs |
| TIM3 | 定时器时钟 72 MHz，PSC=71，ARR=999 | 1 kHz，灯光比较值范围 0～999 |
| ADC1 | IN5，12 位，采样时间 239.5 cycles，软件启动并轮询 | 原始值范围 0～4095 |

SG90 默认中位脉宽为 1500 μs，喂食两端脉宽为 2300 μs / 700 μs，每端停留 350 ms，回中位后等待 250 ms。机械结构不同，需要在 [`app_internal.h`](../smart_system/User/app_internal.h) 中调整 `APP_SERVO_*` 宏。

鱼缸灯亮度由 `APP_FISH_LIGHT_DUTY=900` 决定，当前菜单只调整开关时间。补光灯为 GPIO 开关控制。

## 水位检测与标定

[`app_device.c`](../smart_system/User/app_device.c) 对 ADC 读数做平滑处理，通常每次更新采用 `(旧值 × 3 + 新采样值) / 4`。默认按“水位升高，读数增大”判断。

| 平滑后的判断值 | 状态 | 指示灯 |
| --- | --- | --- |
| `<1200` | DRY：干燥 | 红 |
| `1200～1799` | LOW：低水位 | 绿 |
| `1800～1999` | MID：中水位 | 黄 |
| `≥2000` | HIGH：高水位 | 红 |

OLED 百分比将 `1200～2000` 线性映射到 `0～100%`，区间外分别限制为 0% 和 100%。这是标定区间内的相对读数，不代表鱼缸容量百分比。

标定时可在调试器中观察 `g_waterAdcRaw`，记录干燥、目标低水位、中水位和高水位读数，再修改 `app_internal.h` 中的：

- `APP_WATER_ADC_DRY_TH`、`APP_WATER_ADC_MID_TH`、`APP_WATER_ADC_HIGH_TH`：状态阈值。
- `APP_WATER_ADC_PCT_MIN`、`APP_WATER_ADC_PCT_MAX`：百分比映射端点。
- `APP_WATER_ADC_WET_HIGH`：设为 `0U` 时，判断值改用 `4095 - g_waterAdcRaw`，适配反向输出的传感器。

阈值以宏定义的数值为准，早期记录和部分源码注释中的旧范围不参与实际判断。当前菜单不提供水位阈值编辑。

## PCB 与设计文件

| 资料 | 文件 |
| --- | --- |
| 原理图 PDF | [PCB 原理图](../PCB/PCB原理图_SCH_Schematic2_1_2026-04-20.pdf) |
| PCB 布线图 | [顶层](../PCB/顶层.png)、[底层](../PCB/底层.png) |
| PCB 3D 预览 | [顶层](../PCB/3D顶层.png)、[底层](../PCB/3D底层.png) |
| 嘉立创 EDA 标准版工程 | [电路原理图和 PCB 图.eprj](../电路原理图和PCB图.eprj) |
| 嘉立创 EDA 专业版工程 | [项目导出.epro](../鱼缸喂食控制系统_ProPrj_电路原理图和PCB图_2026-04-20.epro) |
| Gerber 导出 | [Gerber 标准版](../Gerber_标准版_2026-04-20.zip)、[鱼缸喂食控制系统 Gerber](../鱼缸喂食控制系统_Gerber_标准版_2026-04-20.zip) |

两份 Gerber 按原始文件保留；选择打样文件时，应先用 Gerber 查看器核对所需的板层、板框、孔位与设计版本。
