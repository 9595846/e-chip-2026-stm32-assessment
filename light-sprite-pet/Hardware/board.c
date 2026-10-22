#include "stm32f10x.h"
#include "board.h"
#include "config.h"
#include <string.h>

#define UART_RX_SIZE 128u
#define UART_TX_SIZE 512u

static volatile uint32_t g_millis;
static volatile uint16_t g_rx_in;
static volatile uint16_t g_rx_out;
static volatile uint16_t g_tx_in;
static volatile uint16_t g_tx_out;
static volatile uint8_t g_rx_overflow;
static uint8_t g_rx_data[UART_RX_SIZE];
static uint8_t g_tx_data[UART_TX_SIZE];
static uint8_t g_led1_on;
static uint8_t g_led2_on;
static uint8_t g_buzzer_on;
static uint16_t g_last_encoder;

void SysTick_Handler(void)
{
    g_millis++;
}

uint32_t board_millis(void)
{
    return g_millis;
}

static void gpio_output(GPIO_TypeDef *port, uint16_t pins)
{
    GPIO_InitTypeDef gpio;
    gpio.GPIO_Pin = pins;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(port, &gpio);
}

void board_led1_set(uint8_t on)
{
    g_led1_on = on ? 1u : 0u;
    GPIO_WriteBit(LED1_GPIO, LED1_PIN, on ? Bit_SET : Bit_RESET);
}

void board_led2_set(uint8_t on)
{
    g_led2_on = on ? 1u : 0u;
    GPIO_WriteBit(LED2_GPIO, LED2_PIN, on ? Bit_SET : Bit_RESET);
}

uint8_t board_led1_get(void)
{
    return g_led1_on;
}

uint8_t board_led2_get(void)
{
    return g_led2_on;
}

void board_buzzer_set(uint8_t on)
{
    g_buzzer_on = on ? 1u : 0u;
#if BUZZER_ACTIVE_LOW
    GPIO_WriteBit(BUZZER_GPIO, BUZZER_PIN, on ? Bit_RESET : Bit_SET);
#else
    GPIO_WriteBit(BUZZER_GPIO, BUZZER_PIN, on ? Bit_SET : Bit_RESET);
#endif
}

uint8_t board_buzzer_get(void)
{
    return g_buzzer_on;
}

uint8_t board_keys_raw(void)
{
    uint16_t value = GPIO_ReadInputData(GPIOB);
    uint8_t keys = 0u;
    if ((value & GPIO_Pin_12) == 0u) { keys |= 0x01u; }
    if ((value & GPIO_Pin_15) == 0u) { keys |= 0x02u; }
    if ((value & GPIO_Pin_8) == 0u)  { keys |= 0x04u; }
    if ((value & GPIO_Pin_9) == 0u)  { keys |= 0x08u; }
    return keys;
}

int16_t board_encoder_delta(void)
{
    uint16_t current = TIM_GetCounter(TIM3);
    int16_t delta = (int16_t)(current - g_last_encoder);
    g_last_encoder = current;
    return delta;
}

uint16_t board_light_raw(void)
{
#if APP_STAGE >= 3
    return (uint16_t)ADC1->DR;
#else
    return 0u;
#endif
}

void USART1_IRQHandler(void)
{
    uint16_t status = (uint16_t)USART1->SR;

    if ((status & (USART_SR_RXNE | USART_SR_ORE | USART_SR_FE | USART_SR_NE)) != 0u) {
        uint8_t value = (uint8_t)USART_ReceiveData(USART1);
        uint16_t next = (uint16_t)((g_rx_in + 1u) % UART_RX_SIZE);

        if ((status & (USART_SR_ORE | USART_SR_FE | USART_SR_NE)) != 0u || next == g_rx_out) {
            g_rx_overflow = 1u;
        } else {
            g_rx_data[g_rx_in] = value;
            g_rx_in = next;
        }
    }

    if (USART_GetITStatus(USART1, USART_IT_TXE) != RESET) {
        if (g_tx_out == g_tx_in) {
            USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
        } else {
            USART_SendData(USART1, g_tx_data[g_tx_out]);
            g_tx_out = (uint16_t)((g_tx_out + 1u) % UART_TX_SIZE);
        }
    }
}

int uart_read_byte(uint8_t *value)
{
    if (value == 0 || g_rx_out == g_rx_in) {
        return 0;
    }
    *value = g_rx_data[g_rx_out];
    g_rx_out = (uint16_t)((g_rx_out + 1u) % UART_RX_SIZE);
    return 1;
}

int uart_rx_overflow(void)
{
    uint32_t mask = __get_PRIMASK();
    int result;
    __disable_irq();
    result = g_rx_overflow ? 1 : 0;
    g_rx_overflow = 0u;
    __set_PRIMASK(mask);
    return result;
}

int uart_send(const char *text)
{
    size_t i;
    size_t length;
    uint16_t free_count;
    uint32_t mask;

    if (text == 0) {
        return 0;
    }
    length = strlen(text);
    mask = __get_PRIMASK();
    __disable_irq();
    free_count = (uint16_t)((g_tx_out + UART_TX_SIZE - g_tx_in - 1u) % UART_TX_SIZE);
    if (length > free_count) {
        __set_PRIMASK(mask);
        return 0;
    }
    for (i = 0u; i < length; i++) {
        g_tx_data[g_tx_in] = (uint8_t)text[i];
        g_tx_in = (uint16_t)((g_tx_in + 1u) % UART_TX_SIZE);
    }
    USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
    __set_PRIMASK(mask);
    return 1;
}

void board_init(void)
{
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef usart;
    NVIC_InitTypeDef nvic;
#if APP_STAGE >= 2
    TIM_TimeBaseInitTypeDef timer;
    TIM_ICInitTypeDef input_capture;
#endif

    RCC_APB2PeriphClockCmd(
        RCC_APB2Periph_AFIO |
        RCC_APB2Periph_GPIOA |
        RCC_APB2Periph_GPIOB |
        RCC_APB2Periph_USART1,
        ENABLE);
#if APP_STAGE >= 2
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3 | RCC_APB1Periph_I2C1, ENABLE);
#endif
#if APP_STAGE >= 3
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);
#endif

    /* Establish safe output levels before changing the pin modes. */
    GPIO_ResetBits(GPIOA, LED1_PIN);
    GPIO_ResetBits(GPIOB, LED2_PIN);
    GPIO_SetBits(GPIOB, BUZZER_PIN); /* MH-FMD silence level. */
    gpio_output(GPIOA, LED1_PIN);
    gpio_output(GPIOB, LED2_PIN | BUZZER_PIN);
    board_led1_set(0u);
    board_led2_set(0u);
    board_buzzer_set(0u);

    gpio.GPIO_Pin = GPIO_Pin_9;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &gpio);
    gpio.GPIO_Pin = GPIO_Pin_10;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);

    USART_StructInit(&usart);
    usart.USART_BaudRate = UART_BAUDRATE;
    usart.USART_WordLength = USART_WordLength_8b;
    usart.USART_StopBits = USART_StopBits_1;
    usart.USART_Parity = USART_Parity_No;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &usart);

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    nvic.NVIC_IRQChannel = USART1_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 1u;
    nvic.NVIC_IRQChannelSubPriority = 0u;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART1, ENABLE);

    (void)SysTick_Config(CPU_HZ / 1000u);
    NVIC_SetPriority(SysTick_IRQn, 0u);

#if APP_STAGE >= 2
    gpio.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_12 | GPIO_Pin_15;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOB, &gpio);

    gpio.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &gpio);

    TIM_TimeBaseStructInit(&timer);
    timer.TIM_Prescaler = 0u;
    timer.TIM_Period = 65535u;
    TIM_TimeBaseInit(TIM3, &timer);
    TIM_EncoderInterfaceConfig(
        TIM3,
        TIM_EncoderMode_TI12,
        TIM_ICPolarity_Rising,
        TIM_ICPolarity_Rising);
    TIM_ICStructInit(&input_capture);
    input_capture.TIM_ICFilter = 10u;
    input_capture.TIM_Channel = TIM_Channel_1;
    TIM_ICInit(TIM3, &input_capture);
    input_capture.TIM_Channel = TIM_Channel_2;
    TIM_ICInit(TIM3, &input_capture);
    TIM_Cmd(TIM3, ENABLE);
    g_last_encoder = TIM_GetCounter(TIM3);

    bus_init();
#endif

#if APP_STAGE >= 3
    gpio.GPIO_Pin = GPIO_Pin_1;
    gpio.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &gpio);
    /* ADC1 channel 1, long sample time, continuous software conversion. */
    ADC1->CR1 = 0u;
    ADC1->CR2 = ADC_CR2_CONT;
    ADC1->SMPR2 = (7u << 3); /* Channel 1 = 239.5 cycles. */
    ADC1->SQR1 = 0u;
    ADC1->SQR2 = 0u;
    ADC1->SQR3 = 1u;
    ADC1->CR2 |= ADC_CR2_ADON;
    ADC1->CR2 |= ADC_CR2_RSTCAL;
    while ((ADC1->CR2 & ADC_CR2_RSTCAL) != 0u) { }
    ADC1->CR2 |= ADC_CR2_CAL;
    while ((ADC1->CR2 & ADC_CR2_CAL) != 0u) { }
    ADC1->CR2 |= ADC_CR2_ADON; /* A second ADON starts conversion. */
#endif
}

