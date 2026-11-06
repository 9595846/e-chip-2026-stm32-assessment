# 传感器主机回归测试

在 `light-sprite-pet` 目录，用 GCC 编译真实 `User/sensors.c`，通过模拟板级 ADC 和 MPU 总线验证状态逻辑：

```sh
gcc -std=c99 -Wall -Wextra -Werror -DAPP_STAGE=6 -IUser -IHardware User/sensors.c tests/sensors_test.c -o sensors_test
./sensors_test
```

Windows 上执行 `./sensors_test.exe`。二进制可输出到系统临时目录。

覆盖正负极值间的差值计算、12 个字节位置的读取失败、失败时整帧保留，以及重连后重新建立运动基准。总线信号时序、寄存器帧同步、真实运动阈值与供电行为仍需实板验证。

修改了传感器源码后，旧 `release/light_sprite_stage6.hex` 和已有构建日志不会自动更新，需在 Keil 中重新编译。
