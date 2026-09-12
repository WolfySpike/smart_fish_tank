# 第三方组件与许可说明

仓库根目录的 [MIT 许可证](LICENSE)适用于项目贡献者编写的原创代码和说明文档。第三方代码、生成文件中的原有代码及外部资料继续适用原作者声明；保留原文件中的版权信息。

## 固件组件

| 组件 | 仓库位置 | 来源与许可依据 |
| --- | --- | --- |
| STM32F1 HAL | [`smart_system/Drivers/STM32F1xx_HAL_Driver/`](smart_system/Drivers/STM32F1xx_HAL_Driver/) | STMicroelectronics；随附 [LICENSE.txt](smart_system/Drivers/STM32F1xx_HAL_Driver/LICENSE.txt) 指定优先适用软件包许可，缺少适用软件包许可时使用 BSD-3-Clause |
| CMSIS | [`smart_system/Drivers/CMSIS/`](smart_system/Drivers/CMSIS/) | Arm 及各文件标明的贡献者；随附 [Apache-2.0 许可正文](smart_system/Drivers/CMSIS/LICENSE.txt)，具体文件同时保留各自声明 |
| STM32F1 CMSIS Device | [`smart_system/Drivers/CMSIS/Device/ST/STM32F1xx/`](smart_system/Drivers/CMSIS/Device/ST/STM32F1xx/) | STMicroelectronics；随附 [LICENSE.txt](smart_system/Drivers/CMSIS/Device/ST/STM32F1xx/LICENSE.txt) 指定优先适用软件包许可，缺少适用软件包许可时使用 Apache-2.0 |
| CubeMX 生成代码、系统与启动文件 | [`smart_system/Core/`](smart_system/Core/)、[`startup_stm32f103xb.s`](smart_system/MDK-ARM/startup_stm32f103xb.s) | 保留 STMicroelectronics 的文件头及适用组件许可；根目录 MIT 不替代其原有声明 |
| OLED 驱动及字模 | [`smart_system/Hardware/OLED.c`](smart_system/Hardware/OLED.c)、`OLED.h`、`OLED_Data.c`、`OLED_Data.h` | 源文件注明江协科技版权所有，并附使用声明；沿用该声明，不将原实现重新许可为 MIT |

## OLED 驱动来源

[`OLED.c`](smart_system/Hardware/OLED.c) 文件头记录原始驱动版本为 V2.0，发布时间为 2024-10-20，并声明：

> 本程序由江协科技创建并免费开源共享
>
> 你可以任意查看、使用和修改，并应用到自己的项目之中
>
> 程序版权归江协科技所有，任何人或组织不得将其据为己有

原文件提供的介绍地址：[江协科技 OLED 教程](https://jiangxiekeji.com/tutorial/oled.html)。本项目在该驱动基础上进行 HAL 引脚适配、显示适配等修改，保留原作者信息。上述说明并不将作者自述的共享条款等同于 MIT 或其他标准开源许可证。

## 硬件与课程资料

原理图、PCB、Gerber 和课程题目作为项目资料随仓库提供。课程题目原文、EDA 工程中的外部封装／图形等资源不自动纳入根目录 MIT 的授权范围，应以其原始许可为准。仓库未另行为硬件设计声明专用硬件许可证。

README 中的 PCB 预览裁剪自仓库内现有截图，未用于表示实物测试结果。
