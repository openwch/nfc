/********************************** (C) COPYRIGHT *******************************
 * File Name          : NFC_Reader_bsp.c
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2023/05/08
 * Description        : NFC硬件底层初始化，使用TS位设置主从定时器同步控制波形
 * Copyright (c) 2023 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

#include "NFC_Reader_bsp.h"

/* 每个文件单独debug打印的开关，置0可以禁止本文件内部打印 */
#define DEBUG_PRINT_IN_THIS_FILE 0
#if DEBUG_PRINT_IN_THIS_FILE
    #define PRINTF(...) PRINT(__VA_ARGS__)
#else
    #define PRINTF(...) do {} while (0)
#endif

#if ((defined (TIM1)) && (MAIN_PWM_TIM==1))
#if ((defined (TIM2)) && (CTRL_REC_TIM==2)) || ((defined (TIM3)) && (CTRL_REC_TIM==3)) || ((defined (TIM4)) && (CTRL_REC_TIM==4))
#define TIM_NFC_CTRL_TS             0x0000
#else
#error "CTRL_REC_TIM NOT SUPPORTTED!"
#endif
#elif ((defined (TIM8)) && (MAIN_PWM_TIM==8))
#if ((defined (TIM2)) && (CTRL_REC_TIM==2))
#define TIM_NFC_CTRL_TS             0x0010
#elif ((defined (TIM4)) && (CTRL_REC_TIM==4)) || ((defined (TIM5)) && (CTRL_REC_TIM==5))
#define TIM_NFC_CTRL_TS             0x0030
#else
#error "CTRL_REC_TIM NOT SUPPORTTED!"
#endif
#else
#error "MAIN_PWM_TIM NOT SUPPORTTED!"
#endif

#if (NFC_SYS_FREQUENCY >= 120000000)                                        /* 135.6Mhz */
#define TIM_NFC_REC_CHCTLRx         0xf1
#elif (NFC_SYS_FREQUENCY >= 100000000) && (NFC_SYS_FREQUENCY < 120000000)   /* 108.48Mhz */
#define TIM_NFC_REC_CHCTLRx         0xe1
#elif (NFC_SYS_FREQUENCY >= 80000000) && (NFC_SYS_FREQUENCY < 100000000)    /* 81.36Mhz */
#define TIM_NFC_REC_CHCTLRx         0xd1
#elif (NFC_SYS_FREQUENCY >= 60000000) && (NFC_SYS_FREQUENCY < 80000000)
#define TIM_NFC_REC_CHCTLRx         0xc1
#elif (NFC_SYS_FREQUENCY >= 40000000) && (NFC_SYS_FREQUENCY < 60000000)     /* 54.24Mhz */
#define TIM_NFC_REC_CHCTLRx         0xb1
#elif (NFC_SYS_FREQUENCY >= 20000000) && (NFC_SYS_FREQUENCY < 40000000)     /* 27.12Mhz */
#define TIM_NFC_REC_CHCTLRx         0x81
#else
#error "NFC_SYS_FREQUENCY NOT SUPPORTTED!"                                  /* 其他不标准波形，非必要尽量不要使用 */
#endif

#define TIM_REC_PSC_LOAD            ((NFC_SYS_FREQUENCY - 1000000) / 2000000)
#define TIM_REC_ATRLR_LOAD          ((NFC_OVER_TIME * (NFC_SYS_FREQUENCY / (TIM_REC_PSC_LOAD + 1)) / 1000) + 40)
#define TIM_REC_CNT_LOAD            (TIM_REC_ATRLR_LOAD - 120)
#define TIM_REC_STEP_LOAD           (NFC_OVER_TIME_STEP * (NFC_SYS_FREQUENCY / (TIM_REC_PSC_LOAD + 1)) / 1000)
#define TIM_REC_CCER_LOAD           ((0x0001) << ((TIM_NFC_REC_CCx-1)*4))

/* nfc信号控制结构体 */
volatile nfc_pcd_signal_data_t g_nfc_pcd_signal_data;

/*********************************************************************
 * @fn      NFC_OPA_Init
 *
 * @brief   Initializes NFC OPA collection.
 *
 * @return  none
 */
void NFC_OPA_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    OPA_InitTypeDef  OPA_InitStructure;

    /*
     *@Note
     OPA1_CHP1——PB0
     OPA1_CHN1——PA6
     OPA1_OUT_IO_OUT0——PA3
    */

    GPIO_InitStructure.GPIO_Pin = NFC_OPA_CHN_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(NFC_OPA_CHN_GPIOx, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = NFC_OPA_CHP_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(NFC_OPA_CHP_GPIOx, &GPIO_InitStructure);

    OPA_InitStructure.OPA_NUM = NFC_OPA;
    OPA_InitStructure.PSEL = NFC_OPA_PSEL;
    OPA_InitStructure.NSEL = NFC_OPA_NSEL;
    OPA_InitStructure.Mode = NFC_OPA_MODE;
    OPA_Init(&OPA_InitStructure);
    OPA_Cmd(NFC_OPA, DISABLE);

}

/*********************************************************************
 * @fn      NFC_PWMOutGPIO_Init
 *
 * @brief   Initializes TIMx(MAIN_PWM_TIM) output gpio.
 *
 * @param   none
 *
 * @return  none
 */
void NFC_PWMOutGPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* TIMx(MAIN_PWM_TIM)->CH1 */
    GPIO_InitStructure.GPIO_Pin = MAIN_PWM_TIM_CHx_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(MAIN_PWM_TIM_CHx_GPIOx, &GPIO_InitStructure);

    /* TIMx(MAIN_PWM_TIM)->CH1N */
    GPIO_InitStructure.GPIO_Pin = MAIN_PWM_TIM_CHxN_PIN;
    GPIO_Init(MAIN_PWM_TIM_CHxN_GPIOx, &GPIO_InitStructure);
}

/*********************************************************************
 * @fn      NFC_REC_NVIC_GPIO_Init
 *
 * @brief   NFC TIM input capture init.
 *
 * @param   none
 *
 * @return  none
 */
void NFC_REC_NVIC_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    GPIO_InitStructure.GPIO_Pin = CTRL_REC_TIM_CHx_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(CTRL_REC_TIM_CHx_GPIOx, &GPIO_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = TIMx_IRQn(CTRL_REC_TIM);
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannel = DMAxy_IRQn(NFC_DMA, NFC_DMA_CHANNEL);
    NVIC_Init(&NVIC_InitStructure);

}

/*********************************************************************
 * @fn      NFC_PWMOut_Init
 *
 * @brief   Initializes TIMx(MAIN_PWM_TIM) output compare.
 *
 * @param   arr - the period value.
 *          psc - the prescaler value.
 *          ccp - the pulse value.
 *
 * @return  none
 */
void NFC_PWMOut_Init(u16 arr, u16 psc, u16 ccp)
{
    TIMx(MAIN_PWM_TIM)->CTLR1   = 0x0080;
    TIMx(MAIN_PWM_TIM)->ATRLR   = arr;
    TIMx(MAIN_PWM_TIM)->PSC     = psc;
    TIMx(MAIN_PWM_TIM)->RPTCR   = 0x0000;
    TIMx(MAIN_PWM_TIM)->SWEVGR  = 0x0001;
    TIMx(MAIN_PWM_TIM)->CTLR2   = 0x0020;
    TIMx(MAIN_PWM_TIM)->BDTR    = 0x2d00;

    TIMx(MAIN_PWM_TIM)->TIM_CHxCVR(MAIN_PWM_TIM_CCx) = ccp;

#if (MAIN_PWM_TIM_CCx == 1)
    TIMx(MAIN_PWM_TIM)->CHCTLR1 = 0x0060;
#elif (MAIN_PWM_TIM_CCx == 2)
    TIMx(MAIN_PWM_TIM)->CHCTLR1 = 0x6000;
#elif (MAIN_PWM_TIM_CCx == 3)
    TIMx(MAIN_PWM_TIM)->CHCTLR2 = 0x0060;
#elif (MAIN_PWM_TIM_CCx == 4)
    TIMx(MAIN_PWM_TIM)->CHCTLR1 = 0x6000;
#endif

    TIMx(MAIN_PWM_TIM)->CCER = ((0x0005) << ((MAIN_PWM_TIM_CCx - 1) * 4));
}

/*********************************************************************
 * @fn      NFC_CTRL_REC_Prepare_Init
 *
 * @brief   init rec and control of TIMx(CTRL_REC_TIM).
 *
 * @param   none
 *
 * @return  none
 */
void NFC_CTRL_REC_Prepare_Init(void)
{
    TIMx(CTRL_REC_TIM)->CTLR1     = 0x0004;
    TIMx(CTRL_REC_TIM)->ATRLR     = 128;
    TIMx(CTRL_REC_TIM)->PSC       = 0;

#if (TIM_NFC_REC_CCx==1)
    TIMx(CTRL_REC_TIM)->CHCTLR1   = (TIM_NFC_REC_CHCTLRx);
#elif (TIM_NFC_REC_CCx==2)
    TIMx(CTRL_REC_TIM)->CHCTLR1   = (TIM_NFC_REC_CHCTLRx << 8);
#elif (TIM_NFC_REC_CCx==3)
    TIMx(CTRL_REC_TIM)->CHCTLR2   = (TIM_NFC_REC_CHCTLRx);
#elif (TIM_NFC_REC_CCx==4)
    TIMx(CTRL_REC_TIM)->CHCTLR2   = (TIM_NFC_REC_CHCTLRx << 8);
#endif

    TIMx(CTRL_REC_TIM)->CCER      = 0;
    TIMx(CTRL_REC_TIM)->CNT       = 0;
    TIMx(CTRL_REC_TIM)->SWEVGR    = 0x0001;
    TIMx(CTRL_REC_TIM)->SMCFGR    = 0;
}

/*********************************************************************
 * @fn      NFC_DMA_Prepare
 *
 * @brief   init nfc dma.
 *
 * @param   none
 *
 * @return  none
 */
void NFC_DMA_Prepare(void)
{
    uint32_t tmpreg;

    DMAx_CHANNELy(NFC_DMA, NFC_DMA_CHANNEL)->CFGR &= (~DMA_CFGR1_EN);
    DMAx(NFC_DMA)->INTFCR |= ((uint32_t)(DMA_GIFx(NFC_DMA_CHANNEL) | DMA_TCIFx(NFC_DMA_CHANNEL) | DMA_HTIFx(NFC_DMA_CHANNEL) | DMA_TEIFx(NFC_DMA_CHANNEL)));

    tmpreg = DMAx_CHANNELy(NFC_DMA, NFC_DMA_CHANNEL)->CFGR;
    tmpreg &= 0xFFFF800F;
    tmpreg |= 0x3190;

    DMAx_CHANNELy(NFC_DMA, NFC_DMA_CHANNEL)->CFGR = tmpreg;
    DMAx_CHANNELy(NFC_DMA, NFC_DMA_CHANNEL)->PADDR = (uint32_t) & (TIMx(CTRL_REC_TIM)->ATRLR);
}

/*********************************************************************
 * @fn      nfc_send_start_dma
 *
 * @brief   start control of TIMx(CTRL_REC_TIM).
 *
 * @param   data - the dma data.
 *          len - the dma data len.
 *
 * @return  none
 */
void nfc_send_start_dma(uint8_t *data, uint16_t len)
{
    if (len != 0)
    {
        PRINTF("nfc_send_start_dma:");
        for(uint16_t i = 0; i < len; i++)
        {
            PRINTF(" %02x", data[i]);
        }
        PRINTF("\n");
        DMAx_CHANNELy(NFC_DMA, NFC_DMA_CHANNEL)->CFGR &= (~DMA_CFGR1_EN);
        DMAx(NFC_DMA)->INTFCR |= ((uint32_t)(DMA_GIFx(NFC_DMA_CHANNEL) | DMA_TCIFx(NFC_DMA_CHANNEL) | DMA_HTIFx(NFC_DMA_CHANNEL) | DMA_TEIFx(NFC_DMA_CHANNEL)));

        DMAx_CHANNELy(NFC_DMA, NFC_DMA_CHANNEL)->CNTR = len;
        DMAx_CHANNELy(NFC_DMA, NFC_DMA_CHANNEL)->MADDR = (uint32_t)data;

        TIMx(CTRL_REC_TIM)->ATRLR     = 128;
        TIMx(CTRL_REC_TIM)->PSC       = 0;

        TIMx(CTRL_REC_TIM)->CCER      = 0;

        TIMx(CTRL_REC_TIM)->SMCFGR    &= 0xfff8;
        TIMx(CTRL_REC_TIM)->SMCFGR    = TIM_NFC_CTRL_TS;
        TIMx(CTRL_REC_TIM)->SMCFGR    = 0x0007 | TIM_NFC_CTRL_TS;

        TIMx(CTRL_REC_TIM)->CNT = 0;

        TIMx(CTRL_REC_TIM)->DMAINTENR = 0x0100;
        TIMx(CTRL_REC_TIM)->SWEVGR    = 0x0001;
        TIMx(CTRL_REC_TIM)->CTLR1     = 0x0005;

        DMAx_CHANNELy(NFC_DMA, NFC_DMA_CHANNEL)->CFGR |= (DMA_IT_TC | DMA_IT_TE | DMA_CFGR1_EN);

        /* 开始中断 */
        TIMx(CTRL_REC_TIM)->INTFR = (~(TIM_IT_Update));

        TIMx(CTRL_REC_TIM)->DMAINTENR = 0x0101;
    }
    else
    {
        PRINTF("nfc_send_start_dma error len\n");
        g_nfc_pcd_signal_data.index = 0;
        g_nfc_pcd_signal_data.state = NFC_PCD_REC_STATE_OK;
    }
}

/*********************************************************************
 * @fn      nfc_signal_antenna_off
 *
 * @brief   turn off the nfc antenna
 *
 * @param   none
 *
 * @return  none
 */
void nfc_signal_antenna_off(void)
{
    TIMx(MAIN_PWM_TIM)->CTLR1 &= (~TIM_CEN);
    TIMx(MAIN_PWM_TIM)->BDTR &= (~TIM_MOE);
}

/*********************************************************************
 * @fn      nfc_signal_antenna_on
 *
 * @brief   open the nfc antenna
 *
 * @param   none
 *
 * @return  none
 */
void nfc_signal_antenna_on(void)
{
    uint16_t chctlr1_value;

#if (MAIN_PWM_TIM_CCx == 1)
    chctlr1_value = TIMx(MAIN_PWM_TIM)->CHCTLR1;

    if ((chctlr1_value & 0x0070) != 0x0060)
    {
        chctlr1_value = chctlr1_value & (~(0x0070));

        chctlr1_value = chctlr1_value | 0x0060;

        TIMx(MAIN_PWM_TIM)->CHCTLR1 = chctlr1_value;
    }
#elif (MAIN_PWM_TIM_CCx == 2)
    chctlr1_value = TIMx(MAIN_PWM_TIM)->CHCTLR1;

    if ((chctlr1_value & 0x7000) != 0x6000)
    {
        chctlr1_value = chctlr1_value & (~(0x7000));

        chctlr1_value = chctlr1_value | 0x6000;

        TIMx(MAIN_PWM_TIM)->CHCTLR1 = chctlr1_value;
    }
#elif (MAIN_PWM_TIM_CCx == 3)
    chctlr1_value = TIMx(MAIN_PWM_TIM)->CHCTLR2;

    if ((chctlr1_value & 0x0070) != 0x0060)
    {
        chctlr1_value = chctlr1_value & (~(0x0070));

        chctlr1_value = chctlr1_value | 0x0060;

        TIMx(MAIN_PWM_TIM)->CHCTLR2 = chctlr1_value;
    }
#elif (MAIN_PWM_TIM_CCx == 4)
    chctlr1_value = TIMx(MAIN_PWM_TIM)->CHCTLR2;

    if ((chctlr1_value & 0x7000) != 0x6000)
    {
        chctlr1_value = chctlr1_value & (~(0x7000));

        chctlr1_value = chctlr1_value | 0x6000;

        TIMx(MAIN_PWM_TIM)->CHCTLR2 = chctlr1_value;
    }
#endif
    TIMx(MAIN_PWM_TIM)->CNT = 0;

    TIMx(MAIN_PWM_TIM)->CTLR1 |= TIM_CEN;
    TIMx(MAIN_PWM_TIM)->BDTR |= TIM_MOE;
}

DMAxy_IRQ_DEF(NFC_DMA, NFC_DMA_CHANNEL) __attribute__((interrupt("WCH-Interrupt-fast")));
/*********************************************************************
 * @fn      DMAxy_IRQ_DEF(NFC_DMA, NFC_DMA_CHANNEL)
 *
 * @brief   This function handles DMAx_CHANNELy(NFC_DMA,NFC_DMA_CHANNEL).
 *
 * @return  none
 */
DMAxy_IRQ_DEF(NFC_DMA, NFC_DMA_CHANNEL)
{
    uint16_t level_diff;

    TIMx(CTRL_REC_TIM)->CTLR1     = 0x0004;

    level_diff = (g_nfc_pcd_signal_data.wait_level * TIM_REC_STEP_LOAD);

    TIMx(CTRL_REC_TIM)->ATRLR     = TIM_REC_ATRLR_LOAD - level_diff;
    TIMx(CTRL_REC_TIM)->PSC       = TIM_REC_PSC_LOAD;

    TIMx(CTRL_REC_TIM)->SMCFGR    = 0x0000;
    TIMx(CTRL_REC_TIM)->SMCFGR    = 0x0050;
    TIMx(CTRL_REC_TIM)->SMCFGR    = 0x0054;

    TIMx(CTRL_REC_TIM)->CCER      = TIM_REC_CCER_LOAD;
    TIMx(CTRL_REC_TIM)->SWEVGR    = 0x0001;
    TIMx(CTRL_REC_TIM)->INTFR     = (~(TIM_IT_Update | TIM_IT_CCx(TIM_NFC_REC_CCx)));
    TIMx(CTRL_REC_TIM)->DMAINTENR = TIM_IT_Update;
    TIMx(CTRL_REC_TIM)->CNT       = TIM_REC_CNT_LOAD - level_diff;
    TIMx(CTRL_REC_TIM)->CTLR1     = 0x0005;

    DMAx(NFC_DMA)->INTFCR = ((uint32_t)(DMA_GIFx(NFC_DMA_CHANNEL) | DMA_TCIFx(NFC_DMA_CHANNEL) | DMA_HTIFx(NFC_DMA_CHANNEL) | DMA_TEIFx(NFC_DMA_CHANNEL)));
}

TIMx_IRQ_DEF(CTRL_REC_TIM) __attribute__((interrupt("WCH-Interrupt-fast")));;
/*********************************************************************
 * @fn      TIMx_IRQ_DEF(CTRL_REC_TIM)
 *
 * @brief   This function handles TIMx(CTRL_REC_TIM).
 *
 * @return  none
 */
TIMx_IRQ_DEF(CTRL_REC_TIM)
{
    if (TIMx(CTRL_REC_TIM)->CCER == 0)
    {
#if (MAIN_PWM_TIM_CCx == 1)
        TIMx(MAIN_PWM_TIM)->CHCTLR1 ^= 0x0040;
#elif (MAIN_PWM_TIM_CCx == 2)
        TIMx(MAIN_PWM_TIM)->CHCTLR1 ^= 0x4000;
#elif (MAIN_PWM_TIM_CCx == 3)
        TIMx(MAIN_PWM_TIM)->CHCTLR2 ^= 0x0040;
#elif (MAIN_PWM_TIM_CCx == 4)
        TIMx(MAIN_PWM_TIM)->CHCTLR2 ^= 0x4000;
#endif
    }
    else
    {
        uint16_t itstatus, itenable;

        itstatus = TIMx(CTRL_REC_TIM)->INTFR;
        itenable = TIMx(CTRL_REC_TIM)->DMAINTENR;
        if (((itstatus & TIM_IT_CCx(TIM_NFC_REC_CCx)) != RESET) && ((itenable & TIM_IT_CCx(TIM_NFC_REC_CCx)) != RESET))
        {
            uint16_t cap_value;
            TIMx(CTRL_REC_TIM)->CNT = 0;
            cap_value = TIMx(CTRL_REC_TIM)->TIM_CHxCVR(TIM_NFC_REC_CCx);
            if (g_nfc_pcd_signal_data.index >= 254)
            {
                g_nfc_pcd_signal_data.state = NFC_PCD_REC_STATE_OK;
                TIMx(CTRL_REC_TIM)->CTLR1 = 0x0004;
                TIMx(CTRL_REC_TIM)->DMAINTENR = 0;
            }
            else if (g_nfc_pcd_signal_data.index == 0)
            {
                TIMx(CTRL_REC_TIM)->ATRLR = 45;
            }
            g_nfc_pcd_signal_data.buf[g_nfc_pcd_signal_data.index] = cap_value & 0xff;
            g_nfc_pcd_signal_data.index++;
        }

        if (((itstatus & TIM_IT_Update) != RESET) && ((itenable & TIM_IT_Update) != RESET))
        {
            if (g_nfc_pcd_signal_data.state == NFC_PCD_REC_STATE_WAITING)
            {
                TIMx(CTRL_REC_TIM)->CNT = 0;
                g_nfc_pcd_signal_data.state = NFC_PCD_REC_STATE_WORKING;
                g_nfc_pcd_signal_data.index = 0;
                TIMx(CTRL_REC_TIM)->DMAINTENR |= TIM_IT_CCx(TIM_NFC_REC_CCx);
            }
            else
            {
                g_nfc_pcd_signal_data.state = NFC_PCD_REC_STATE_OK;
                TIMx(CTRL_REC_TIM)->CTLR1 = 0x0004;
                TIMx(CTRL_REC_TIM)->DMAINTENR = 0;
            }
        }
    }
    TIMx(CTRL_REC_TIM)->INTFR = (~(TIM_IT_Update | TIM_IT_CCx(TIM_NFC_REC_CCx)));
}
