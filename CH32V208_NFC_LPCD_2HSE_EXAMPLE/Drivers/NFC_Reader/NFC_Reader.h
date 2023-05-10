/********************************** (C) COPYRIGHT *******************************
 * File Name          : NFC_Reader.h
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2023/05/08
 * Description        : NFC M1¿¨²Ù×÷Àý³Ì
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

#ifndef _NFC_READER_H_
#define _NFC_READER_H_

#include "NFC_Reader_bsp.h"

extern void nfc_signal_bsp_init(void);

extern void NFC_Process(void);

extern void changeHSETo27_12Mhz(void);

extern void changeHSETo32Mhz(void);

#endif /* _NFC_READER_H_ */
