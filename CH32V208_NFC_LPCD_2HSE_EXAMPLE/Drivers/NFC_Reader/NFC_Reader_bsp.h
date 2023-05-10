/********************************** (C) COPYRIGHT *******************************
 * File Name          : NFC_Reader_bsp.h
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2023/05/08
 * Description        : NFC硬件底层初始化
 * Copyright (c) 2023 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

#ifndef _NFC_READER_BSP_H_
#define _NFC_READER_BSP_H_

#include "debug.h"
#include "wch_nfc_pcd.h"

#define NFC_SYS_FREQUENCY           108480000       /* 系统运行时钟，单位HZ */

#define MAIN_PWM_TIM                1               /* 发送的nfc基本波形的高级定时器，1即为TIM1 */
#define MAIN_PWM_TIM_CCx            1               /* 发送的nfc基本波形的高级定时器的通道 */
#define MAIN_PWM_TIM_CHx_GPIOx      GPIOA           /* 发送的nfc基本波形的高级定时器的通道对应的GPIO组 */
#define MAIN_PWM_TIM_CHx_PIN        GPIO_Pin_8      /* 发送的nfc基本波形的高级定时器的通道对应的GPIO PIN */
#define MAIN_PWM_TIM_CHxN_GPIOx     GPIOB           /* 发送的nfc基本波形的高级定时器的互补通道对应的GPIO组 */
#define MAIN_PWM_TIM_CHxN_PIN       GPIO_Pin_13     /* 发送的nfc基本波形的高级定时器的互补通道对应的GPIO PIN */

#define CTRL_REC_TIM                2               /* 发送和接收载波的定时器，2即为TIM2 */
#define TIM_NFC_REC_CCx             4               /* 接收载波的定时器捕获通道 */
#define CTRL_REC_TIM_CHx_GPIOx      GPIOA           /* 接收的nfc波形的定时器的通道对应的GPIO组 */
#define CTRL_REC_TIM_CHx_PIN        GPIO_Pin_3      /* 接收的nfc波形的定时器的通道对应的GPIO PIN */

#define NFC_DMA                     1               /* 发送使用的DMA，即CTRL_REC_TIM的UPDATE更新事件所对应的DMA通道，1即为DMA1_Channely */
#define NFC_DMA_CHANNEL             2               /* 发送使用的DMA下的通道，2即为DMAx_Channel2 */

#define NFC_OPA                     OPA1            /* 使用的放大器，OPA_Num_TypeDef */
#define NFC_OPA_PSEL                CHP1            /* 选择的放大器的正向输入通道，OPA_PSEL_TypeDef */
#define NFC_OPA_NSEL                CHN1            /* 选择的放大器的负向输入通道，OPA_NSEL_TypeDef */
#define NFC_OPA_MODE                OUT_IO_OUT0     /* 选择的放大器的输出通道，OPA_Mode_TypeDef */
#define NFC_OPA_CHP_GPIOx           GPIOB           /* 芯片集成放大器正向输入端引脚对应的GPIO组 */
#define NFC_OPA_CHP_PIN             GPIO_Pin_0      /* 芯片集成放大器正向输入端引脚对应的GPIO PIN */
#define NFC_OPA_CHN_GPIOx           GPIOA           /* 芯片集成放大器负向输入端引脚对应的GPIO组 */
#define NFC_OPA_CHN_PIN             GPIO_Pin_6      /* 芯片集成放大器负向输入端引脚对应的GPIO PIN */

#define _TIM_CHxCVR(X)              CH##X##CVR
#define _TIM_IT_CCx(X)              TIM_IT_CC##X
#define _TIMx_IRQn(X)               TIM##X##_IRQn
#define _TIMx_IRQ_DEF(X)            void TIM##X##_IRQHandler(void)
#define _TIMx(X)                    TIM##X

#define TIM_CHxCVR(X)               _TIM_CHxCVR(X)
#define TIM_IT_CCx(X)               _TIM_IT_CCx(X)
#define TIMx_IRQn(X)                _TIMx_IRQn(X)
#define TIMx_IRQ_DEF(X)             _TIMx_IRQ_DEF(X)
#define TIMx(X)                     _TIMx(X)

#define _DMAx_CHANNELy(X,Y)         DMA##X##_Channel##Y
#define _DMAxy_IRQn(X,Y)            DMA##X##_Channel##Y##_IRQn
#define _DMAxy_IRQ_DEF(X,Y)         void DMA##X##_Channel##Y##_IRQHandler(void)
#define _DMA_TEIFx(X)               DMA_TEIF##X
#define _DMA_HTIFx(X)               DMA_HTIF##X
#define _DMA_TCIFx(X)               DMA_TCIF##X
#define _DMA_GIFx(X)                DMA_GIF##X
#define _DMAx_IT_TEy(X,Y)           DMA##X##_IT_TE##Y
#define _DMAx_IT_HTy(X,Y)           DMA##X##_IT_HT##Y
#define _DMAx_IT_TCy(X,Y)           DMA##X##_IT_TC##Y
#define _DMAx_IT_GLy(X,Y)           DMA##X##_IT_GL##Y
#define _DMAx_FLAG_TEy(X,Y)         DMA##X##_FLAG_TE##Y
#define _DMAx_FLAG_HTy(X,Y)         DMA##X##_FLAG_HT##Y
#define _DMAx_FLAG_TCy(X,Y)         DMA##X##_FLAG_TC##Y
#define _DMAx_FLAG_GLy(X,Y)         DMA##X##_FLAG_GL##Y
#define _DMAx(X)                    DMA##X

#define DMAx_CHANNELy(X,Y)          _DMAx_CHANNELy(X,Y)
#define DMAxy_IRQn(X,Y)             _DMAxy_IRQn(X,Y)
#define DMAxy_IRQ_DEF(X,Y)          _DMAxy_IRQ_DEF(X,Y)
#define DMA_TEIFx(X)                _DMA_TEIFx(X)
#define DMA_HTIFx(X)                _DMA_HTIFx(X)
#define DMA_TCIFx(X)                _DMA_TCIFx(X)
#define DMA_GIFx(X)                 _DMA_GIFx(X)
#define DMAx_IT_TEy(X,Y)            _DMAx_IT_TEy(X,Y)
#define DMAx_IT_HTy(X,Y)            _DMAx_IT_HTy(X,Y)
#define DMAx_IT_TCy(X,Y)            _DMAx_IT_TCy(X,Y)
#define DMAx_IT_GLy(X,Y)            _DMAx_IT_GLy(X,Y)
#define DMAx_FLAG_TEy(X,Y)          _DMAx_FLAG_TEy(X,Y)
#define DMAx_FLAG_HTy(X,Y)          _DMAx_FLAG_HTy(X,Y)
#define DMAx_FLAG_TCy(X,Y)          _DMAx_FLAG_TCy(X,Y)
#define DMAx_FLAG_GLy(X,Y)          _DMAx_FLAG_GLy(X,Y)
#define DMAx(X)                     _DMAx(X)

/*********************************************************************
 * @fn      NFC_OPA_Init
 *
 * @brief   Initializes NFC OPA collection.
 *
 * @return  none
 */
extern void NFC_OPA_Init( void );

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
extern void NFC_PWMOut_Init(u16 arr, u16 psc, u16 ccp);

/*********************************************************************
 * @fn      NFC_PWMOutGPIO_Init
 *
 * @brief   Initializes TIMx(MAIN_PWM_TIM) output gpio.
 *
 * @param   none
 *
 * @return  none
 */
extern void NFC_PWMOutGPIO_Init(void);

/*********************************************************************
 * @fn      NFC_REC_NVIC_GPIO_Init
 *
 * @brief   NFC TIM input capture init.
 *
 * @param   none
 *
 * @return  none
 */
extern void NFC_REC_NVIC_GPIO_Init(void);

/*********************************************************************
 * @fn      nfc_send_start_dma
 *
 * @brief   start control of TIMx(CTRL_REC_TIM).
 *
 * @param   none
 *
 * @return  none
 */
extern void NFC_DMA_Prepare(void);

/*********************************************************************
 * @fn      nfc_send_start_dma
 *
 * @brief   start control of TIMx(CTRL_REC_TIM).
 *
 * @param   none
 *
 * @return  none
 */
extern void NFC_CTRL_REC_Prepare_Init(void);

/*********************************************************************
 * @fn      nfc_signal_antenna_off
 *
 * @brief   turn off the nfc antenna
 *
 * @param   none
 *
 * @return  none
 */
extern void nfc_signal_antenna_off(void);

/*********************************************************************
 * @fn      nfc_signal_antenna_on
 *
 * @brief   open the nfc antenna
 *
 * @param   none
 *
 * @return  none
 */
extern void nfc_signal_antenna_on(void);

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
extern void nfc_send_start_dma(uint8_t *data, uint16_t len);

#endif /* _NFC_READER_BSP_H_ */
