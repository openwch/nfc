/********************************** (C) COPYRIGHT *******************************
* File Name          : main.c
* Author             : WCH
* Version            : V1.0
* Date               : 2020/08/06
* Description        :
*******************************************************************************/



/******************************************************************************/
/* 头文件包含 */
#include "CONFIG.h"
#include "HAL.h"
#include "stdlib.h"
#include "NFC_Reader.h"
#include "led_bsp.h"
#include "peripheral.h"

//每个文件单独debug打印的开关，置0可以禁止本文件内部打印
#define DEBUG_PRINT_IN_THIS_FILE    1
#if DEBUG_PRINT_IN_THIS_FILE
    #define PRINTF(...) PRINT(__VA_ARGS__)
#else
    #define PRINTF(...) do {} while (0)
#endif

/*********************************************************************
 * GLOBAL TYPEDEFS
 */
__attribute__((aligned(4))) u32 MEM_BUF[BLE_MEMHEAP_SIZE / 4];

#if (defined (BLE_MAC)) && (BLE_MAC == TRUE)
uint8_t const MacAddr[6] = {0x84, 0xC2, 0xE4, 0x03, 0x02, 0x02};
#endif

/*********************************************************************
 * @fn      Main_Circulation
 *
 * @brief   tmos主循环.
 *
 * @param   None.
 *
 * @return  None.
 */
__attribute__((noinline))
__attribute__((section(".highcode")))
void Main_Circulation()
{
    while (1)
    {
        TMOS_SystemProcess();
    }
}

/*********************************************************************
 * @fn      main
 *
 * @brief   主函数
 *
 * @return  none
 */
int main(void)
{
    u8 i = 0;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    Delay_Init();

#ifdef DEBUG
    USART_Printf_Init(115200);
#endif
    PRINT("%s\n", VER_LIB);

    led_bsp_init();

    nfc_signal_bsp_init();

    WCHBLE_Init();

    HAL_Init();

    GAPRole_PeripheralInit();

    Peripheral_Init();

    Main_Circulation();
}

/******************************** endfile @ main ******************************/
