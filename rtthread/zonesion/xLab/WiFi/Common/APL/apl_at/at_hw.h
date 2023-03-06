#ifndef __AT_HW_H__
#define __AT_HW_H__

#include "stm32f10x.h"

//���ݻ���������
#define AT_RINGBUF_SIZE 512

#ifdef WIFI_SERIAL      /*͸���ڵ�--����2*/
  //����IO
  #define AT_TX_CLK       RCC_APB2Periph_GPIOA
  #define AT_TX_PORT      GPIOA
  #define AT_TX_PIN       GPIO_Pin_2
  #define AT_RX_CLK       RCC_APB2Periph_GPIOA
  #define AT_RX_PORT      GPIOA
  #define AT_RX_PIN       GPIO_Pin_3
  //�����豸
  #define AT_UART         USART2
  #define AT_UART_CLK     RCC_APB1Periph_USART2
  #define AT_UART_CLKInit RCC_APB1PeriphClockCmd
  #define AT_UART_IRQn    USART2_IRQn
  #define AT_UART_ISR     USART2_IRQHandler
  //����DMA����
  #define AT_DMA_CLK      RCC_AHBPeriph_DMA1
  #define AT_DMA_CHANNEL  DMA1_Channel6
#else                   /*�ն˽ڵ�--����3*/
  //����IO
  #define AT_TX_CLK       RCC_APB2Periph_GPIOB
  #define AT_TX_PORT      GPIOB
  #define AT_TX_PIN       GPIO_Pin_10
  #define AT_RX_CLK       RCC_APB2Periph_GPIOB
  #define AT_RX_PORT      GPIOB
  #define AT_RX_PIN       GPIO_Pin_11
  //�����豸
  #define AT_UART         USART3
  #define AT_UART_CLK     RCC_APB1Periph_USART3
  #define AT_UART_CLKInit RCC_APB1PeriphClockCmd
  #define AT_UART_IRQn    USART3_IRQn
  #define AT_UART_ISR     USART3_IRQHandler
  //����DMA����
  #define AT_DMA_CLK      RCC_AHBPeriph_DMA1
  #define AT_DMA_CHANNEL  DMA1_Channel3
#endif

extern struct rt_mutex at_uart_mutex;
extern struct rt_mailbox at_data_mb;

void at_uart_send(char *pdata);
void at_uart_flush(uint16_t timeout_ms);
int16_t at_uart_read(uint8_t *pdata, uint16_t length, uint16_t timeout_ms);
void at_HW_Init(void);
void at_uart_speed(uint32_t baudrate);

#endif  //__AT_HW_H__
