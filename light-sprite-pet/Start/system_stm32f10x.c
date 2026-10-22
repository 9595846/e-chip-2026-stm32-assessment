#include "stm32f10x.h"
#include "config.h"
uint32_t SystemCoreClock=CPU_HZ;
void SystemCoreClockUpdate(void) { SystemCoreClock=CPU_HZ; }
void SystemInit(void) {
    uint32_t budget=200000u;
    RCC_DeInit(); RCC_HSICmd(ENABLE);
    while(RCC_GetFlagStatus(RCC_FLAG_HSIRDY)==RESET && --budget) {}
    FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);
    FLASH_SetLatency(FLASH_Latency_2);
    RCC_HCLKConfig(RCC_SYSCLK_Div1); RCC_PCLK1Config(RCC_HCLK_Div2); RCC_PCLK2Config(RCC_HCLK_Div1);
    RCC_PLLConfig(RCC_PLLSource_HSI_Div2,RCC_PLLMul_16); RCC_PLLCmd(ENABLE);
    budget=200000u; while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY)==RESET && --budget) {}
    if(!budget) for(;;) {} /* Startup clock failure; no outputs have been enabled. */
    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
    budget=200000u; while(RCC_GetSYSCLKSource()!=0x08u && --budget) {}
    if(!budget) for(;;) {}
    SCB->VTOR=FLASH_BASE;
}

