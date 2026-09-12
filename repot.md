# 自动喂食照明系统硬件移植记录

> 本文保留硬件移植过程中的记录，水位阈值已按当前宏定义更新。完整接线与复现步骤见 [README](README.md)、[硬件说明](docs/HARDWARE.md)和[编译与使用指南](docs/BUILD_AND_USAGE.md)。

## 1. 项目概述
- 主控：STM32F103（HAL 工程，CubeMX 生成）
- 功能：DS1302 实时时钟、OLED 显示、定时喂食（SG90）、鱼缸灯与补光灯定时控制、水位检测与 R/Y/G 指示、参数掉电保存到 Flash
- 灯光实物：白色 LED 共 2 盏（鱼缸灯 1 盏 + 补光灯 1 盏）
- 代码结构：`User/` 下模块化（`app_controller/app_menu/app_schedule/app_device/app_storage/app_time`）

## 2. 当前外设与引脚总表（按工程实际）

| 功能 | 外设/模式 | 引脚 | 备注 |
|---|---|---|---|
| OLED 软件 I2C SCL | GPIO OD 输出 | PB8 | `OLED.c` 中 bit-bang |
| OLED 软件 I2C SDA | GPIO OD 输出 | PB9 | `OLED.c` 中 bit-bang |
| DS1302 RST/CE | GPIO 输出 | PB12 | `main.h` 宏定义 |
| DS1302 CLK | GPIO 输出 | PB13 | `main.h` 宏定义 |
| DS1302 DAT/IO | GPIO 输出/输入切换 | PB14 | `ds1302.h` 默认宏 |
| SG90 信号 | TIM1_CH1 PWM | PA8 | 50Hz 舵机 PWM |
| R 指示灯 | TIM3_CH1 PWM | PA6 | 水位颜色指示 |
| Y 指示灯 | TIM3_CH2 PWM | PA7 | 水位颜色指示 |
| G 指示灯 | TIM3_CH3 PWM | PB0 | 水位颜色指示 |
| 鱼缸灯（白灯1，PWM） | TIM3_CH4 PWM | PB1 | `App_SetFishLight()`，当前开启时占空比由宏固定 |
| 植物补光灯（白灯2，开关） | GPIO 输出 | PB6 | `FILL_LIGHT_Pin` |
| 水位传感器模拟量 S | ADC1_IN5 | PA5 | 12bit ADC |
| KEY0 | GPIO 输入上拉 | PA0 | 低电平按下 |
| KEY1 | GPIO 输入上拉 | PA2 | 低电平按下 |
| KEY2 | GPIO 输入上拉 | PA4 | 低电平按下 |
| KEY3 | GPIO 输入上拉 | PB11 | 低电平按下 |
| KEY4 | GPIO 输入上拉 | PC14 | 低电平按下 |

## 3. 关键定时器与 ADC 参数
- TIM1（SG90）：Prescaler=71，Period=19999，PWM=50Hz，初始 Pulse=1500us（中位）
- TIM3（灯光）：Prescaler=71，Period=999，PWM 基频约 1kHz
- ADC1：Channel=ADC_CHANNEL_5（PA5），12bit，采样时间 239.5cycles，软件触发

## 4. 接线建议（面包板到 PCB 保持一致）

### 4.1 供电
- STM32：5V USB 或板载稳压供电
- SG90：建议独立 5V 供电（电流裕量 >= 1A）
- DS1302/OLED/水位传感器：按模块额定供电（目前工程按 3.3V 逻辑）
- 所有模块必须共地（GND 共地）

### 4.2 模块接线
- OLED：`VCC->3.3V`，`GND->GND`，`SCL->PB8`，`SDA->PB9`
- DS1302：`VCC->3.3V`，`GND->GND`，`RST->PB12`，`CLK->PB13`，`DAT->PB14`
- SG90：`SIG->PA8`，`VCC->外部5V`，`GND->系统GND`
- 水位传感器（模拟型）：`+->3.3V`，`-->GND`，`S->PA5`
- R/Y/G 指示：`R->PA6`，`Y->PA7`，`G->PB0`（串限流电阻）
- 鱼缸白灯（LED1）驱动：`PB1(TIM3_CH4)`（建议串 220~1kΩ 限流）
- 植物补光白灯（LED2）驱动：`PB6(GPIO)`（建议串 220~1kΩ 限流）
- 按键：一端接引脚（PA0/PA2/PA4/PB11/PC14），一端接 GND（内部上拉）

## 5. 软件控制逻辑（与硬件相关）
- K0：进入/切换菜单页面，最后一页退出并保存参数到 Flash
- K1/K2：参数加减（支持长按连发）
- KEY3/KEY4：字段前后切换
- 非菜单主页面：短按 K1 可手动喂食（OLED 最底部提示 `Press K1 to feed`）
- 时间调度：
  - 鱼缸灯按 `fishOn/fishOff` 时间窗控制
  - 补光灯按 `fillOn/fillOff` 时间窗控制
  - 喂食支持每天 1~3 次，每次到点执行 `feedCycles` 次舵机摆动
- 掉电保存：配置写入芯片最后 1 页 Flash（运行时自动按芯片容量计算地址）

## 6. 水位阈值（当前代码）
- 采样值 `<1200`：DRY（红）
- `1200~1799`：LOW（绿）
- `1800~1999`：MID（黄）
- `>=2000`：HIGH（红）
- OLED 百分比映射区间：1200~2000 -> 0~100%
- 以上采用平滑后的判断值，阈值以 `User/app_internal.h` 中的宏定义为准。

## 7. RTC 校时开关（开发调试）
- 文件：`User/app_internal.h`
- 宏：`APP_RTC_SYNC_FROM_BUILD_AT_BOOT`
- 用法：
  - 设为 `1U`：每次上电把 DS1302 同步到编译时间
  - 设为 `0U`：仅在停振/读到非法时间时自动回写
- 建议：校时时短暂设 `1U`，重新编译烧录并启动一次；随后改回 `0U`，再次编译烧录。

## 8. PCB 迁移注意事项（重点）
- SG90 电源与 MCU 电源分区，地线单点汇接，避免舵机冲击导致复位/花屏
- 为 SG90 与 LED 负载预留驱动级（MOSFET/三极管）与续流/抑制措施，不要直接超流驱动 MCU 引脚
- OLED 与 DS1302 走线尽量短，靠近主控，预留测试点（PB8/PB9/PB12/PB13/PB14）
- ADC（PA5）旁路与滤波：建议传感器端加 0.1uF，必要时串 1k 保护
- 每个模块接口预留 `VCC/GND/SIG` 丝印，按键接口统一方向，便于装配
- 预留 SWD 下载口（SWDIO/SWCLK/3V3/GND/NRST）

## 9. 上板联调清单
1. 先只上电 STM32，确认 3.3V 稳定
2. 接 OLED，确认主页面显示
3. 接 DS1302，确认时间可读、星期正确
4. 接按键，确认菜单可切页/改值/保存
5. 接 SG90（外部5V），确认手动喂食动作
6. 接水位传感器，确认 ADC 变化与红黄绿切换
7. 接鱼缸灯/补光灯负载，确认定时开关逻辑
