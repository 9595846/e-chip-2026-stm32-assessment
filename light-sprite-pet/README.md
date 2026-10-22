# 光影精灵：STM32 电子宠物

基于 **STM32F103C8T6** 的桌面电子宠物。项目包含 Keil MDK 工程、固件源码、可烧录 HEX、接线资料和现场操作说明。

## 功能

- 0.96 英寸 I²C OLED 中文界面、启动动画和多种表情。
- 电子宠物状态：喂食、玩耍、睡眠、状态显示与重置。
- 光敏模块读取环境亮度；MPU6050 读取运动数据并参与表情/尾巴反应。
- 舵机驱动尾巴，支持居中、摆动、自动反应及安全角度限制。
- 旋转编码器、四个按键、两颗 LED 和低电平触发蜂鸣器。
- USART 串口命令可在任意页面操作，也可查看传感器与 MPU6050 诊断。

> 5461BS-1 数码管因本版本没有 74HC595 驱动器而暂不接入；相应接线图仅作后续扩展参考，当前固件不驱动数码管。

## 目录

```text
Project.uvprojx       Keil uVision 工程（含 Stage1–Stage6）
Start/                STM32 启动文件与 CMSIS
Library/              STM32F10x 标准外设库
Hardware/             OLED、总线、MPU6050 软件 I²C、舵机驱动
User/                 主程序、按键/传感器、界面与宠物状态机
wiring/               引脚表和面包板/模块接线图
release/              Stage6 HEX 文件
build-evidence/       Keil Stage6 编译记录
docs/                 使用、协议和项目总结
```

## 编译与下载

1. 使用 Keil uVision 5（MDK 5.34）打开 `Project.uvprojx`。
2. 选择 `Stage6` 作为当前 Target。
3. 编译；需要下载时选择 ST-Link（SWD），连接 SWDIO、SWCLK、GND 后下载。
4. 可直接用 `release/light_sprite_stage6.hex` 烧录。该 HEX 对应 MPU6050 接 PB10/PB11 的版本。

该工程采用 ARMCC 5.06 update 7 与 STM32F10x 标准外设库。Stage6 的已有 Keil 编译记录为 0 error、0 warning；实体板结果以实际供电、接线和现场测试为准。

## 接线速览

| 开发板引脚 | 连接 |
|---|---|
| PA0 / PB0 | LED1 / LED2，各自串联限流电阻 |
| PA1 | 光敏模块 AO；模块 VCC 接 3.3V，GND 共地 |
| PA6 / PA7 | 旋转编码器 A / B；公共端接 GND |
| PA8 | 舵机信号线（通常橙色）；舵机电源正极接独立 5V |
| PA9 / PA10 | USB 转串口 RXD / TXD（交叉连接） |
| PB6 / PB7 | OLED SCL / SDA；OLED VCC 接 3.3V |
| PB10 / PB11 | MPU6050 SCL / SDA；MPU VCC 接 3.3V，AD0 接 GND |
| PB12 / PB15 / PB8 / PB9 | KEY1 / KEY2 / KEY3 / KEY4；按键另一端接 GND |
| PB14 | MH-FMD 蜂鸣器 I/O（低电平触发） |
| PA13 / PA14 | ST-Link SWDIO / SWCLK，保留用于下载调试 |

**舵机注意：**不同舵机线色不一定完全一致，先按标签/型号确认电源正负，不要只凭颜色猜。舵机正极用独立 5V，负极接该电源 GND；该 GND 还必须与 STM32 GND 相连（共地）。不要把舵机接到 STM32 3.3V 或 ST-Link 3.3V 供电。

完整图示与按钮/总线说明见 [`wiring/接线与引脚表.md`](wiring/接线与引脚表.md)。

## 按键和交互

- KEY1：喂食。
- KEY2：玩耍。
- KEY3：切换睡眠/唤醒。
- KEY4：切换显示页面。
- 旋钮：切换表情或调整界面选择；方向以当前固件设置为准。
- LED1 为状态/亮度反馈，LED2 为提醒指示；具体自动行为由传感器状态与程序状态机控制。

## 串口

终端设置：**115200 baud、8 数据位、无校验、1 停止位、无流控**；串口模块 GND 与 STM32 GND 相连，模块 VCC 不接（除非另有明确供电设计）。

常用命令：

```text
HELP
PING
STATUS
SENSOR STATUS
I2C SCAN
MPU DIAG
DIAG VIEW
DIAG EXIT
PET STATUS
PET FEED
PET PLAY
PET SLEEP
PET WAKE
TAIL CENTER
TAIL WAG
TAIL SWAY
TAIL OFF
```

发送 `HELP` 查看固件支持的完整命令。命令为大写 ASCII 文本并以回车/换行结束。MPU 诊断扫描 `0x68` 与 `0x69`，无应答时优先检查供电、共地、SCL/SDA 和杜邦线接触。

## 验收记录

Stage1–Stage6 的 Keil 构建日志已随项目提供。项目使用者反馈此前遇到的问题均已解决；这里不把代码编译记录等同于当前每一块实体板的独立复测。接线图中明确标记为“扩展/未接入”的数码管硬件不属于本版固件功能。

## 许可

应用代码为本项目内容；STM32F10x 标准外设库遵循随附的 ST 许可文件 `Library/License.doc`。使用或再发布第三方库时请一并遵守其许可条款。

