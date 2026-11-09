# 光影精灵 · STM32 电子宠物

基于 **STM32F103C8T6** 的桌面电子宠物项目，用 OLED 中文表情、光照与运动感知、舵机尾巴和按键交互，实现喂食、玩耍、睡眠与状态查看。仓库包含分阶段 Keil 工程、固件源码、Stage6 HEX、接线图和现场验收说明，便于学习、复现与演示。

[快速开始](#快速开始) · [硬件与接线](#硬件与接线) · [操作与串口](#操作与串口) · [项目结构](#项目结构) · [常见问题](#常见问题)

![光影精灵模块与接线总览](light-sprite-pet/wiring/pet_wiring_overview.svg)

> 上图为接线示意，具体连接以[接线与引脚表](light-sprite-pet/wiring/接线与引脚表.md)及当前源码为准。5461BS-1 数码管属于后续扩展，**本版固件未启用**，也不需要接入才能运行电子宠物。

## 功能概览

| 模块 | 当前实现 |
| --- | --- |
| OLED 显示 | 0.96 英寸 I²C OLED、中文界面、启动动画、多种表情及诊断页面 |
| 宠物交互 | 喂食、玩耍、睡眠 / 唤醒、状态查看与重置 |
| 环境感知 | 光敏模块 ADC 采样、MPU6050 运动数据与自动反应 |
| 尾巴动作 | 舵机居中、摆动、自动反应及角度限制 |
| 本地操作 | 四个按键、旋转编码器、两颗 LED、低电平触发蜂鸣器 |
| 串口调试 | 状态查询、手动控制、MPU 地址探测和寄存器诊断 |

## 快速开始

### 1. 获取项目

```bash
git clone https://github.com/9595846/e-chip-2026-stm32-assessment.git
cd e-chip-2026-stm32-assessment
```

主工程位于 **`light-sprite-pet/`**。先阅读[接线与引脚表](light-sprite-pet/wiring/接线与引脚表.md)，核对模块丝印、引脚和供电后再上电。

### 2. 编译与下载

1. 使用 **Keil uVision 5** 打开 [Project.uvprojx](light-sprite-pet/Project.uvprojx)。
2. 选择 **Stage6** Target，使用工程配置的 **ARMCC 5.06 update 7（build 960）** 编译器。
3. 执行 Build，确认编译成功。
4. 连接 ST-Link 的 SWDIO、SWCLK、GND，在 Keil 中选择 ST-Link / SWD 下载并复位。

项目文档记录的 IDE 为 MDK 5.34。若提示找不到 ARM Compiler 5，请先安装或配置对应编译器；工程现有配置使用 ARMCC 5。

仓库也提供 [Stage6 HEX](light-sprite-pet/release/light_sprite_stage6.hex)，对应 **MPU6050 接 PB10 / PB11** 的发布版本。修改源码或引脚后，请重新编译生成固件。

### 3. 确认运行

串口终端设置为 **115200 baud、8N1、无流控**。发送大写 ASCII 命令，每条以回车或换行结束：

```text
PING
STATUS
HELP
```

正常情况下，`PING` 返回 `PONG`；`STATUS` 可查看阶段、OLED、MPU、尾巴等状态，完整功能固件应显示 `STAGE=6`。OLED 启动后，可按 KEY4 切换表情、宠物状态与诊断页面。

## 硬件与接线

### 器件清单

- STM32F103C8T6 开发板、ST-Link 下载器。
- 0.96 英寸 I²C OLED、MPU6050、带 AO 输出的光敏模块。
- 舵机及合适的独立 5V 电源。
- 旋转编码器、四个按键、两颗 LED 及各自的限流电阻。
- MH-FMD 低电平触发蜂鸣器模块。
- USB 转 TTL 串口模块、面包板、杜邦线。

### 引脚速查

| STM32 引脚 | 连接模块 | 说明 |
| --- | --- | --- |
| PA0 / PB0 | LED1 / LED2 | 每颗 LED 单独串联限流电阻，阴极接 GND |
| PA1 | 光敏模块 AO | 模块 VCC 接 3.3V，GND 共地，DO 暂不接 |
| PA6 / PA7 | 编码器 A / B | 公共端接 GND |
| PA8 | 舵机信号 | 舵机电源使用独立 5V，并与 STM32 共地 |
| PA9 / PA10 | USB-TTL RXD / TXD | TX / RX 交叉，另接 GND |
| PB6 / PB7 | OLED SCL / SDA | 模块 VCC 接 3.3V |
| PB10 / PB11 | MPU6050 SCL / SDA | 独立软件 I²C，模块 VCC 接 3.3V，AD0 接 GND |
| PB12 / PB15 / PB8 / PB9 | KEY1 / KEY2 / KEY3 / KEY4 | 按键另一端接 GND，程序启用内部上拉 |
| PB14 | 蜂鸣器 I/O | 低电平触发，模块供电按实际型号核对 |
| PA13 / PA14 | ST-Link SWDIO / SWCLK | 保留用于下载与调试 |

### 供电要点

- **舵机使用独立 5V，电源 GND 与 STM32 GND 相连。** 不使用 STM32 或 ST-Link 的 3.3V 给舵机供电；线序按标签和型号核实。
- OLED、MPU6050 的 I²C 上拉连接到 3.3V，先检查模块是否已有上拉电阻。
- USB-TTL 通常只接 GND、TXD、RXD；VCC 不接，除非已核对整体供电方案。串口信号使用兼容 STM32 的 3.3V TTL 电平。
- 数码管图纸仅作扩展参考，不要将旧预留线接到 MPU6050 正在使用的 PB10 / PB11。

详细图示：[完整接线表](light-sprite-pet/wiring/接线与引脚表.md) · [面包板布局](light-sprite-pet/wiring/pet_breadboard_layout.svg)

## 操作与串口

### 按键操作（Stage6）

| 输入 | 操作 |
| --- | --- |
| KEY1 | 喂食 |
| KEY2 | 玩耍 |
| KEY3 | 切换睡眠 / 唤醒 |
| KEY4 | 循环切换表情、宠物状态、诊断页面 |
| 旋转编码器 | 切换表情或调整界面选择，具体行为以当前页面与固件为准 |

### 常用串口命令

下表每个代码片段均为一条独立命令；其余支持项可发送 `HELP` 查看。

| 用途 | 命令示例 |
| --- | --- |
| 连通与系统状态 | `PING`、`STATUS`、`HELP` |
| OLED 与表情 | `OLED STATUS`、`FACE NEXT`、`FACE PREV`、`FACE HAPPY` |
| 传感器 | `SENSOR STATUS`、`SENSOR AUTO ON`、`SENSOR AUTO OFF` |
| 宠物互动 | `PET STATUS`、`PET FEED`、`PET PLAY`、`PET SLEEP`、`PET WAKE` |
| 宠物页面与重置 | `PET VIEW`、`PET RESET` |
| 尾巴控制 | `TAIL CENTER`、`TAIL WAG`、`TAIL SWAY`、`TAIL OFF` |
| 尾巴自动模式 | `TAIL AUTO ON`、`TAIL AUTO OFF` |
| LED 与蜂鸣器 | `LED1 ON`、`LED1 OFF`、`LED2 ON`、`LED2 OFF`、`BUZZER TEST` |
| 诊断页面 | `DIAG VIEW`、`DIAG EXIT` |
| MPU 通信诊断 | `I2C SCAN`、`MPU DIAG` |

`I2C SCAN` 仅探测 MPU 软件总线上的 **0x68 / 0x69**，并非扫描全部 I²C 地址，也不用于检测另一条总线上的 OLED。`MPU DIAG` 会读取身份寄存器并尝试初始化传感器。

舵机手动设角命令为 `TAIL SET 90`，当前代码允许 **35°–145°**，中位为 **90°**。初测时先取下机械负载，确认行程后再安装尾巴。

## 分阶段学习

工程通过 Target 中的 `APP_STAGE` 宏选择功能，后续阶段在前面阶段的基础上增加能力。完整体验请选择 Stage6。

| Target | 主要内容 |
| --- | --- |
| Stage1 | 基础 GPIO、LED、蜂鸣器与串口命令 |
| Stage2 | OLED 界面、表情与本地输入 |
| Stage3 | 光照与运动传感器、自动反应 |
| Stage4 | 舵机尾巴控制 |
| Stage5 | 宠物状态与喂食、玩耍、睡眠交互 |
| Stage6 | 诊断页面、MPU 地址探测与诊断命令 |

## 项目结构

```text
.
├── README.md
├── light-sprite-pet/
│   ├── Project.uvprojx       # Keil 工程，含 Stage1–Stage6
│   ├── Start/               # 启动文件与 CMSIS
│   ├── Library/             # STM32F10x 标准外设库
│   ├── Hardware/            # 板级、OLED、总线、MPU 软件 I²C、舵机驱动
│   ├── User/                # 主程序、界面、传感器与宠物状态机
│   ├── wiring/              # 引脚表、模块接线与面包板示意
│   ├── release/             # Stage6 HEX
│   ├── build-evidence/      # 已保存的 Stage6 Keil 构建日志
│   └── docs/                # 串口验收与项目总结
└── skills/
    └── stm32-wiring-assistant/ # 配套 STM32 接线助手 Skill
```

源码阅读入口：[主循环与串口命令](light-sprite-pet/User/main.c) · [配置与阈值](light-sprite-pet/User/config.h) · [宠物状态机](light-sprite-pet/User/pet_game.c) · [传感器处理](light-sprite-pet/User/sensors.c)

## 验证与演示

仓库保存的 [Stage6 构建日志](light-sprite-pet/build-evidence/Keil_Stage6_Build.log) 使用 ARMCC 5.06 update 7，记录为 **0 Error、0 Warning**。这是已有构建证据；重新修改源码后仍需重新编译，编译通过也不能替代实板验收。

建议按以下顺序演示：

1. 下载 Stage6，复位观察 OLED 启动画面。
2. 按 KEY1 / KEY2 / KEY3 演示喂食、玩耍、睡眠，按 KEY4 切换页面。
3. 遮挡与照亮光敏模块，通过 `SENSOR STATUS` 对比读数。
4. 发送 `I2C SCAN`、`MPU DIAG`，检查 MPU 通信，再观察运动响应。
5. 发送 `TAIL CENTER`、`TAIL WAG`、`TAIL SWAY` 检查尾巴。
6. 使用 LED 和蜂鸣器命令验证输出。

完整流程见[串口与现场验收](light-sprite-pet/docs/串口与现场验收.md)。光照与运动阈值、舵机机械行程和长时间运行效果需结合实际装配验证；实物照片、演示视频与逐项实测记录可后续补充。

## 常见问题

| 现象 | 优先检查 |
| --- | --- |
| Keil 提示缺少编译器 | 配置 ARMCC 5.06 update 7，确认选中 Stage6 Target |
| 串口没有回复或乱码 | 115200 / 8N1、共地、TX / RX 交叉、命令大写且带行结束符 |
| 返回未知命令 | 发送 `HELP`，确认当前下载的是所需阶段固件 |
| OLED 不亮 | 3.3V 供电、PB6 / PB7 线序、模块地址与 `OLED STATUS` |
| OLED 正常但 MPU 无应答 | MPU 使用 PB10 / PB11；检查供电、共地、AD0 与接触，运行 `MPU DIAG` |
| 舵机抖动或板子复位 | 独立 5V 电源能力、共地与机械卡阻 |
| 数码管不亮 | 当前固件未驱动数码管，不属于本版运行故障 |

## 相关资料

- [子项目完整说明](light-sprite-pet/README.md)
- [接线与引脚表](light-sprite-pet/wiring/接线与引脚表.md)
- [串口与现场验收](light-sprite-pet/docs/串口与现场验收.md)
- [项目总结](light-sprite-pet/docs/项目总结.md)
- [配套 STM32 接线助手 Skill 说明](skills/stm32-wiring-assistant/README.md)

## 许可说明

STM32F10x 标准外设库遵循随附的 [ST 许可文件](light-sprite-pet/Library/License.doc)。仓库当前未为项目应用代码单独设置开源许可证。
