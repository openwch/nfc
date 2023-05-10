#ifndef _LED_BSP_H_
#define _LED_BSP_H_

#include "debug.h"

#define LED2_BLUE_PIN       GPIO_Pin_1
#define LED2_BLUE_ON()      GPIO_ResetBits(GPIOC, LED2_BLUE_PIN)
#define LED2_BLUE_OFF()     GPIO_SetBits(GPIOC, LED2_BLUE_PIN)

#define LED3_RED_PIN        GPIO_Pin_2
#define LED3_RED_ON()       GPIO_ResetBits(GPIOC, LED3_RED_PIN)
#define LED3_RED_OFF()      GPIO_SetBits(GPIOC, LED3_RED_PIN)

extern void led_bsp_init(void);

#endif /* _LED_BSP_H_ */
