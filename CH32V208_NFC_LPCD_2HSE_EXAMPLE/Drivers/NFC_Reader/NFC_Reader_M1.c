/********************************** (C) COPYRIGHT *******************************
 * File Name          : NFC_Reader_M1.c
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2023/05/08
 * Description        : Mifare Classic One卡操作库
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

#include "NFC_Reader_M1.h"
#include "wchble.h"

/* 每个文件单独debug打印的开关，置0可以禁止本文件内部打印 */
#define DEBUG_PRINT_IN_THIS_FILE 0
#if DEBUG_PRINT_IN_THIS_FILE
    #define PRINTF(...) PRINT(__VA_ARGS__)
#else
    #define PRINTF(...) do {} while (0)
#endif

/* The overtime are set according to the MIFARE Classic (1K) Datasheet. */
#define PCD_REQUEST_OVER_TIME           5
#define PCD_ANTICOLL_OVER_TIME          10
#define PCD_SELECT_OVER_TIME            10
#define PCD_AUTH_OVER_TIME              5
#define PCD_READ_OVER_TIME              5
#define PCD_WRITE_STEP1_OVER_TIME       5
#define PCD_WRITE_STEP2_OVER_TIME       10
#define PCD_VALUE_STEP1_2_OVER_TIME     5
#define PCD_VALUE_STEP3_OVER_TIME       10

/**
 * @brief   Find a card.
 *
 * @param   req_code - 0x52/0x26.
 *          0x52 = find all 14443A-compliant cards in the sensing area.
 *          0x26 = find all 14443A-compliant cards which is not in halt mode in the sensing area.
 *
 * @return  0 = No Card.<BR>
 *          0x0004 = Mifare_One(S50).<BR>
 *          0x0002 = Mifare_One(S70).<BR>
 *          0x0044 = Mifare Ultralight.<BR>
*/
uint16_t PcdRequest(uint8_t req_code)
{
    uint16_t res;

    g_nfc_pcd_signal_data.state = NFC_PCD_REC_STATE_WAITING;

    g_nfc_pcd_signal_data.wait_level = NFC_CALC_WAIT_LEVEL(PCD_REQUEST_OVER_TIME);
    nfc_pcd_send_cmd(req_code);
    while (1)
    {
        if (g_nfc_pcd_signal_data.state == NFC_PCD_REC_STATE_OK)
        {
            break;
        }
    }
    g_nfc_pcd_signal_data.decode_state = NFC_PCD_DECODE_STATE_FREE;

    res = nfc_pcd_decode_data(&g_nfc_pcd_signal_data);

    if ((g_nfc_pcd_signal_data.decode_state == NFC_PCD_DECODE_STATE_OK) && (res == 16))
    {
        PRINTF("ATQA:0x%04x\r\n", g_nfc_pcd_signal_data.decode_buf.v16[0]);
        return g_nfc_pcd_signal_data.decode_buf.v16[0];
    }
    else
    {
        PRINTF("no Card\n");
    }

end:
    return 0;
}

/**
 * @brief   Perform an anti-collision session.
 *
 * @param   cmd - PICC_ANTICOLL1/PICC_ANTICOLL2/PICC_ANTICOLL3.
 *
 * @return  PCD_NO_ERROR = success.<BR>
 *          others = fail.<BR>
*/
uint16_t PcdAnticoll(uint8_t cmd)
{
    uint16_t res;

    g_nfc_pcd_signal_data.state = NFC_PCD_REC_STATE_WAITING;

    g_nfc_pcd_signal_data.wait_level = NFC_CALC_WAIT_LEVEL(PCD_ANTICOLL_OVER_TIME);
    g_nfc_pcd_signal_data.decode_buf.v8[0] = cmd;
    g_nfc_pcd_signal_data.decode_buf.v8[1] = 0x20;

    nfc_pcd_send_data((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, 2);
    while (1)
    {
        if (g_nfc_pcd_signal_data.state == NFC_PCD_REC_STATE_OK)
        {
            break;
        }
    }

    g_nfc_pcd_signal_data.decode_state = NFC_PCD_DECODE_STATE_FREE;

    res = nfc_pcd_decode_data(&g_nfc_pcd_signal_data);

    if ((g_nfc_pcd_signal_data.decode_state == NFC_PCD_DECODE_STATE_OK) && (res == 40))
    {
        if (ISO14443A_CHECK_BCC(g_nfc_pcd_signal_data.decode_buf.v8))
        {
            res = PCD_NO_ERROR;
        }
        else
        {
            res = PCD_BCC_ERROR;
            PRINTF("check bcc error\n");
        }
    }

end:
    return res;
}

/**
 * @brief   select a card.
 *
 * @param   pSnr - 4 bytes card UUID.
 *
 * @return  PCD_NO_ERROR = success.<BR>
 *          others = fail.<BR>
*/
uint16_t PcdSelect(uint8_t cmd, uint8_t *pSnr)
{
    uint16_t res;

    g_nfc_pcd_signal_data.state = NFC_PCD_REC_STATE_WAITING;

    g_nfc_pcd_signal_data.wait_level = NFC_CALC_WAIT_LEVEL(PCD_SELECT_OVER_TIME);
    g_nfc_pcd_signal_data.decode_buf.v8[0] = cmd;
    g_nfc_pcd_signal_data.decode_buf.v8[1] = 0x70;
    g_nfc_pcd_signal_data.decode_buf.v8[6] = 0;

    for (res = 0; res < 4; res++)
    {
        g_nfc_pcd_signal_data.decode_buf.v8[res + 2] = *(pSnr + res);
        g_nfc_pcd_signal_data.decode_buf.v8[6] ^= *(pSnr + res);
    }
    ISO14443AAppendCRCA((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, 7);

    nfc_pcd_send_data((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, 9);

    while (1)
    {
        if (g_nfc_pcd_signal_data.state == NFC_PCD_REC_STATE_OK)
        {
            break;
        }
    }

    g_nfc_pcd_signal_data.decode_state = NFC_PCD_DECODE_STATE_FREE;

    res = nfc_pcd_decode_data(&g_nfc_pcd_signal_data);

    if ((g_nfc_pcd_signal_data.decode_state == NFC_PCD_DECODE_STATE_OK) && (res == 24))
    {
        if (ISO14443_CRCA((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, 3) == 0)
        {
            res = PCD_NO_ERROR;
        }
        else
        {
            res = PCD_CRC_ERROR;
            PRINTF("check crc error\n");
        }
    }

end:
    return res;
}

/**
 * @brief   decrement or increment a value on the value block.
 *
 * @param   auth_mode - 0x60 = authenticate A key, 0x61 = authenticate B key.
 * @param   addr - the addr of the value block.
 * @param   pValue - 4 bytes value(Little-Endian), for decrement or increment.
 *
 * @return  PCD_NO_ERROR = success.<BR>
 *          others = fail.<BR>
*/
uint16_t PcdAuthState(uint8_t auth_mode, uint8_t addr, uint8_t *pKey, uint8_t *pUid)
{
    uint16_t res;
    uint8_t tag_clg[4];
    uint8_t reader_clg[4] = {0};
    uint8_t reader_rsp[4] = {0};
    uint8_t tag_rsp[4] = {0};
    uint8_t oddparity[12] = {0};

step1:
    g_nfc_pcd_signal_data.state = NFC_PCD_REC_STATE_WAITING;

    g_nfc_pcd_signal_data.wait_level = NFC_CALC_WAIT_LEVEL(PCD_AUTH_OVER_TIME);
    g_nfc_pcd_signal_data.decode_buf.v8[0] = auth_mode;
    g_nfc_pcd_signal_data.decode_buf.v8[1] = addr;

    ISO14443AAppendCRCA((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, 2);

    if (g_nfc_pcd_signal_data.is_encrypted != 0)
    {
        nfc_pcd_cypto_encrypt((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, (uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, oddparity, 4 * 8);
        nfc_pcd_send_bits((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, 4, oddparity);
        g_nfc_pcd_signal_data.decode_state = NFC_PCD_DECODE_STATE_OWN;
    }
    else
    {
        nfc_pcd_send_data((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, 4);
        g_nfc_pcd_signal_data.decode_state = NFC_PCD_DECODE_STATE_FREE;
    }

    while (1)
    {
        if (g_nfc_pcd_signal_data.state == NFC_PCD_REC_STATE_OK)
        {
            break;
        }
    }

    res = nfc_pcd_decode_data(&g_nfc_pcd_signal_data);
    tmos_memcpy(tag_clg, (const void *)g_nfc_pcd_signal_data.decode_buf.v8, 4);

    nfc_pcd_cypto_create(pKey, pUid, tag_clg, reader_clg, reader_rsp, tag_rsp, oddparity, g_nfc_pcd_signal_data.is_encrypted);

    tmos_memcpy((void *)g_nfc_pcd_signal_data.decode_buf.v8, (const void *)reader_clg, 4);
    tmos_memcpy((void *)&g_nfc_pcd_signal_data.decode_buf.v8[4], (const void *)reader_rsp, 4);

step2:
    g_nfc_pcd_signal_data.state = NFC_PCD_REC_STATE_WAITING;
    nfc_pcd_send_bits((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, 8, oddparity);
    while (1)
    {
        if (g_nfc_pcd_signal_data.state == NFC_PCD_REC_STATE_OK)
        {
            break;
        }
    }

    g_nfc_pcd_signal_data.decode_state = NFC_PCD_DECODE_STATE_OWN;

    res = nfc_pcd_decode_data(&g_nfc_pcd_signal_data);
    if (tmos_memcmp((const void *)tag_rsp, (const void *)g_nfc_pcd_signal_data.decode_buf.v8, 4) == TRUE)
    {
        res = PCD_NO_ERROR;
    }
    else
    {
        res = PCD_AUTH_ERROR;
        PRINTF("Auth err, addr: %02d\n", addr);
    }

end:
    return res;
}

/**
 * @brief   Read data from the given block.
 *
 * @param   addr - the addr of the block.
 *
 * @return  PCD_NO_ERROR = success.<BR>
 *          others = fail.<BR>
*/
uint16_t PcdRead(uint8_t addr)
{
    uint16_t res;
    uint8_t oddparity[4] = {0};

    g_nfc_pcd_signal_data.state = NFC_PCD_REC_STATE_WAITING;

    g_nfc_pcd_signal_data.wait_level = NFC_CALC_WAIT_LEVEL(PCD_READ_OVER_TIME);
    g_nfc_pcd_signal_data.decode_buf.v8[0] = PICC_READ;
    g_nfc_pcd_signal_data.decode_buf.v8[1] = addr;

    ISO14443AAppendCRCA((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, 2);

    nfc_pcd_cypto_encrypt((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, (uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, oddparity, 4 * 8);
    nfc_pcd_send_bits((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, 4, oddparity);
    while (1)
    {
        if (g_nfc_pcd_signal_data.state == NFC_PCD_REC_STATE_OK)
        {
            break;
        }
    }

    g_nfc_pcd_signal_data.decode_state = NFC_PCD_DECODE_STATE_OWN;

    res = nfc_pcd_decode_data(&g_nfc_pcd_signal_data);

    if (nfc_pcd_cypto_decrypt((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, (uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, (uint8_t *)g_nfc_pcd_signal_data.buf, res))
    {
        PRINTF("nfc_pcd_cypto_decrypt fail len: %d\n", res);
        res = PCD_DECRYPT_ERROR;
    }
    else
    {
        if (res == 18 * 8)
        {
            res = PCD_NO_ERROR;
        }
        else if (res == ACK_NAK_FRAME_SIZE)
        {
            res = PICC_NAK_HEAD | g_nfc_pcd_signal_data.decode_buf.v8[0];
        }
        else
        {
            res = PCD_FRAME_ERROR;
        }
    }

end:
    return res;
}

/**
 * @brief   Writes data to the given block.
 *
 * @param   addr - the addr of the block.
 * @param   pData - 16 bytes data need to write.
 *
 * @return  PCD_NO_ERROR = success.<BR>
 *          others = fail.<BR>
*/
uint16_t PcdWrite(uint8_t addr, uint8_t *pData)
{
    uint16_t res;
    uint8_t oddparity[18] = {0};

step1:
    g_nfc_pcd_signal_data.state = NFC_PCD_REC_STATE_WAITING;

    g_nfc_pcd_signal_data.wait_level = NFC_CALC_WAIT_LEVEL(PCD_WRITE_STEP1_OVER_TIME);
    g_nfc_pcd_signal_data.decode_buf.v8[0] = PICC_WRITE;
    g_nfc_pcd_signal_data.decode_buf.v8[1] = addr;

    ISO14443AAppendCRCA((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, 2);
    nfc_pcd_cypto_encrypt((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, (uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, oddparity, 4 * 8);
    nfc_pcd_send_bits((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, 4, oddparity);

    while (1)
    {
        if (g_nfc_pcd_signal_data.state == NFC_PCD_REC_STATE_OK)
        {
            break;
        }
    }

    g_nfc_pcd_signal_data.decode_state = NFC_PCD_DECODE_STATE_OWN;

    res = nfc_pcd_decode_data(&g_nfc_pcd_signal_data);

    if (nfc_pcd_cypto_decrypt((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, (uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, (uint8_t *)g_nfc_pcd_signal_data.buf, res))
    {
        PRINTF("nfc_pcd_cypto_decrypt fail len: %d\n", res);
        res = PCD_DECRYPT_ERROR;
        goto end;
    }

    if (res == ACK_NAK_FRAME_SIZE)
    {
        if (g_nfc_pcd_signal_data.decode_buf.v8[0] != ACK_VALUE)
        {
            res = PICC_NAK_HEAD | g_nfc_pcd_signal_data.decode_buf.v8[0];
            goto end;
        }
    }
    else
    {
        res = PCD_FRAME_ERROR;
        goto end;
    }

step2:
    g_nfc_pcd_signal_data.state = NFC_PCD_REC_STATE_WAITING;

    g_nfc_pcd_signal_data.wait_level = NFC_CALC_WAIT_LEVEL(PCD_WRITE_STEP2_OVER_TIME);

    tmos_memcpy((void *)g_nfc_pcd_signal_data.decode_buf.v8, (const void *)pData, 16);

    ISO14443AAppendCRCA((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, 16);

    nfc_pcd_cypto_encrypt((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, (uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, oddparity, 18 * 8);

    nfc_pcd_send_bits((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, 18, oddparity);

    while (1)
    {
        if (g_nfc_pcd_signal_data.state == NFC_PCD_REC_STATE_OK)
        {
            break;
        }
    }

    g_nfc_pcd_signal_data.decode_state = NFC_PCD_DECODE_STATE_OWN;

    res = nfc_pcd_decode_data(&g_nfc_pcd_signal_data);

    if (nfc_pcd_cypto_decrypt((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8,
                      (uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8,
                      (uint8_t *)g_nfc_pcd_signal_data.buf, res))
    {
        res = PCD_DECRYPT_ERROR;
        goto end;
    }

    if (res == ACK_NAK_FRAME_SIZE)
    {
        if (g_nfc_pcd_signal_data.decode_buf.v8[0] == ACK_VALUE)
        {
            res = PCD_NO_ERROR;
        }
        else
        {
            res = PICC_NAK_HEAD | g_nfc_pcd_signal_data.decode_buf.v8[0];
        }
    }
    else
    {
        res = PCD_FRAME_ERROR;
    }

end:
    return res;
}

/**
 * @brief   decrement or increment a value on the value block.
 *
 * @param   dd_mode - 0xC0 = decrement, 0xC1 = increment.
 * @param   addr - the addr of the value block.
 * @param   pValue - 4 bytes value(Little-Endian), for decrement or increment.
 *
 * @return  PCD_NO_ERROR = success.<BR>
 *          others = fail.<BR>
*/
uint16_t PcdValue(uint8_t dd_mode, uint8_t addr, uint8_t *pValue)
{
    uint16_t res;
    uint8_t oddparity[18] = {0};

step1:
    g_nfc_pcd_signal_data.state = NFC_PCD_REC_STATE_WAITING;

    g_nfc_pcd_signal_data.wait_level = NFC_CALC_WAIT_LEVEL(PCD_VALUE_STEP1_2_OVER_TIME);
    g_nfc_pcd_signal_data.decode_buf.v8[0] = dd_mode;
    g_nfc_pcd_signal_data.decode_buf.v8[1] = addr;

    ISO14443AAppendCRCA((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, 2);

    nfc_pcd_cypto_encrypt((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, (uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, oddparity, 4 * 8);
    nfc_pcd_send_bits((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, 4, oddparity);

    while (1)
    {
        if (g_nfc_pcd_signal_data.state == NFC_PCD_REC_STATE_OK)
        {
            break;
        }
    }

    g_nfc_pcd_signal_data.decode_state = NFC_PCD_DECODE_STATE_OWN;

    res = nfc_pcd_decode_data(&g_nfc_pcd_signal_data);

    if (nfc_pcd_cypto_decrypt((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, (uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, (uint8_t *)g_nfc_pcd_signal_data.buf, res))
    {
        PRINTF("nfc_pcd_cypto_decrypt fail len: %d\n", res);
        res = PCD_DECRYPT_ERROR;
        goto end;
    }

    if (res == ACK_NAK_FRAME_SIZE)
    {
        if (g_nfc_pcd_signal_data.decode_buf.v8[0] != ACK_VALUE)
        {
            res = PICC_NAK_HEAD | g_nfc_pcd_signal_data.decode_buf.v8[0];
            goto end;
        }
    }
    else
    {
        res = PCD_FRAME_ERROR;
        goto end;
    }

step2:
    g_nfc_pcd_signal_data.state = NFC_PCD_REC_STATE_WAITING;

    tmos_memcpy((void *)g_nfc_pcd_signal_data.decode_buf.v8, (const void *)pValue, 4);

    ISO14443AAppendCRCA((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, 4);

    nfc_pcd_cypto_encrypt((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, (uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, oddparity, 6 * 8);

    nfc_pcd_send_bits((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, 6, oddparity);

    while (1)
    {
        if (g_nfc_pcd_signal_data.state == NFC_PCD_REC_STATE_OK)
        {
            break;
        }
    }

step3:
    g_nfc_pcd_signal_data.state = NFC_PCD_REC_STATE_WAITING;
    g_nfc_pcd_signal_data.wait_level = NFC_CALC_WAIT_LEVEL(PCD_VALUE_STEP3_OVER_TIME);
    g_nfc_pcd_signal_data.decode_buf.v8[0] = PICC_TRANSFER;
    g_nfc_pcd_signal_data.decode_buf.v8[1] = addr;

    ISO14443AAppendCRCA((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, 2);

    nfc_pcd_cypto_encrypt((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, (uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, oddparity, 4 * 8);
    nfc_pcd_send_bits((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, 4, oddparity);

    while (1)
    {
        if (g_nfc_pcd_signal_data.state == NFC_PCD_REC_STATE_OK)
        {
            break;
        }
    }

    g_nfc_pcd_signal_data.decode_state = NFC_PCD_DECODE_STATE_OWN;

    res = nfc_pcd_decode_data(&g_nfc_pcd_signal_data);

    if (nfc_pcd_cypto_decrypt((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, (uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, (uint8_t *)g_nfc_pcd_signal_data.buf, res))
    {
        PRINTF("nfc_pcd_cypto_decrypt fail len: %d\n", res);
        res = PCD_DECRYPT_ERROR;
        goto end;
    }

    if (res == ACK_NAK_FRAME_SIZE)
    {
        if (g_nfc_pcd_signal_data.decode_buf.v8[0] == ACK_VALUE)
        {
            res = PCD_NO_ERROR;
        }
        else
        {
            res = PICC_NAK_HEAD | g_nfc_pcd_signal_data.decode_buf.v8[0];
        }
    }
    else
    {
        res = PCD_FRAME_ERROR;
    }

end:
    return res;
}

/**
 * @brief   backup a value block to another block.
 *
 * @param   sourceaddr - the addr of the value block need to be backup.
 * @param   goaladdr - the addr of a value block need backup to.
 *
 * @return  PCD_NO_ERROR = success.<BR>
 *          others = fail.<BR>
*/
uint16_t PcdBakValue(uint8_t sourceaddr, uint8_t goaladdr)
{
    uint16_t res;
    uint8_t oddparity[18] = {0};

step1:
    g_nfc_pcd_signal_data.state = NFC_PCD_REC_STATE_WAITING;

    g_nfc_pcd_signal_data.wait_level = NFC_CALC_WAIT_LEVEL(PCD_VALUE_STEP1_2_OVER_TIME);
    g_nfc_pcd_signal_data.decode_buf.v8[0] = PICC_RESTORE;
    g_nfc_pcd_signal_data.decode_buf.v8[1] = sourceaddr;

    ISO14443AAppendCRCA((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, 2);

    nfc_pcd_cypto_encrypt((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, (uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, oddparity, 4 * 8);
    nfc_pcd_send_bits((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, 4, oddparity);

    while (1)
    {
        if (g_nfc_pcd_signal_data.state == NFC_PCD_REC_STATE_OK)
        {
            break;
        }
    }

    g_nfc_pcd_signal_data.decode_state = NFC_PCD_DECODE_STATE_OWN;

    res = nfc_pcd_decode_data(&g_nfc_pcd_signal_data);

    if (nfc_pcd_cypto_decrypt((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, (uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, (uint8_t *)g_nfc_pcd_signal_data.buf, res))
    {
        PRINTF("nfc_pcd_cypto_decrypt fail len: %d\n", res);
        res = PCD_DECRYPT_ERROR;
        goto end;
    }

    if (res == ACK_NAK_FRAME_SIZE)
    {
        if (g_nfc_pcd_signal_data.decode_buf.v8[0] != ACK_VALUE)
        {
            res = PICC_NAK_HEAD | g_nfc_pcd_signal_data.decode_buf.v8[0];
            goto end;
        }
    }
    else
    {
        res = PCD_FRAME_ERROR;
        goto end;
    }

step2:
    g_nfc_pcd_signal_data.state = NFC_PCD_REC_STATE_WAITING;

    g_nfc_pcd_signal_data.decode_buf.v32[0] = 0;

    ISO14443AAppendCRCA((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, 4);

    nfc_pcd_cypto_encrypt((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, (uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, oddparity, 6 * 8);

    nfc_pcd_send_bits((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, 6, oddparity);

    while (1)
    {
        if (g_nfc_pcd_signal_data.state == NFC_PCD_REC_STATE_OK)
        {
            break;
        }
    }

step3:
    g_nfc_pcd_signal_data.state = NFC_PCD_REC_STATE_WAITING;
    g_nfc_pcd_signal_data.wait_level = NFC_CALC_WAIT_LEVEL(PCD_VALUE_STEP3_OVER_TIME);
    g_nfc_pcd_signal_data.decode_buf.v8[0] = PICC_TRANSFER;
    g_nfc_pcd_signal_data.decode_buf.v8[1] = goaladdr;

    ISO14443AAppendCRCA((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, 2);

    nfc_pcd_cypto_encrypt((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, (uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, oddparity, 4 * 8);
    nfc_pcd_send_bits((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, 4, oddparity);

    while (1)
    {
        if (g_nfc_pcd_signal_data.state == NFC_PCD_REC_STATE_OK)
        {
            break;
        }
    }

    g_nfc_pcd_signal_data.decode_state = NFC_PCD_DECODE_STATE_OWN;

    res = nfc_pcd_decode_data(&g_nfc_pcd_signal_data);

    if (nfc_pcd_cypto_decrypt((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, (uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, (uint8_t *)g_nfc_pcd_signal_data.buf, res))
    {
        PRINTF("nfc_pcd_cypto_decrypt fail len: %d\n", res);
        res = PCD_DECRYPT_ERROR;
        goto end;
    }

    if (res == ACK_NAK_FRAME_SIZE)
    {
        if (g_nfc_pcd_signal_data.decode_buf.v8[0] == ACK_VALUE)
        {
            res = PCD_NO_ERROR;
        }
        else
        {
            res = PICC_NAK_HEAD | g_nfc_pcd_signal_data.decode_buf.v8[0];
        }
    }
    else
    {
        res = PCD_FRAME_ERROR;
    }

end:
    return res;
}

/**
 * @brief   format a block as a value block which can use decrement or increment cmd.
 *
 * @param   addr - the addr of the data block, which need to be format as a value block.
 * @param   value_from - the pointer to a 4 byte memory (Little-Endian) which need to be saved.
 * @param   adr - the adr of the value block.
 *
 * @return  PCD_NO_ERROR = success.<BR>
 *          others = fail.<BR>
*/
uint16_t PcdInitValueBlock(uint8_t addr, uint8_t *value_from, uint8_t adr)
{
    uint16_t res;
    uint8_t init_buffer[16] = {0};

    for (res = 0; res < 4; res++)
    {
        init_buffer[res] = value_from[res];
        init_buffer[res + 8] = value_from[res];
        init_buffer[res + 4] = ~value_from[res];
    }

    init_buffer[12] = adr;
    init_buffer[13] = ~adr;
    init_buffer[14] = adr;
    init_buffer[15] = ~adr;

    res = PcdWrite(addr, init_buffer);

end:
    return res;
}

/**
 * @brief   read value from a value block,
 *          g_nfc_pcd_signal_data.decode_buf.v32[0] is the value in the value block,
 *          g_nfc_pcd_signal_data.decode_buf.v8[12] is the adr in the value block.
 *
 * @param   addr - the addr of the data block, which need to be format as a value block.
 *
 * @return  PCD_NO_ERROR = success.<BR>
 *          others = fail.<BR>
*/
uint16_t PcdReadValueBlock(uint8_t addr)
{
    uint16_t res;

    res = PcdRead(addr);

    if (res == PCD_NO_ERROR)
    {
        if ((g_nfc_pcd_signal_data.decode_buf.v32[0] != g_nfc_pcd_signal_data.decode_buf.v32[2])
                || ((g_nfc_pcd_signal_data.decode_buf.v32[0] != (~g_nfc_pcd_signal_data.decode_buf.v32[1])))
                || (g_nfc_pcd_signal_data.decode_buf.v8[12] != g_nfc_pcd_signal_data.decode_buf.v8[14])
                || (g_nfc_pcd_signal_data.decode_buf.v8[13] != g_nfc_pcd_signal_data.decode_buf.v8[15])
                || ((g_nfc_pcd_signal_data.decode_buf.v8[12] != (uint8_t)(~g_nfc_pcd_signal_data.decode_buf.v8[13])))
           )
        {
            res = PCD_VALUE_BLOCK_INVALID;
        }
    }

end:
    return res;
}

/**
 * @brief   set card to halt mode.
 *
 * @param   None.
 *
 * @return  None.
*/
void PcdHalt(void)
{
    g_nfc_pcd_signal_data.state = NFC_PCD_REC_STATE_FREE;

    g_nfc_pcd_signal_data.decode_buf.v8[0] = PICC_HALT;
    g_nfc_pcd_signal_data.decode_buf.v8[1] = 0;

    ISO14443AAppendCRCA((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, 2);

    nfc_pcd_send_data((uint8_t *)g_nfc_pcd_signal_data.decode_buf.v8, 4);

    while (1)
    {
        if (g_nfc_pcd_signal_data.state == NFC_PCD_REC_STATE_OK)
        {
            break;
        }
    }
}
