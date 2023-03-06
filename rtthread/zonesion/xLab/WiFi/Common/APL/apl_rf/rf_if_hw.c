/* ͨѶģ��2�Žӿ�Ӳ������ */
#include <rtthread.h>
#include <string.h>
#include "rf_if_hw.h"
#include "drv_led/fml_led.h"
#include "wifi/wifi_drv.h"

struct rt_mutex rf_if_uart_mutex;
//�ʼ����ݣ���Ϣ��Դ��1--���жϷ������ʼ���2--��Wifi����̨�������ʼ�
struct rt_mailbox rf_if_data_mb;
ALIGN(4) static uint8_t rf_if_data_mb_poll[40]; //��Ϣ���仺����������Ϊ��������*4

ALIGN(4) static uint8_t rf_if_ringbuf[RF_IF_RINGBUF_SIZE];    //�������ݻ�����
static volatile uint16_t rf_if_Write_Index=0, rf_if_Read_Index=0;//������������
extern uint8_t debug_flag;

static void rf_if_GPIO_Init(void);
static void rf_if_DMA_Init(void);
static void rf_if_UART_Init(void);
static uint16_t rf_if_uart_GetRemain(void);

/* ������������ */
void rf_if_NetStatus(FlagStatus Status) {
  if(Status == SET) {
    led_on(LED_NET);
  } else {
    led_off(LED_NET);
  }
}

/* ���ڷ������� */
void rf_if_uart_send(uint8_t *pdata, uint16_t length) {
  if(rt_mutex_take(&rf_if_uart_mutex, 1000) != RT_EOK) return;
  for(uint16_t i=0;i<length;i++){
    while(USART_GetFlagStatus(RF_IF_UART, USART_FLAG_TXE) == RESET);
    USART_SendData(RF_IF_UART, pdata[i]);
  }
  if(debug_flag & 0x01) rt_kprintf("rf<--%s\n", pdata);
  rt_mutex_release(&rf_if_uart_mutex);
}

/* ���ڻ������������ */
void rf_if_uart_flush(uint16_t timeout_ms) {
  if(rt_mutex_take(&rf_if_uart_mutex, 1000) != RT_EOK) return;
  if(timeout_ms > 0) {
    rt_thread_mdelay(timeout_ms);
  }
  USART_Cmd(RF_IF_UART, DISABLE);
  USART_ITConfig(RF_IF_UART, USART_IT_IDLE, DISABLE);
  rf_if_Read_Index = rf_if_Write_Index;
  rt_mb_control(&rf_if_data_mb, RT_IPC_CMD_RESET, NULL);
  USART_ClearFlag(RF_IF_UART, USART_FLAG_IDLE);
  USART_ITConfig(RF_IF_UART, USART_IT_IDLE, ENABLE);
  USART_Cmd(RF_IF_UART, ENABLE);
  rt_mutex_release(&rf_if_uart_mutex);
}

/* ��ȡ���ڻ��������ݡ�����ֵ��>0--��ȡ�ɹ��������ĳ��ȣ�=0--δ�������� */
int16_t rf_if_uart_read(uint8_t *pdata, uint16_t length, uint16_t timeout_ms) {
  uint16_t remain_length = rf_if_uart_GetRemain();
  if(length > remain_length) {//ʣ�����ݳ��Ȳ���
    if(timeout_ms > 0) {//�ȴ�һ��ʱ�䣬���Ƿ����յ��㹻���ȵ�����
      rt_thread_mdelay(timeout_ms);
      remain_length = rf_if_uart_GetRemain();
    }
    if(length > remain_length) length = remain_length;
  }
  for(uint16_t i = 0; i < length; i++) {//���ݴ����ȡ��
    *(pdata+i) = rf_if_ringbuf[(rf_if_Read_Index+i)%RF_IF_RINGBUF_SIZE];
  }
  rf_if_Read_Index = (rf_if_Read_Index+length)%RF_IF_RINGBUF_SIZE;
  pdata[length] = 0;
  if(debug_flag & 0x01) rt_kprintf("rf-->%s\n", pdata);
  return length;
}

/* ���ڳ�ʼ�� */
void rf_if_HW_Init(void) {
  rt_mutex_init(&rf_if_uart_mutex, "rf if uart mutex", RT_IPC_FLAG_FIFO);
  rt_mb_init(&rf_if_data_mb, "rf if data mb",\
              rf_if_data_mb_poll, sizeof(rf_if_data_mb_poll)/4, RT_IPC_FLAG_PRIO);
  rf_if_GPIO_Init();
  rf_if_DMA_Init();
  rf_if_UART_Init();
}

//�����жϴ���
void RF_IF_UART_ISR(void) {
  if(USART_GetITStatus(RF_IF_UART, USART_IT_IDLE) != RESET) {
    USART_ReceiveData(RF_IF_UART);//���IDLE�жϱ�־λ
    rf_if_Write_Index = (RF_IF_RINGBUF_SIZE - DMA_GetCurrDataCounter(RF_IF_DMA_CHANNEL))%RF_IF_RINGBUF_SIZE;
    rt_mb_send(&rf_if_data_mb, 1);//�ʼ����ݣ�1--���жϷ������ʼ�
  }
}

/* ���ڲ��������� */
void rf_if_uart_speed(uint32_t baudrate) {
  USART_InitTypeDef USART_InitStructure;
  USART_StructInit(&USART_InitStructure);
  USART_InitStructure.USART_BaudRate = baudrate;
  
  USART_Cmd(RF_IF_UART, DISABLE);
  USART_Init(RF_IF_UART, &USART_InitStructure);
  rf_if_uart_flush(0);//�˺����ڻ�ENABLE USART
}

/* ��ȡ��������ʣ��δ��ȡ���ݳ��� */
static uint16_t rf_if_uart_GetRemain(void) {
  uint16_t remain_length;//ʣ�����ݳ���
  uint16_t read_index=rf_if_Read_Index, write_index=rf_if_Write_Index;//��ǰ��������
  if(write_index >= read_index) {//��ȡʣ�����ݳ���
    remain_length = write_index - read_index;
  } else {
    remain_length = RF_IF_RINGBUF_SIZE - read_index + write_index;
  }
  return remain_length;
}

/* ���߽ӿ�1-IO��ʼ�� */
static void rf_if_GPIO_Init(void) {
  GPIO_InitTypeDef	GPIO_InitStructure;
  
  RCC_APB2PeriphClockCmd(RF_IF_TX_CLK | RF_IF_RX_CLK, ENABLE);
  
  /* Configure USARTy Rx as input floating */
  GPIO_InitStructure.GPIO_Pin = RF_IF_RX_PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(RF_IF_RX_PORT, &GPIO_InitStructure);
        
  /* Configure USARTx_Tx as alternate function push-pull */
  GPIO_InitStructure.GPIO_Pin = RF_IF_TX_PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(RF_IF_TX_PORT, &GPIO_InitStructure);
}

/* ���߽ӿ�1-DMA��ʼ�� */
static void rf_if_DMA_Init(void) {
  DMA_InitTypeDef DMA_InitStruct;
  RCC_AHBPeriphClockCmd(RF_IF_DMA_CLK, ENABLE);

  DMA_DeInit(RF_IF_DMA_CHANNEL);
  DMA_InitStruct.DMA_MemoryBaseAddr = (uint32_t)(&rf_if_ringbuf[0]);
  DMA_InitStruct.DMA_PeripheralBaseAddr = (uint32_t)&(RF_IF_UART->DR);
  DMA_InitStruct.DMA_DIR = DMA_DIR_PeripheralSRC;
  DMA_InitStruct.DMA_BufferSize = RF_IF_RINGBUF_SIZE;
  DMA_InitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  DMA_InitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_InitStruct.DMA_Mode = DMA_Mode_Circular;
  DMA_InitStruct.DMA_Priority = DMA_Priority_High;
  DMA_InitStruct.DMA_M2M = DMA_M2M_Disable;      
  DMA_Init(RF_IF_DMA_CHANNEL, &DMA_InitStruct);
  DMA_Cmd(RF_IF_DMA_CHANNEL, ENABLE);
}

/* ���߽ӿ�1-���ڳ�ʼ�� */
static void rf_if_UART_Init(void) {
  USART_InitTypeDef USART_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  
  RF_IF_UART_CLKInit(RF_IF_UART_CLK, ENABLE);
  
  USART_DeInit(RF_IF_UART);
  USART_InitStructure.USART_BaudRate            = 115200;
  USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits            = USART_StopBits_1;
  USART_InitStructure.USART_Parity              = USART_Parity_No ;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
  USART_Init(RF_IF_UART, &USART_InitStructure);
  
  NVIC_InitStructure.NVIC_IRQChannel = RF_IF_UART_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  USART_ClearFlag(RF_IF_UART, USART_FLAG_RXNE|USART_FLAG_IDLE);
  USART_ITConfig(RF_IF_UART, USART_IT_IDLE, ENABLE);
  USART_DMACmd(RF_IF_UART, USART_DMAReq_Rx, ENABLE);
  USART_Cmd(RF_IF_UART, ENABLE);
}
