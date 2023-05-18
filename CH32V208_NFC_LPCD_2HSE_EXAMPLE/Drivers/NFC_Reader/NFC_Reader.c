/********************************** (C) COPYRIGHT *******************************
 * File Name          : NFC_Reader.c
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2023/05/08
 * Description        : NFC M1����������
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

#include "NFC_Reader.h"
#include "NFC_Reader_M1.h"
#include "wchble.h"

/* ÿ���ļ�����debug��ӡ�Ŀ��أ���0���Խ�ֹ���ļ��ڲ���ӡ */
#define DEBUG_PRINT_IN_THIS_FILE 1
#if DEBUG_PRINT_IN_THIS_FILE
    #define PRINTF(...) PRINT(__VA_ARGS__)
#else
    #define PRINTF(...) do {} while (0)
#endif

#define NFC_LPCD_THRESHOLD_VALUE    3300

uint8_t write_data[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
uint8_t picc_uuid[10];
uint8_t picc_uuid_len;
uint8_t default_key[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void nfc_signal_bsp_init(void)
{
    NFC_PWMOutGPIO_Init();
    NFC_PWMOut_Init(6, 0, 4);

    NFC_REC_NVIC_GPIO_Init();
    NFC_CTRL_REC_Prepare_Init();
    NFC_DMA_Prepare();      /* ����DMA���ͣ���ʼ�� */

    NFC_OPA_Init();

    nfc_pcd_cypto_rand_register(tmos_rand);     /* ע��������������� */

}

s32 adcValueRead(void)
{
    s32 adcv = 0;
    uint8_t i;

    ADC1->CTLR1 &= (uint32_t)0xE0F0FEFF;
    ADC1->CTLR1 |= (uint32_t)(ADC_Mode_Independent | (DISABLE << 8));
    ADC1->CTLR2 &= (uint32_t)0xFFF1F7FD;
    ADC1->CTLR2 |= (uint32_t)(ADC_DataAlign_Right | ADC_ExternalTrigConv_None | (DISABLE << 1));
    ADC1->RSQR1 &= (uint32_t)0xFF0FFFFF;
    ADC1->RSQR1 |= (uint32_t)7 << 20;
    ADC1->SAMPTR2 |= (uint32_t)0X07000000;  //ADC_SampleTime_239Cycles5 << 24;
    ADC1->RSQR3 &= ~(uint32_t)0x0000001F;
    ADC1->RSQR3 |= (uint32_t)0X08;          //ADC_Channel_8;
    ADC1->CTLR2 |= (uint32_t)0x00000001;
//  Delay_Us(1);

    for(i=0;i<5;i++)
    {
        ADC1->CTLR2 |= (uint32_t)0x00500000;    //ADC_SoftwareStartConvCmd(ADC1, ENABLE);//
        while ((ADC1->STATR & ADC_FLAG_EOC) == (uint8_t)RESET);
        adcv += (uint16_t)ADC1->RDATAR;
    }
    ADC1->CTLR2 &= (uint32_t)0xFFFFFFFE;

//  for(uint8_t i=0;i<((sizeof(ADC_TypeDef)/4)-5); i++)
//      PRINT("2.%u ADC1:%08X\r\n",i,*((uint32_t*)(ADC1_BASE+i*4)));
    adcv /= 5;

    return adcv;
}

void NFC_Process(void)
{
    s32 adcvalue;

    nfc_signal_antenna_on();        /* �������� */
    adcvalue = adcValueRead();
    PRINTF("adc:%d\n", adcvalue);
    if(adcvalue < NFC_LPCD_THRESHOLD_VALUE)         /*  ��ֵ��Ҫ���ݰ��Ӳ�ͬ�����岻ͬ�����ؽ��е��� */
    {

        nfc_signal_antenna_off();                   /* �ر����� */

        changeHSETo27_12Mhz();                      /* �л�����27.12Mhz��ϵͳ��ƵΪ108.48Mhz */

        TIMx(MAIN_PWM_TIM)->ATRLR = 7;              /* ��Ϊ13.56Mhz */
        nfc_signal_antenna_on();                    /* �������� */

        OPA_Cmd(NFC_OPA, ENABLE);                   /* ��Ҫ����ʱ�򿪷Ŵ��� */
        Delay_Ms(2);                                /* ÿ��������ر����շ���֮��Ӧ������1ms�ļ��������Ҫ�ȳ����ܱ�֤�������� */

        {
            uint16_t res;
            res = PcdRequest(PICC_REQALL);

            if ((res == 0x0004) || (res == 0x0002) || (res == 0x0044))      /* ��ͨm1��4�ֽڿ��ţ�Mifare UltralightΪ7�ֽڿ��� */
            {
                res = PcdAnticoll(PICC_ANTICOLL1);
                if (res == PCD_NO_ERROR)
                {
                    tmos_memcpy(picc_uuid, (const void *)g_nfc_pcd_signal_data.decode_buf.v8, 4);   /*  */
                    PRINTF("ANTICOLL1:%02x %02x %02x %02x\n", g_nfc_pcd_signal_data.decode_buf.v8[0], g_nfc_pcd_signal_data.decode_buf.v8[1], g_nfc_pcd_signal_data.decode_buf.v8[2], g_nfc_pcd_signal_data.decode_buf.v8[3]);
                    res = PcdSelect(PICC_ANTICOLL1, picc_uuid);
                    if (res == PCD_NO_ERROR)
                    {
                        if(picc_uuid[0] == 0x88)
                        {
                            /* ��4�ֽ�nfc����������һ��UUID��Ҫ��ȡ */
                            picc_uuid[0] = picc_uuid[1];
                            picc_uuid[1] = picc_uuid[2];
                            picc_uuid[2] = picc_uuid[3];

                            res = PcdAnticoll(PICC_ANTICOLL2);
                            if (res == PCD_NO_ERROR)
                            {
                                tmos_memcpy(picc_uuid + 3, (const void *)g_nfc_pcd_signal_data.decode_buf.v8, 4);
                                PRINTF("ANTICOLL2:%02x %02x %02x %02x\n", g_nfc_pcd_signal_data.decode_buf.v8[0], g_nfc_pcd_signal_data.decode_buf.v8[1], g_nfc_pcd_signal_data.decode_buf.v8[2], g_nfc_pcd_signal_data.decode_buf.v8[3]);
                                res = PcdSelect(PICC_ANTICOLL2, picc_uuid + 3);
                                if (res == PCD_NO_ERROR)
                                {
                                    if(picc_uuid[3] == 0x88)
                                    {
                                        /* ��7�ֽ�nfc����������һ��UUID��Ҫ��ȡ */
                                        picc_uuid[3] = picc_uuid[4];
                                        picc_uuid[4] = picc_uuid[5];
                                        picc_uuid[5] = picc_uuid[6];

                                        res = PcdAnticoll(PICC_ANTICOLL3);
                                        if (res == PCD_NO_ERROR)
                                        {
                                            tmos_memcpy(picc_uuid + 6, (const void *)g_nfc_pcd_signal_data.decode_buf.v8, 4);
                                            PRINTF("ANTICOLL3:%02x %02x %02x %02x\n", g_nfc_pcd_signal_data.decode_buf.v8[0], g_nfc_pcd_signal_data.decode_buf.v8[1], g_nfc_pcd_signal_data.decode_buf.v8[2], g_nfc_pcd_signal_data.decode_buf.v8[3]);
                                            res = PcdSelect(PICC_ANTICOLL3, picc_uuid + 6);
                                            if (res == PCD_NO_ERROR)
                                            {
                                                /* ��ʱû�и�����UUID */
                                                picc_uuid_len = 10;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        picc_uuid_len = 7;
                                    }
                                }
                            }
                        }
                        else
                        {
                            picc_uuid_len = 4;
                        }

                        if(res == PCD_NO_ERROR)
                        {
                            PRINTF("UUID:");
                            for(uint8_t i = 0; i < picc_uuid_len; i++)
                            {
                                PRINTF(" %02x", picc_uuid[i]);
                            }
                            PRINTF("\nselect OK, SAK:%02x\n", g_nfc_pcd_signal_data.decode_buf.v8[0]);
                        }
                        else
                        {
                            goto nfc_exit;
                        }

                        if(picc_uuid_len != 4)      /* Mifare UltralightЭ���ݲ�֧�ֺ���������ֻ�ɶ����� */
                        {
                            goto nfc_exit;
                        }

                        g_nfc_pcd_signal_data.is_encrypted = 0;     /* select�ɹ����������λ */

    #if 1   /* ���ú������ԣ�����ֱ��HALT�˳� */

                        res = PcdAuthState(PICC_AUTHENT1A, 0, default_key, picc_uuid);
                        if (res == PCD_NO_ERROR)
                        {
                            g_nfc_pcd_signal_data.is_encrypted = 1;     /* ��һ����֤�ɹ�����λ����λ */
                        }
                        else
                        {
                            goto nfc_exit;
                        }

                        for (uint8_t i = 0; i < 4; i++)
                        {
                            res = PcdRead(i);
                            if (res != PCD_NO_ERROR)
                            {
                                PRINTF("ERR: 0x%x\n", res);
                                goto nfc_exit;
                            }
                            PRINTF("block %02d: ", i);
                            for (uint8_t j = 0; j < 16; j++)
                            {
                                PRINTF("%02x ", g_nfc_pcd_signal_data.decode_buf.v8[j]);
                            }
                            PRINTF("\n");
                        }

    #if 1   /* ֵ���ȡ�ͳ�ʼ������ */

                        res = PcdReadValueBlock(1);
                        if (res == PCD_VALUE_BLOCK_INVALID)
                        {
                            PRINTF("not a value block, init it.");
                            uint32_t vdata = 100;
                            res = PcdInitValueBlock(1, (uint8_t *)&vdata, 2);
                            if (res != PCD_NO_ERROR)
                            {
                                PRINTF("ERR: 0x%x\n", res);
                                goto nfc_exit;
                            }
                        }
                        else if (res != PCD_NO_ERROR)
                        {
                            PRINTF("ERR: 0x%x\n", res);
                            goto nfc_exit;
                        }
                        else
                        {
                            PRINTF("value:%d, adr:%d\n", g_nfc_pcd_signal_data.decode_buf.v32[0], g_nfc_pcd_signal_data.decode_buf.v8[12]);
                        }

    #endif  /* ֵ���ȡ�ͳ�ʼ������ */

    #if 1   /* ֵ��ۿ�ͱ��ݲ��� */
                        PRINTF("PcdValue\n");
                        uint32_t di_data = 1;
                        res = PcdValue(PICC_DECREMENT, 1, (uint8_t *)&di_data);
                        if(res != PCD_NO_ERROR)
                        {
                            PRINTF("ERR: 0x%x\n",res);
                            goto nfc_exit;
                        }
                        PRINTF("PcdBakValue\n");
                        res = PcdBakValue(1,2);
                        if(res != PCD_NO_ERROR)
                        {
                            PRINTF("ERR: 0x%x\n",res);
                            goto nfc_exit;
                        }

    #endif  /* ֵ��ۿ�ͱ��ݲ��� */

                        for (uint8_t l = 1; l < 16; l++)
                        {
                            res = PcdAuthState(PICC_AUTHENT1A, 4 * l, default_key, picc_uuid);
                            if (res)
                            {
                                PRINTF("ERR: 0x%x\n", res);
                                goto nfc_exit;
                            }

                            PRINTF("1st read:\n");
                            for (uint8_t i = 0; i < 3; i++)
                            {
                                res = PcdRead(i + 4 * l);
                                if (res)
                                {
                                    PRINTF("ERR: 0x%x\n", res);
                                    goto nfc_exit;
                                }
                                PRINTF("block %02d: ", i + 4 * l);
                                for (uint8_t j = 0; j < 16; j++)
                                {
                                    PRINTF("%02x ", g_nfc_pcd_signal_data.decode_buf.v8[j]);
                                }
                                PRINTF("\n");
                            }

    #if 1   /* ����д����� */

                            for (uint8_t i = 0; i < 16; i++)
                            {
                                write_data[i]++;
                            }

                            for (uint8_t i = 0; i < 3; i++)
                            {
                                res = PcdWrite(i + 4 * l, write_data);
                                if (res)
                                {
                                    PRINTF("ERR: 0x%x\n", res);
                                    goto nfc_exit;
                                }
                                else
                                {
                                    PRINTF("write ok\n");
                                }
                            }
                            PRINTF("2nd read:\n");
                            for (uint8_t i = 0; i < 3; i++)
                            {
                                res = PcdRead(i + 4 * l);
                                if (res)
                                {
                                    PRINTF("ERR: 0x%x\n", res);
                                    goto nfc_exit;
                                }
                                PRINTF("block %02d: ", i + 4 * l);
                                for (uint8_t j = 0; j < 16; j++)
                                {
                                    PRINTF("%02x ", g_nfc_pcd_signal_data.decode_buf.v8[j]);
                                }
                                PRINTF("\n");
                            }
                            if(l == 15)
                            {
                                PRINT("TEST OK\r\n");
                            }
    #endif  /* ����д����� */
                        }
    #endif  /* ���ú������ԣ�����ֱ��HALT�˳� */
                        PcdHalt();
                    }
                }

            }
        }
nfc_exit:
        OPA_Cmd(NFC_OPA, DISABLE);         /* ����������������һ�ζ��������Ļ����رշŴ��� */
        nfc_signal_antenna_off();       /* �ر����� */
        changeHSETo32Mhz();
        TIMx(MAIN_PWM_TIM)->ATRLR = 6;                /* �ָ�13.714Mhz���е͹��ļ쿨 */
    }
    else
    {
        nfc_signal_antenna_off();       /* �ر����� */
    }
}

/*
 * change HSE to 27.12Mhz, and set SYSCLOCK to 108.48Mhz
 */
void changeHSETo27_12Mhz(void)
{
    __IO uint32_t StartUpCounter = 0, HSEStatus = 0;

#if(DEBUG)
    PRINT("27.12 s\n");
#if(DEBUG == DEBUG_UART1)
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
#elif(DEBUG == DEBUG_UART2)
    while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);
#elif(DEBUG == DEBUG_UART3)
    while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET);
#endif
#endif

    RCC->CFGR0 &= (uint32_t)0xfffffffc;         /* sysclockΪhsi */
    RCC->CTLR &= (uint32_t)(~RCC_PLLON);        /* �ر�PLL */
    RCC->CTLR &= ((uint32_t)(~RCC_HSEON));      /* �ر�HSE */

    RCC->CFGR0 &= (uint32_t)0xF8FF0000;
    RCC->CTLR &= (uint32_t)0xFEF6FFFF;
    RCC->CTLR &= (uint32_t)0xFFFBFFFF;
    RCC->CFGR0 &= (uint32_t)0xFF80FFFF;

    /* �ȴ�HSE״̬λ���㣬�ֲ�����ʾ��Ҫ6��HSE������0 */
    __asm volatile ("nop");
    __asm volatile ("nop");
    __asm volatile ("nop");
    __asm volatile ("nop");
    __asm volatile ("nop");
    __asm volatile ("nop");

    GPIO_SetBits(GPIOC, GPIO_Pin_13);

    RCC->CTLR |= ((uint32_t)RCC_HSEON);

    /* Wait till HSE is ready and if Time out is reached exit */
    do
    {
      HSEStatus = RCC->CTLR & RCC_HSERDY;
      StartUpCounter++;
    } while((HSEStatus == 0) && (StartUpCounter != HSE_STARTUP_TIMEOUT));

    if ((RCC->CTLR & RCC_HSERDY) != RESET)
    {
      HSEStatus = (uint32_t)0x01;
    }
    else
    {
      HSEStatus = (uint32_t)0x00;
    }

    if (HSEStatus == (uint32_t)0x01)
    {
      /* HCLK = SYSCLK */
      RCC->CFGR0 |= (uint32_t)RCC_HPRE_DIV2;
      /* PCLK2 = HCLK */
      RCC->CFGR0 |= (uint32_t)RCC_PPRE2_DIV1;
      /* PCLK1 = HCLK */
      RCC->CFGR0 |= (uint32_t)RCC_PPRE1_DIV2;

      /*  PLL configuration: PLLCLK = HSE * 15 = 120 MHz */
      RCC->CFGR0 &= (uint32_t)((uint32_t)~(RCC_PLLSRC | RCC_PLLXTPRE |
                                          RCC_PLLMULL |((uint32_t)(3 << 22))));

      RCC->CFGR0 |= (uint32_t)(RCC_PLLSRC_HSE | RCC_PLLXTPRE_HSE | RCC_PLLMULL16 | ((uint32_t)(3 << 22)));

      /* Enable PLL */
      RCC->CTLR |= RCC_PLLON;
      /* Wait till PLL is ready */
      while((RCC->CTLR & RCC_PLLRDY) == 0)
      {
      }
      /* Select PLL as system clock source */
      RCC->CFGR0 &= (uint32_t)((uint32_t)~(RCC_SW));
      RCC->CFGR0 |= (uint32_t)RCC_SW_PLL;
      /* Wait till PLL is used as system clock source */
      while ((RCC->CFGR0 & (uint32_t)RCC_SWS) != (uint32_t)0x08)
      {
      }
    }
    else
    {
        PRINT("fail\n");
          /*
           * If HSE fails to start-up, the application will have wrong clock
       * configuration. User can add here some code to deal with this error
           */
    }

#if(DEBUG)
    USART_Printf_Init(135929);
    PRINT("27.12 e\n");
#endif

}

/*
 * change HSE to 32Mhz, and set SYSCLOCK to 96Mhz
 */
void changeHSETo32Mhz(void)
{
    __IO uint32_t StartUpCounter = 0, HSEStatus = 0;

#if(DEBUG)
    PRINT("32 s\n");
#if(DEBUG == DEBUG_UART1)
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
#elif(DEBUG == DEBUG_UART2)
    while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);
#elif(DEBUG == DEBUG_UART3)
    while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET);
#endif
#endif

    RCC->CFGR0 &= (uint32_t)(~(RCC_SW));        /* sysclockΪhsi */
    RCC->CTLR &= (uint32_t)(~RCC_PLLON);        /* �ر�PLL */
    RCC->CTLR &= ((uint32_t)(~RCC_HSEON));      /* �ر�HSE */

    RCC->CFGR0 &= (uint32_t)0xF8FF0000;
    RCC->CTLR &= (uint32_t)0xFEF6FFFF;
    RCC->CTLR &= (uint32_t)0xFFFBFFFF;
    RCC->CFGR0 &= (uint32_t)0xFF80FFFF;

    /* �ȴ�HSE״̬λ���㣬�ֲ�����ʾ��Ҫ6��HSE������0 */
    __asm volatile ("nop");
    __asm volatile ("nop");
    __asm volatile ("nop");
    __asm volatile ("nop");
    __asm volatile ("nop");
    __asm volatile ("nop");

    GPIO_ResetBits(GPIOC, GPIO_Pin_13);

    RCC->CTLR |= ((uint32_t)RCC_HSEON);

    /* Wait till HSE is ready and if Time out is reached exit */
    do
    {
      HSEStatus = RCC->CTLR & RCC_HSERDY;
      StartUpCounter++;
    } while((HSEStatus == 0) && (StartUpCounter != HSE_STARTUP_TIMEOUT));

    if ((RCC->CTLR & RCC_HSERDY) != RESET)
    {
      HSEStatus = (uint32_t)0x01;
    }
    else
    {
      HSEStatus = (uint32_t)0x00;
    }

    if (HSEStatus == (uint32_t)0x01)
    {
        /* HCLK = SYSCLK */
        RCC->CFGR0 |= (uint32_t)RCC_HPRE_DIV1;
        /* PCLK2 = HCLK */
        RCC->CFGR0 |= (uint32_t)RCC_PPRE2_DIV1;
        /* PCLK1 = HCLK */
        RCC->CFGR0 |= (uint32_t)RCC_PPRE1_DIV2;

        /*  PLL configuration: PLLCLK = HSE * 12 = 96 MHz */
        RCC->CFGR0 &= (uint32_t)((uint32_t) ~(RCC_PLLSRC | RCC_PLLXTPRE |
                                              RCC_PLLMULL));

        RCC->CFGR0 |= (uint32_t)(RCC_PLLSRC_HSE | RCC_PLLXTPRE_HSE | RCC_PLLMULL12);

        /* Enable PLL */
        RCC->CTLR |= RCC_PLLON;
        /* Wait till PLL is ready */
        while ((RCC->CTLR & RCC_PLLRDY) == 0)
        {
        }
        /* Select PLL as system clock source */
        RCC->CFGR0 &= (uint32_t)((uint32_t) ~(RCC_SW));
        RCC->CFGR0 |= (uint32_t)RCC_SW_PLL;
        /* Wait till PLL is used as system clock source */
        while ((RCC->CFGR0 & (uint32_t)RCC_SWS) != (uint32_t)0x08)
        {
        }
    }
    else
    {
          /*
           * If HSE fails to start-up, the application will have wrong clock
       * configuration. User can add here some code to deal with this error
           */
        PRINT("fail\n");
    }

#if(DEBUG)
    USART_Printf_Init(115200);
    PRINT("32 e\n");
#endif
}
