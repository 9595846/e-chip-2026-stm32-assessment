#include "servo.h"
#include "config.h"
#include "stm32f10x.h"

#if APP_STAGE >= 4

typedef enum { SERVO_IDLE = 0, SERVO_WAG, SERVO_SWAY } ServoMode;

static uint8_t g_enabled;
static uint8_t g_angle = SERVO_CENTER_ANGLE;
static ServoMode g_mode;
static uint32_t g_until;
static uint32_t g_last_step;
static uint8_t g_phase;

static uint16_t angle_to_pulse(uint8_t angle)
{
    uint32_t span_angle = SERVO_MAX_ANGLE - SERVO_MIN_ANGLE;
    uint32_t span_pulse = SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US;
    if (angle < SERVO_MIN_ANGLE) { angle = SERVO_MIN_ANGLE; }
    if (angle > SERVO_MAX_ANGLE) { angle = SERVO_MAX_ANGLE; }
    return (uint16_t)(SERVO_MIN_PULSE_US +
        ((uint32_t)(angle - SERVO_MIN_ANGLE) * span_pulse + span_angle / 2u) / span_angle);
}

void servo_init(void)
{
    GPIO_InitTypeDef gpio;
    TIM_TimeBaseInitTypeDef timer;
    TIM_OCInitTypeDef oc;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_TIM1, ENABLE);
    gpio.GPIO_Pin = GPIO_Pin_8;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &gpio);
    TIM_TimeBaseStructInit(&timer);
    timer.TIM_Prescaler = 63u;
    timer.TIM_Period = 19999u;
    timer.TIM_ClockDivision = TIM_CKD_DIV1;
    timer.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM1, &timer);
    TIM_OCStructInit(&oc);
    oc.TIM_OCMode = TIM_OCMode_PWM1;
    oc.TIM_OutputState = TIM_OutputState_Enable;
    oc.TIM_Pulse = angle_to_pulse(SERVO_CENTER_ANGLE);
    oc.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init(TIM1, &oc);
    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM1, ENABLE);
    TIM_Cmd(TIM1, ENABLE);
    TIM_CtrlPWMOutputs(TIM1, ENABLE);
    g_enabled = 1u;
    g_angle = SERVO_CENTER_ANGLE;
    g_mode = SERVO_IDLE;
}

void servo_set_angle(uint8_t angle)
{
    if (angle < SERVO_MIN_ANGLE) { angle = SERVO_MIN_ANGLE; }
    if (angle > SERVO_MAX_ANGLE) { angle = SERVO_MAX_ANGLE; }
    g_angle = angle;
    TIM_SetCompare1(TIM1, angle_to_pulse(angle));
}

void servo_enable(uint8_t enabled)
{
    g_enabled = enabled ? 1u : 0u;
    TIM_CCxCmd(TIM1, TIM_Channel_1, g_enabled ? TIM_CCx_Enable : TIM_CCx_Disable);
    if (!g_enabled) { g_mode = SERVO_IDLE; }
}

void servo_center(void)
{
    servo_enable(1u);
    g_mode = SERVO_IDLE;
    servo_set_angle(SERVO_CENTER_ANGLE);
}

void servo_wag(uint32_t now)
{
    servo_enable(1u);
    g_mode = SERVO_WAG;
    g_until = now + 1200u;
    g_last_step = now - 180u;
    g_phase = 0u;
}

void servo_sway(uint32_t now)
{
    servo_enable(1u);
    g_mode = SERVO_SWAY;
    g_until = now + 1400u;
    g_last_step = now - 100u;
    g_phase = 0u;
}

void servo_task(uint32_t now)
{
    uint32_t interval;
    uint8_t left;
    uint8_t right;
    if (g_mode == SERVO_IDLE || !g_enabled) { return; }
    if ((int32_t)(now - g_until) >= 0) { servo_center(); return; }
    interval = g_mode == SERVO_WAG ? 180u : 100u;
    if ((uint32_t)(now - g_last_step) < interval) { return; }
    g_last_step = now;
    g_phase ^= 1u;
    if (g_mode == SERVO_WAG) { left = 70u; right = 110u; }
    else { left = 52u; right = 128u; }
    servo_set_angle(g_phase ? left : right);
}

uint8_t servo_angle(void) { return g_angle; }
uint8_t servo_is_enabled(void) { return g_enabled; }

#else
void servo_init(void) { }
void servo_set_angle(uint8_t angle) { (void)angle; }
void servo_enable(uint8_t enabled) { (void)enabled; }
void servo_wag(uint32_t now) { (void)now; }
void servo_sway(uint32_t now) { (void)now; }
void servo_center(void) { }
void servo_task(uint32_t now) { (void)now; }
uint8_t servo_angle(void) { return 0u; }
uint8_t servo_is_enabled(void) { return 0u; }
#endif

