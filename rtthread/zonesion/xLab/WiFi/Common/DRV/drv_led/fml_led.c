#include "fml_led.h"

void led_Init(void) {
  GPIO_InitTypeDef  GPIO_InitStructure;
  
  RCC_APB2PeriphClockCmd(LED_NET_CLK | LED_DAT_CLK, ENABLE);
  
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_Pin =  LED_NET_PIN;
  GPIO_Init(LED_NET_GPIO, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin =  LED_DAT_PIN;
  GPIO_Init(LED_DAT_GPIO, &GPIO_InitStructure);
  
  led_off(LED_NET);
  led_off(LED_DAT);
}

void led_on(e_LED led) {
  switch(led) {
    case LED_NET: GPIO_ResetBits(LED_NET_GPIO, LED_NET_PIN); break;
    case LED_DAT: GPIO_ResetBits(LED_DAT_GPIO, LED_DAT_PIN); break;
  }
}

void led_off(e_LED led) {
  switch(led) {
    case LED_NET: GPIO_SetBits(LED_NET_GPIO, LED_NET_PIN); break;
    case LED_DAT: GPIO_SetBits(LED_DAT_GPIO, LED_DAT_PIN); break;
  }
}
