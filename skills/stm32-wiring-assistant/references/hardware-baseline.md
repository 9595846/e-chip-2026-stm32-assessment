# 用户的历史硬件与接线基线

这是 2026-11-02 对话中的历史记录，不保证仍是当前接线。开始工作时核对用户指定的最新工程与实物，不要把此表当成已验证的当前状态。

| 器件 | 曾使用或计划的接法 / 已知信息 |
|---|---|
| 主控 | STM32F103C8T6 最小系统板；ST-Link 专用 SWD 接口 |
| 主控供电 | 用户曾确认由 ST-Link 3.3V 供电；全部模块的供电余量待实测 |
| OLED | 0.96 英寸四针 I²C，曾点亮；PB10 SCL、PB11 SDA，3.3V；驱动芯片未独立确认 |
| IMU | 商品标称 MPU6050；WHO_AM_I 实测 0x70，曾使用 MPU6500 兼容驱动且观察到数据随动作变化；PB6 SCL、PB7 SDA、AD0 GND，3.3V；两条信号各曾加 10kΩ 上拉 |
| 光敏模块 | VCC、GND、AO、DO；3.3V，AO PA1，DO 未使用；亮暗变化方向需实测 |
| 旋钮编码器 | A/CLK PA6，B/DT PA7，公共端 GND；若有 VCC，按实际模块确认；SW 未使用 |
| 四按键 | KEY1 PB12、KEY2 PB15、KEY3 PB8、KEY4 PB9，另一有效触点接 GND；实物标签曾需核对 |
| 蜂鸣器 | MH-FMD，VCC、I/O、GND，低电平触发；历史方案 3.3V，I/O PB14 |
| LED | LED1 PA0、LED2 PB0；每个串自己的电阻后接地；用户有 1kΩ 和 10kΩ，后者可能较暗 |
| 舵机 | SG90，信号 PA8；用户称可稳定转动；独立 5V 电源，电源地与 STM32 共地。线皮颜色相近，不能凭色彩推断电源正负 |
| USB 转串口 | RXD PA9、TXD PA10、GND 共地；3.3V 逻辑，串口模块电源脚未接 |
| 其他 | 面包板、杜邦线；5461BS-1 四位数码管；用户曾说暂时没有 74HC595 |

软件：用户称 MDK 5.34；曾实际使用 ARM Compiler 5.06 update 7 build 960、STM32F10x 标准外设库、CMSIS 和 Tera Term 5。曾有 Keil 工程 XML 带 UTF-8 BOM 导致无法读取，相关工程使用无 BOM 配置。历史拆弹箱工程为 `defuse_box/firmware/Project.uvprojx（相对于历史工作区）`；是否是用户目前维护的最新副本仍需确认。
