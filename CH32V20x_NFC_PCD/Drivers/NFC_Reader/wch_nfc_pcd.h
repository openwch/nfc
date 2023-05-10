/********************************** (C) COPYRIGHT *******************************
 * File Name          : wch_nfc_pcd.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2023/01/12
 * Description        : NFC PCD head file for WCH chips.
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

#ifndef _WCH_NFC_PCD_H_
#define _WCH_NFC_PCD_H_

#include "debug.h"

#define NFC_OVER_TIME                   25              /* operation overtime, ms, default 25ms */
#define NFC_OVER_TIME_STEP              5               /* operation overtime step time */
#define NFC_CALC_WAIT_LEVEL(X)          ((NFC_OVER_TIME + NFC_OVER_TIME_STEP - 1 - X) / NFC_OVER_TIME_STEP)

typedef __attribute__((aligned(4))) struct _nfc_pcd_signal_data_struct
{
    uint8_t buf[312];
    union
    {
        uint32_t v32[5];
        uint16_t v16[10];
        uint8_t v8[20];
    } decode_buf;
    uint8_t state;
    uint8_t index;
    uint8_t decode_state;
    uint8_t decode_index;
    uint8_t wait_level;
    uint8_t is_encrypted;
} nfc_pcd_signal_data_t;

extern volatile nfc_pcd_signal_data_t g_nfc_pcd_signal_data;

enum
{
    NFC_PCD_REC_STATE_FREE = 0,
    NFC_PCD_REC_STATE_WAITING,
    NFC_PCD_REC_STATE_WORKING,
    NFC_PCD_REC_STATE_OK,
};

enum
{
    NFC_PCD_DECODE_STATE_FREE = 0,
    NFC_PCD_DECODE_STATE_OWN,
    NFC_PCD_DECODE_STATE_ERR,
    NFC_PCD_DECODE_STATE_OK,
};

typedef uint32_t (*nfc_pcd_cypto_rand_t)(void);

#define ISO14443A_CALC_BCC(ByteBuffer) (ByteBuffer[0] ^ ByteBuffer[1] ^ ByteBuffer[2] ^ ByteBuffer[3])

#define ISO14443A_CHECK_BCC(B) ((B[0] ^ B[1] ^ B[2] ^ B[3]) == B[4])

/*********************************************************************
 * @fn      nfc_pcd_send_cmd
 *
 * @brief   send 7-bits Command.
 *
 * @param   cmd - cmd need to send.
 *
 * @return  none
 */
extern void nfc_pcd_send_cmd(uint8_t cmd);

/*********************************************************************
 * @fn      nfc_pcd_send_data
 *
 * @brief   Commands and data are sent by byte,
 *          and the check digit is calculated during the send.
 *
 * @param   data - pointer to the data need to send.
 *          len - the length of data, max length is 18.
 *
 * @return  none
 */
extern void nfc_pcd_send_data(uint8_t *data, uint8_t len);

/*********************************************************************
 * @fn      nfc_pcd_send_bits
 *
 * @brief   Commands and data are sent by byte,
 *          using the given check digit directly in the send.
 *
 * @param   data - pointer to the data need to send.
 *          len - the length of data, max length is 18.
 *          parity - the parity bits need to send.
 *
 * @return  none
 */
extern void nfc_pcd_send_bits(uint8_t *data, uint8_t len, uint8_t *parity);

/*********************************************************************
 * @fn      nfc_pcd_decode_data
 *
 * @brief   decode the captured data.
 *
 * @param   dat - volatile nfc_pcd_signal_data_t *
 *
 * @return  uint16_t, the num of decode bits.
 */
extern uint16_t nfc_pcd_decode_data(volatile nfc_pcd_signal_data_t *dat);

/*********************************************************************
 * @fn      ISO14443_CRCA
 *
 * @brief   check CRC.
 *
 * @param   Buffer - the data need to check
 *          ByteCount -  the length of data
 *
 * @return  0 if no error.
 */
extern uint16_t ISO14443_CRCA(uint8_t *Buffer, uint8_t ByteCount);

/*********************************************************************
 * @fn      ISO14443AAppendCRCA
 *
 * @brief   add CRC value behind the data.
 *
 * @param   Buffer - the data need to check
 *          ByteCount -  the length of data
 *
 * @return  crc value.
 */
extern uint16_t ISO14443AAppendCRCA(void *Buffer, uint16_t ByteCount);

/**
 * @brief Register random generate.
 *
 * @param[in] cb    random generate function.
 */
extern void nfc_pcd_cypto_rand_register(nfc_pcd_cypto_rand_t cb);

/**
 * @brief Create cipher stream.
 *
 * @param[in]   key             sector key.
 * @param[in]   tag_uid         uuid of tag.
 * @param[in]   tag_clg         challenge of tag.
 * @param[out]  reader_clg      challenge of reader.
 * @param[out]  reader_rsp      response of reader.
 * @param[out]  tag_rsp         response of tag.
 * @param[out]  oddparity       oddparity of reader_clg,reader_rsp.
 * @param[in]   is_encrypted    1 - not the first auth
 *                              0 - first auth.
 */
extern void nfc_pcd_cypto_create(uint8_t *key, uint8_t *tag_uid, uint8_t *tag_clg, uint8_t *reader_clg, uint8_t *reader_rsp,
                         uint8_t *tag_rsp, uint8_t *oddparity, uint8_t is_encrypted);

/**
 * @brief Encrypt data.
 *
 * @param[in]   in              plaintext.
 * @param[out]  out             chiphertext.
 * @param[out]  oddparity       oddparity of data.
 * @param[in]   len             bit length of data.
 */
extern void nfc_pcd_cypto_encrypt(uint8_t *in, uint8_t *out, uint8_t *oddparity, uint8_t len);

/**
 * @brief Decrypt data.
 *
 * @param[in]   in              chiphertext.
 * @param[out]  out             plaintext.
 * @param[in]   oddparity       oddparity need to confirm.
 * @param[in]   len             bit length of data.
 *
 * @return      0 - success
 *              otherwise - failed.
 */
extern uint8_t nfc_pcd_cypto_decrypt(uint8_t *in, uint8_t *out, uint8_t *oddparity, uint8_t len);

#endif /* _WCH_NFC_PCD_H_ */
