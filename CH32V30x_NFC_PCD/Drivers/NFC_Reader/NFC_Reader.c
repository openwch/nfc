/********************************** (C) COPYRIGHT *******************************
 * File Name          : NFC_Reader.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2022/12/26
 * Description        : NFC M1卡操作例程
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

#include "NFC_Reader.h"
#include "NFC_Reader_M1.h"

/* 每个文件单独debug打印的开关，置0可以禁止本文件内部打印 */
#define DEBUG_PRINT_IN_THIS_FILE 1
#if DEBUG_PRINT_IN_THIS_FILE
    #define PRINTF(...) printf_(__VA_ARGS__)
#else
    #define PRINTF(...) do {} while (0)
#endif

uint8_t write_data[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
uint8_t picc_uuid[10];
uint8_t picc_uuid_len;
uint8_t default_key[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void nfc_signal_bsp_init(void)
{
    NFC_PWMOutGPIO_Init();
    NFC_PWMOut_Init(9, 0, 5);

    NFC_REC_NVIC_GPIO_Init();
    NFC_CTRL_REC_Prepare_Init();
    NFC_DMA_Prepare();              /* DMA发送波形初始化 */

    NFC_OPA_Init();

    while (1)
    {

        /* 发送 */
        nfc_signal_antenna_on();            /* 启动天线 */
        OPA_Cmd(NFC_OPA, ENABLE);           /* 打开放大器 */

        Delay_Ms(1);                        /* 每次启动天线到发射数据之间应至少有1ms的间隔，卡需要先充电才能保证后续操作 */

        uint16_t res;

        res = PcdRequest(PICC_REQALL);

        if ((res == 0x0004) || (res == 0x0002) || (res == 0x0044))      /* 普通m1卡4字节卡号，Mifare Ultralight为7字节卡号 */
        {
            res = PcdAnticoll(PICC_ANTICOLL1);
            if (res == PCD_NO_ERROR)
            {
                memcpy(picc_uuid, (const void *)g_nfc_pcd_signal_data.decode_buf.v8, 4);
                PRINTF("ANTICOLL1:%02x %02x %02x %02x\n", g_nfc_pcd_signal_data.decode_buf.v8[0], g_nfc_pcd_signal_data.decode_buf.v8[1], g_nfc_pcd_signal_data.decode_buf.v8[2], g_nfc_pcd_signal_data.decode_buf.v8[3]);
                res = PcdSelect(PICC_ANTICOLL1, picc_uuid);
                if (res == PCD_NO_ERROR)
                {
                    if(picc_uuid[0] == 0x88)
                    {
                        /* 非4字节nfc卡，还有下一层UUID需要读取 */
                        picc_uuid[0] = picc_uuid[1];
                        picc_uuid[1] = picc_uuid[2];
                        picc_uuid[2] = picc_uuid[3];

                        res = PcdAnticoll(PICC_ANTICOLL2);
                        if (res == PCD_NO_ERROR)
                        {
                            memcpy(picc_uuid + 3, (const void *)g_nfc_pcd_signal_data.decode_buf.v8, 4);
                            PRINTF("ANTICOLL2:%02x %02x %02x %02x\n", g_nfc_pcd_signal_data.decode_buf.v8[0], g_nfc_pcd_signal_data.decode_buf.v8[1], g_nfc_pcd_signal_data.decode_buf.v8[2], g_nfc_pcd_signal_data.decode_buf.v8[3]);
                            res = PcdSelect(PICC_ANTICOLL2, picc_uuid + 3);
                            if (res == PCD_NO_ERROR)
                            {
                                if(picc_uuid[3] == 0x88)
                                {
                                    /* 非7字节nfc卡，还有下一层UUID需要读取 */
                                    picc_uuid[3] = picc_uuid[4];
                                    picc_uuid[4] = picc_uuid[5];
                                    picc_uuid[5] = picc_uuid[6];

                                    res = PcdAnticoll(PICC_ANTICOLL3);
                                    if (res == PCD_NO_ERROR)
                                    {
                                        memcpy(picc_uuid + 6, (const void *)g_nfc_pcd_signal_data.decode_buf.v8, 4);
                                        PRINTF("ANTICOLL3:%02x %02x %02x %02x\n", g_nfc_pcd_signal_data.decode_buf.v8[0], g_nfc_pcd_signal_data.decode_buf.v8[1], g_nfc_pcd_signal_data.decode_buf.v8[2], g_nfc_pcd_signal_data.decode_buf.v8[3]);
                                        res = PcdSelect(PICC_ANTICOLL3, picc_uuid + 6);
                                        if (res == PCD_NO_ERROR)
                                        {
                                            /* 暂时没有更长的UUID */
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
                        goto request_next;
                    }

                    if(picc_uuid_len != 4)      /* Mifare Ultralight协议暂不支持后续操作，只可读卡号 */
                    {
                        goto request_next;
                    }

                    g_nfc_pcd_signal_data.is_encrypted = 0;     /* select成功后，清除加密位 */

#if 1   /* 启用后续测试，否则直接HALT退出 */

                    res = PcdAuthState(PICC_AUTHENT1A, 0, default_key, picc_uuid);
                    if (res == PCD_NO_ERROR)
                    {
                        g_nfc_pcd_signal_data.is_encrypted = 1;     /* 第一次认证成功后，置位加密位 */
                    }
                    else
                    {
                        goto request_next;
                    }

                    for (uint8_t i = 0; i < 4; i++)
                    {
                        res = PcdRead(i);
                        if (res != PCD_NO_ERROR)
                        {
                            PRINTF("ERR: 0x%x\n", res);
                            goto request_next;
                        }
                        PRINTF("block %02d: ", i);
                        for (uint8_t j = 0; j < 16; j++)
                        {
                            PRINTF("%02x ", g_nfc_pcd_signal_data.decode_buf.v8[j]);
                        }
                        PRINTF("\n");
                    }

#if 1   /* 值块读取和初始化测试 */

                    res = PcdReadValueBlock(1);
                    if (res == PCD_VALUE_BLOCK_INVALID)
                    {
                        PRINTF("not a value block, init it.");
                        uint32_t vdata = 100;
                        res = PcdInitValueBlock(1, (uint8_t *)&vdata, 2);
                        if (res != PCD_NO_ERROR)
                        {
                            PRINTF("ERR: 0x%x\n", res);
                            goto request_next;
                        }
                    }
                    else if (res != PCD_NO_ERROR)
                    {
                        PRINTF("ERR: 0x%x\n", res);
                        goto request_next;
                    }
                    else
                    {
                        PRINTF("value:%d, adr:%d\n", g_nfc_pcd_signal_data.decode_buf.v32[0], g_nfc_pcd_signal_data.decode_buf.v8[12]);
                    }

#endif  /* 值块读取和初始化测试 */

#if 1   /* 值块扣款和备份测试 */
                    PRINTF("PcdValue\n");
                    uint32_t di_data = 1;
                    res = PcdValue(PICC_DECREMENT, 1, (uint8_t *)&di_data);
                    if(res != PCD_NO_ERROR)
                    {
                        PRINTF("ERR: 0x%x\n",res);
                        goto request_next;
                    }
                    PRINTF("PcdBakValue\n");
                    res = PcdBakValue(1,2);
                    if(res != PCD_NO_ERROR)
                    {
                        PRINTF("ERR: 0x%x\n",res);
                        goto request_next;
                    }

#endif  /* 值块扣款和备份测试 */

                    for (uint8_t l = 1; l < 16; l++)
                    {
                        res = PcdAuthState(PICC_AUTHENT1A, 4 * l, default_key, picc_uuid);
                        if (res)
                        {
                            PRINTF("ERR: 0x%x\n", res);
                            goto request_next;
                        }

                        PRINTF("1st read:\n");
                        for (uint8_t i = 0; i < 3; i++)
                        {
                            res = PcdRead(i + 4 * l);
                            if (res)
                            {
                                PRINTF("ERR: 0x%x\n", res);
                                goto request_next;
                            }
                            PRINTF("block %02d: ", i + 4 * l);
                            for (uint8_t j = 0; j < 16; j++)
                            {
                                PRINTF("%02x ", g_nfc_pcd_signal_data.decode_buf.v8[j]);
                            }
                            PRINTF("\n");
                        }

#if 1   /* 扇区写入测试 */

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
                                goto request_next;
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
                                goto request_next;
                            }
                            PRINTF("block %02d: ", i + 4 * l);
                            for (uint8_t j = 0; j < 16; j++)
                            {
                                PRINTF("%02x ", g_nfc_pcd_signal_data.decode_buf.v8[j]);
                            }
                            PRINTF("\n");
                        }
#endif  /* 扇区写入测试 */
                    }
#endif  /* 启用后续测试，否则直接HALT退出 */
                    PcdHalt();
                }
            }
        }

request_next:
        nfc_signal_antenna_off();       /* 关闭天线  */
        OPA_Cmd(NFC_OPA, DISABLE);      /* 关闭放大器 */
        Delay_Ms(500);

    }
}
