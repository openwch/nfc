/********************************** (C) COPYRIGHT *******************************
 * File Name          : SLEEP.c
 * Author             : WCH
 * Version            : V1.2
 * Date               : 2022/01/18
 * Description        : 睡眠配置及其初始化
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

/******************************************************************************/
/* 头文件包含 */
#include "HAL.h"

#define US_TO_TICK(us)                  (uint32_t)((us)/(1000000/((CAB_LSIFQ/2))))

#define SLEEP_PERIOD_MIN_US             200
#define SLEEP_PERIOD_MAX_TICK           0xFFD2393F
#define SLEEP_PERIOD_MIN_TICK           US_TO_TICK(SLEEP_PERIOD_MIN_US)
#define HSEREADY_TICK                   US_TO_TICK(WAKE_UP_MAX_TIME_US)

/*******************************************************************************
 * @fn          BLE_LowPower
 *
 * @brief       启动睡眠
 *
 * @param   time    - 唤醒的时间点（RTC绝对值）
 *
 * @return      state.
 */
uint32_t BLE_LowPower(uint32_t time)
{
#if(defined(HAL_SLEEP)) && (HAL_SLEEP == TRUE)
    uint32_t current_time;
    uint32_t sleep_period;
    uint32_t wake_time;

    wake_time = time - HSEREADY_TICK;

    __disable_irq();
    current_time = RTC_GetCounter();
    sleep_period = wake_time - current_time;

    if((sleep_period < SLEEP_PERIOD_MIN_TICK)
            || (sleep_period > SLEEP_PERIOD_MAX_TICK)) {
        __enable_irq();
        return 2;
    }
    RTC_SetTignTime(wake_time);
    __enable_irq();

  #if(DEBUG == DEBUG_UART1) // 使用其他串口输出打印信息需要修改这行代码
    while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET)
    {
        __NOP();
    }
    #elif(DEBUG == DEBUG_UART2)
    while(USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET)
    {
        __NOP();
    }
  #endif
    // LOW POWER-sleep模式
    if((RTC->CTLRL & RTC_FLAG_ALR) == 0) {

        PWR_EnterSTOPMode_RAM_LV(PWR_Regulator_LowPower, PWR_STOPEntry_WFE);

        GPIO_ResetBits(GPIOC, GPIO_Pin_13);

        RCC->CTLR |= (uint32_t)0x00000001;
        RCC->CFGR0 &= (uint32_t)0xF8FF0000;
        RCC->CTLR &= (uint32_t)0xFEF6FFFF;
        RCC->CTLR &= (uint32_t)0xFFFBFFFF;
        RCC->CFGR0 &= (uint32_t)0xFF80FFFF;
        RCC->INTR = 0x009F0000;

        __IO uint32_t StartUpCounter = 0, HSEStatus = 0;

        RCC->CTLR |= ((uint32_t)RCC_HSEON);

        /* HCLK = SYSCLK - PCLK2 = HCLK - PCLK1 = HCLK */
        RCC->CFGR0 |= (uint32_t)(RCC_HPRE_DIV1 | RCC_PPRE2_DIV1 | RCC_PPRE1_DIV2);

        /*  PLL configuration: PLLCLK = HSE * 12 = 96 MHz */
        RCC->CFGR0 &= (uint32_t)((uint32_t) ~(RCC_PLLSRC | RCC_PLLXTPRE |
                                              RCC_PLLMULL));

        RCC->CFGR0 |= (uint32_t)(RCC_PLLSRC_HSE | RCC_PLLXTPRE_HSE | RCC_PLLMULL12);

        /* Enable PLL */
        RCC->CTLR |= RCC_PLLON;

        /* Select PLL as system clock source */
        RCC->CFGR0 &= (uint32_t)((uint32_t)~(RCC_SW));
        RCC->CFGR0 |= (uint32_t)RCC_SW_PLL;

        __disable_irq();
        RTC_SetTignTime(time);
        __enable_irq();

        __WFE();

        do
        {
            HSEStatus = RCC->CTLR & RCC_HSERDY;
            StartUpCounter++;
        } while((HSEStatus == 0) && (StartUpCounter != HSE_STARTUP_TIMEOUT));

        /* Wait till PLL is used as system clock source */
        while ((RCC->CFGR0 & (uint32_t)RCC_SWS) != (uint32_t)0x08)
        {
        }

    } else {
       return 3; 
    }

#endif
    return 0;
}

/*******************************************************************************
 * @fn      HAL_SleepInit
 *
 * @brief   配置睡眠唤醒的方式   - RTC唤醒，触发模式
 *
 * @param   None.
 *
 * @return  None.
 */
void HAL_SleepInit(void)
{
#if(defined(HAL_SLEEP)) && (HAL_SLEEP == TRUE)
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

    EXTI_InitTypeDef EXTI_InitStructure = {0};
    NVIC_InitTypeDef NVIC_InitStructure = {0};

//    EXTI->INTENR &= ~EXTI_Line17;
//    EXTI->EVENR |= EXTI_Line17;
//    EXTI->FTENR &= ~EXTI_Line17;
//    EXTI->RTENR |= EXTI_Line17;

    EXTI_InitStructure.EXTI_Line = EXTI_Line17;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Event;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

//    NVIC_InitStructure.NVIC_IRQChannel = RTCAlarm_IRQn;
//    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
//    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//    NVIC_Init(&NVIC_InitStructure);
#endif
}
