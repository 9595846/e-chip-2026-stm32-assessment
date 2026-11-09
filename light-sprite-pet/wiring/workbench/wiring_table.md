# 逐根接线表

由接线工作台同一份 JSON 导出。相同网络名表示电气相连，电源不是必须全挤在开发板一个插针上，可用电源分配排。

|编号|起点|终点|网络/用途|说明|
|---|---|---|---|---|
|w1|STM32F103C8T6 / PA0|R1 · 限流电阻 / IN|LED1|GPIO → 电阻 → LED 阳极|
|w2|R1 · 限流电阻 / OUT|LED1 / A|LED1_A|LED1_A|
|w3|LED1 / K|网络 GND|GND|同名网络相连；GND|
|w4|STM32F103C8T6 / PB0|R2 · 限流电阻 / IN|LED2|GPIO → 电阻 → LED 阳极|
|w5|R2 · 限流电阻 / OUT|LED2 / A|LED2_A|LED2_A|
|w6|LED2 / K|网络 GND|GND|同名网络相连；GND|
|w7|STM32F103C8T6 / 3.3V|网络 +3V3|+3V3|同名网络相连；+3V3|
|w8|STM32F103C8T6 / GND|网络 GND|GND|同名网络相连；GND|
|w9|0.96 OLED · I²C / VCC|网络 +3V3|+3V3|同名网络相连；+3V3|
|w10|0.96 OLED · I²C / GND|网络 GND|GND|同名网络相连；GND|
|w11|MPU 模块 · 型号待核实 / VCC|网络 +3V3|+3V3|同名网络相连；+3V3|
|w12|MPU 模块 · 型号待核实 / GND|网络 GND|GND|同名网络相连；GND|
|w13|MPU 模块 · 型号待核实 / AD0|网络 GND|GND|同名网络相连；GND|
|w14|光敏模块 AO / DO / VCC|网络 +3V3|+3V3|同名网络相连；+3V3|
|w15|光敏模块 AO / DO / GND|网络 GND|GND|同名网络相连；GND|
|w16|旋钮编码器 · A/B / COM|网络 GND|GND|同名网络相连；GND|
|w17|3.3V 逻辑电源 / +|网络 +3V3|+3V3|同名网络相连；+3V3|
|w18|3.3V 逻辑电源 / -|网络 GND|GND|同名网络相连；GND|
|w19|MH-FMD 蜂鸣器 / GND|网络 GND|GND|同名网络相连；GND|
|w20|尾巴舵机 · 型号待核对 / +|网络 +5V_SERVO|+5V_SERVO|同名网络相连；+5V_SERVO|
|w21|尾巴舵机 · 型号待核对 / -|网络 GND|GND|同名网络相连；GND|
|w22|舵机独立 5V 电源 / +|网络 +5V_SERVO|+5V_SERVO|同名网络相连；+5V_SERVO|
|w23|舵机独立 5V 电源 / -|网络 GND|GND|同名网络相连；GND|
|w24|USB 转 TTL 串口 / GND|网络 GND|GND|同名网络相连；GND|
|w25|ST-Link · SWD / GND|网络 GND|GND|同名网络相连；GND|
|w26|STM32F103C8T6 / PB6|0.96 OLED · I²C / SCL|OLED_SCL|OLED_SCL|
|w27|STM32F103C8T6 / PB7|0.96 OLED · I²C / SDA|OLED_SDA|OLED_SDA|
|w28|STM32F103C8T6 / PB10|MPU 模块 · 型号待核实 / SCL|MPU_SCL|MPU_SCL|
|w29|STM32F103C8T6 / PB11|MPU 模块 · 型号待核实 / SDA|MPU_SDA|MPU_SDA|
|w30|STM32F103C8T6 / PA1|光敏模块 AO / DO / AO|LIGHT_AO|LIGHT_AO|
|w31|STM32F103C8T6 / PA6|旋钮编码器 · A/B / A|ENC_A|ENC_A|
|w32|STM32F103C8T6 / PA7|旋钮编码器 · A/B / B|ENC_B|ENC_B|
|w33|STM32F103C8T6 / PB14|MH-FMD 蜂鸣器 / I/O|BUZZER_LOW|BUZZER_LOW|
|w34|STM32F103C8T6 / PA8|尾巴舵机 · 型号待核对 / SIG|SERVO_PWM|SERVO_PWM|
|w35|STM32F103C8T6 / PA9|USB 转 TTL 串口 / RXD|UART_TX|UART_TX|
|w36|STM32F103C8T6 / PA10|USB 转 TTL 串口 / TXD|UART_RX|UART_RX|
|w37|STM32F103C8T6 / PA13|ST-Link · SWD / SWDIO|SWDIO|SWDIO|
|w38|STM32F103C8T6 / PA14|ST-Link · SWD / SWCLK|SWCLK|SWCLK|
|w39|KEY1 · PB12 / 触点2|网络 GND|GND|同名网络相连；GND|
|w40|STM32F103C8T6 / PB12|KEY1 · PB12 / 触点1|KEY1|PB12 内部上拉，按下接地|
|w41|KEY2 · PB15 / 触点2|网络 GND|GND|同名网络相连；GND|
|w42|STM32F103C8T6 / PB15|KEY2 · PB15 / 触点1|KEY2|PB15 内部上拉，按下接地|
|w43|KEY3 · PB8 / 触点2|网络 GND|GND|同名网络相连；GND|
|w44|STM32F103C8T6 / PB8|KEY3 · PB8 / 触点1|KEY3|PB8 内部上拉，按下接地|
|w45|KEY4 · PB9 / 触点2|网络 GND|GND|同名网络相连；GND|
|w46|STM32F103C8T6 / PB9|KEY4 · PB9 / 触点1|KEY4|PB9 内部上拉，按下接地|

## 未接针脚

|模块|针脚|处理|
|---|---|---|
|MPU 模块 · 型号待核实|INT|当前设计不接|
|MPU 模块 · 型号待核实|XDA|当前设计不接|
|MPU 模块 · 型号待核实|XCL|当前设计不接|
|光敏模块 AO / DO|DO|当前设计不接|
|MH-FMD 蜂鸣器|VCC|必接供电尚待确认，暂不连接|
|USB 转 TTL 串口|VCC|当前设计不接|
|ST-Link · SWD|VTref?|当前设计不接|
|ST-Link · SWD|NRST?|当前设计不接|
