#include "led_bsp.h"


void led_bsp_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure={0};

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init( GPIOC, &GPIO_InitStructure );

    GPIO_SetBits(GPIOC, GPIO_Pin_1 | GPIO_Pin_2);

}


