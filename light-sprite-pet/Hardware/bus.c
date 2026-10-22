#include "stm32f10x.h"
#include "board.h"
#include "config.h"

static uint32_t g_transaction_start;

static void i2c_configure(void)
{
    I2C_InitTypeDef i2c;
    I2C_StructInit(&i2c);
    i2c.I2C_ClockSpeed = I2C_SPEED_HZ;
    i2c.I2C_Ack = I2C_Ack_Enable;
    I2C_Init(I2C1, &i2c);
    I2C_Cmd(I2C1, ENABLE);
}

static void short_gap(void)
{
    volatile unsigned i;
    for (i = 0u; i < 80u; i++) {
        __NOP();
    }
}

void bus_init(void)
{
    GPIO_InitTypeDef gpio;
    unsigned pulse;

    I2C_Cmd(I2C1, DISABLE);
    gpio.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    gpio.GPIO_Mode = GPIO_Mode_Out_OD;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOB, &gpio);
    GPIO_SetBits(GPIOB, GPIO_Pin_6 | GPIO_Pin_7);

    /* Release a slave that was interrupted while holding SDA low. */
    for (pulse = 0u; pulse < 9u && !GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7); pulse++) {
        GPIO_ResetBits(GPIOB, GPIO_Pin_6);
        short_gap();
        GPIO_SetBits(GPIOB, GPIO_Pin_6);
        short_gap();
    }
    GPIO_ResetBits(GPIOB, GPIO_Pin_7);
    short_gap();
    GPIO_SetBits(GPIOB, GPIO_Pin_6);
    short_gap();
    GPIO_SetBits(GPIOB, GPIO_Pin_7);

    gpio.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_Init(GPIOB, &gpio);
    I2C_SoftwareResetCmd(I2C1, ENABLE);
    I2C_SoftwareResetCmd(I2C1, DISABLE);
    i2c_configure();
}

static int wait_sr1(uint16_t flag)
{
    unsigned budget = 150000u;
    while ((I2C1->SR1 & flag) == 0u) {
        if ((I2C1->SR1 & (I2C_SR1_AF | I2C_SR1_BERR | I2C_SR1_ARLO | I2C_SR1_OVR)) != 0u) {
            return 0;
        }
        if ((uint32_t)(board_millis() - g_transaction_start) >= 4u || --budget == 0u) {
            return 0;
        }
    }
    return 1;
}

static void clear_address_flag(void)
{
    volatile uint32_t value = I2C1->SR1;
    value = I2C1->SR2;
    (void)value;
}

static int fail_and_recover(void)
{
    I2C_GenerateSTOP(I2C1, ENABLE);
    bus_init();
    return 0;
}

int bus_write(uint8_t address, const uint8_t *data, unsigned length)
{
    unsigned i;
    unsigned budget = 150000u;

    if (data == 0 || length == 0u) {
        return 0;
    }
    g_transaction_start = board_millis();
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) != RESET) {
        if ((uint32_t)(board_millis() - g_transaction_start) >= 4u || --budget == 0u) {
            return fail_and_recover();
        }
    }
    I2C_GenerateSTART(I2C1, ENABLE);
    if (!wait_sr1(I2C_SR1_SB)) {
        return fail_and_recover();
    }
    I2C_Send7bitAddress(I2C1, (uint8_t)(address << 1), I2C_Direction_Transmitter);
    if (!wait_sr1(I2C_SR1_ADDR)) {
        return fail_and_recover();
    }
    clear_address_flag();
    for (i = 0u; i < length; i++) {
        if (!wait_sr1(I2C_SR1_TXE)) {
            return fail_and_recover();
        }
        I2C_SendData(I2C1, data[i]);
    }
    if (!wait_sr1(I2C_SR1_BTF)) {
        return fail_and_recover();
    }
    I2C_GenerateSTOP(I2C1, ENABLE);
    return 1;
}

static int bus_receive_one(uint8_t address, uint8_t *value)
{
    unsigned budget = 150000u;
    g_transaction_start = board_millis();
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) != RESET) {
        if ((uint32_t)(board_millis() - g_transaction_start) >= 4u || --budget == 0u) {
            return fail_and_recover();
        }
    }
    I2C_GenerateSTART(I2C1, ENABLE);
    if (!wait_sr1(I2C_SR1_SB)) { return fail_and_recover(); }
    I2C_Send7bitAddress(I2C1, (uint8_t)(address << 1), I2C_Direction_Receiver);
    if (!wait_sr1(I2C_SR1_ADDR)) { return fail_and_recover(); }
    I2C_AcknowledgeConfig(I2C1, DISABLE);
    clear_address_flag();
    I2C_GenerateSTOP(I2C1, ENABLE);
    if (!wait_sr1(I2C_SR1_RXNE)) {
        I2C_AcknowledgeConfig(I2C1, ENABLE);
        return fail_and_recover();
    }
    *value = I2C_ReceiveData(I2C1);
    I2C_AcknowledgeConfig(I2C1, ENABLE);
    return 1;
}

int bus_read_register_byte(uint8_t address, uint8_t reg, uint8_t *value)
{
    if (value == 0) { return 0; }
    if (!bus_write(address, &reg, 1u)) { return 0; }
    return bus_receive_one(address, value);
}

