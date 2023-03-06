#ifndef __RF_IF_HW_H__
#define __RF_IF_HW_H__

#include "stm32f10x.h"

//数据缓冲区长度
#define RF_IF_RINGBUF_SIZE 1024

//串口IO
#define RF_IF_TX_CLK       RCC_APB2Periph_GPIOA
#define RF_IF_TX_PORT      GPIOA
#define RF_IF_TX_PIN       GPIO_Pin_9
//#define RF_IF_TX_AF        GPIO_AF_UART5
//#define RF_IF_TX_SOURCE    GPIO_PinSource12
#define RF_IF_RX_CLK       RCC_APB2Periph_GPIOA
#define RF_IF_RX_PORT      GPIOA
#define RF_IF_RX_PIN       GPIO_Pin_10
//#define RF_IF_RX_AF        GPIO_AF_UART5
//#define RF_IF_RX_SOURCE    GPIO_PinSource2
//串口设备
#define RF_IF_UART         USART1
#define RF_IF_UART_CLK     RCC_APB2Periph_USART1
#define RF_IF_UART_CLKInit RCC_APB2PeriphClockCmd
#define RF_IF_UART_IRQn    USART1_IRQn
#define RF_IF_UART_ISR     USART1_IRQHandler
//串口DMA配置
#define RF_IF_DMA_CLK      RCC_AHBPeriph_DMA1
#define RF_IF_DMA_CHANNEL  DMA1_Channel5

extern struct rt_mutex rf_if_uart_mutex;
extern struct rt_mailbox rf_if_data_mb;

void rf_if_NetStatus(FlagStatus Status);
void rf_if_uart_send(uint8_t *pdata, uint16_t length);
void rf_if_uart_flush(uint16_t timeout_ms);
int16_t rf_if_uart_read(uint8_t *pdata, uint16_t length, uint16_t timeout_ms);
void rf_if_HW_Init(void);
void rf_if_uart_speed(uint32_t baudrate);

#endif  //__RF_IF_HW_H__
