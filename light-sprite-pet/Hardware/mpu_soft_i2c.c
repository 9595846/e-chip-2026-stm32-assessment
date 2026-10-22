#include "mpu_soft_i2c.h"
#include "config.h"
#include "stm32f10x.h"

/* MPU6050-only software I2C: SCL=PB10, SDA=PB11. OLED stays on I2C1 PB6/PB7. */
#if APP_STAGE >= 3

#define MPU_GPIO     GPIOB
#define MPU_SCL_PIN GPIO_Pin_10
#define MPU_SDA_PIN GPIO_Pin_11

static void short_delay(void)
{
    volatile unsigned i;
    for (i = 0u; i < 70u; i++) { __NOP(); }
}

static void scl(uint8_t high)
{
    if (high) { GPIO_SetBits(MPU_GPIO, MPU_SCL_PIN); }
    else { GPIO_ResetBits(MPU_GPIO, MPU_SCL_PIN); }
    short_delay();
}

static void sda(uint8_t high)
{
    if (high) { GPIO_SetBits(MPU_GPIO, MPU_SDA_PIN); }
    else { GPIO_ResetBits(MPU_GPIO, MPU_SDA_PIN); }
    short_delay();
}

static uint8_t sda_read(void)
{
    return GPIO_ReadInputDataBit(MPU_GPIO, MPU_SDA_PIN) ? 1u : 0u;
}

static void start(void)
{
    sda(1u); scl(1u); sda(0u); scl(0u);
}

static void stop(void)
{
    sda(0u); scl(1u); sda(1u);
}

static int write_byte(uint8_t value)
{
    unsigned bit;
    for (bit = 0u; bit < 8u; bit++) {
        sda((value & 0x80u) ? 1u : 0u);
        scl(1u);
        scl(0u);
        value <<= 1;
    }
    sda(1u);
    scl(1u);
    bit = sda_read(); /* 0 is acknowledge. */
    scl(0u);
    return bit == 0u;
}

static uint8_t read_byte(uint8_t acknowledge)
{
    unsigned bit;
    uint8_t value = 0u;
    sda(1u);
    for (bit = 0u; bit < 8u; bit++) {
        scl(1u);
        value = (uint8_t)((value << 1) | sda_read());
        scl(0u);
    }
    sda(acknowledge ? 0u : 1u);
    scl(1u);
    scl(0u);
    sda(1u);
    return value;
}

void mpu_bus_init(void)
{
    GPIO_InitTypeDef gpio;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    gpio.GPIO_Pin = MPU_SCL_PIN | MPU_SDA_PIN;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    gpio.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_Init(MPU_GPIO, &gpio);
    GPIO_SetBits(MPU_GPIO, MPU_SCL_PIN | MPU_SDA_PIN);
}

int mpu_bus_probe(uint8_t address)
{
    int acknowledged;
    start();
    acknowledged = write_byte((uint8_t)(address << 1));
    stop();
    return acknowledged;
}

uint8_t mpu_bus_scl_level(void)
{
    return GPIO_ReadInputDataBit(MPU_GPIO, MPU_SCL_PIN) ? 1u : 0u;
}

uint8_t mpu_bus_sda_level(void)
{
    return GPIO_ReadInputDataBit(MPU_GPIO, MPU_SDA_PIN) ? 1u : 0u;
}

int mpu_bus_write(uint8_t address, const uint8_t *data, unsigned length)
{
    unsigned i;
    if (data == 0 || length == 0u) { return 0; }
    start();
    if (!write_byte((uint8_t)(address << 1))) { stop(); return 0; }
    for (i = 0u; i < length; i++) {
        if (!write_byte(data[i])) { stop(); return 0; }
    }
    stop();
    return 1;
}

int mpu_bus_read_register_byte(uint8_t address, uint8_t reg, uint8_t *value)
{
    if (value == 0) { return 0; }
    start();
    if (!write_byte((uint8_t)(address << 1))) { stop(); return 0; }
    if (!write_byte(reg)) { stop(); return 0; }
    start();
    if (!write_byte((uint8_t)((address << 1) | 1u))) { stop(); return 0; }
    *value = read_byte(0u); /* Last and only byte: send NACK. */
    stop();
    return 1;
}

#else
void mpu_bus_init(void) { }
int mpu_bus_probe(uint8_t address) { (void)address; return 0; }
int mpu_bus_write(uint8_t address, const uint8_t *data, unsigned length)
{ (void)address; (void)data; (void)length; return 0; }
int mpu_bus_read_register_byte(uint8_t address, uint8_t reg, uint8_t *value)
{ (void)address; (void)reg; (void)value; return 0; }
uint8_t mpu_bus_scl_level(void) { return 0u; }
uint8_t mpu_bus_sda_level(void) { return 0u; }
#endif

