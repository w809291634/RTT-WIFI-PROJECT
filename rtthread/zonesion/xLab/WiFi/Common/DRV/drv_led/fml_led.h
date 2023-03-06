#ifndef _FML_LED_H_
#define _FML_LED_H_
#include "stm32f10x.h"

#define LED_NET_CLK     RCC_APB2Periph_GPIOB
#define LED_NET_GPIO    GPIOB
#define LED_NET_PIN     GPIO_Pin_13

#define LED_DAT_CLK     RCC_APB2Periph_GPIOB
#define LED_DAT_GPIO    GPIOB
#define LED_DAT_PIN     GPIO_Pin_14

typedef enum {
  LED_NET=0, LED_DAT
} e_LED;

void led_on(e_LED led);
void led_off(e_LED led);
void led_toggle(e_LED led);
void led_Init(void);

#endif
