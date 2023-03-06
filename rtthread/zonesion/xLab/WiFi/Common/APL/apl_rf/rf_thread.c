#include <rtthread.h>
#include "rf_thread.h"
#include "drv_led/fml_led.h"

static struct rt_thread rf_thread;      /* �߳̽ṹ�� */
ALIGN(8) static rt_uint8_t rf_stack[RT_RF_THREAD_STACK_SIZE]; /* �߳�ջ */
static void rf_thread_entry(void *parameter); /* �߳���ں��� */
static struct rt_thread rf_link_thread;      /* �߳̽ṹ�� */
ALIGN(8) static rt_uint8_t rf_link_stack[RT_RF_LINK_THREAD_STACK_SIZE]; /* �߳�ջ */
static void rf_link_thread_entry(void *parameter); /* �߳���ں��� */

struct rt_mutex rf_zxbee_mutex;         /* zxbee������ */
struct rt_mutex rf_send_mutex;          /* ���߷����� */

uint8_t rf_Link_On = 0;                 /* ����ͨѶģ�鵽���ص�ͨѶ״̬��0--δ���ӣ�1--������ */

static void rf_link_thread_entry(void *parameter) {
  while(1) {
    rf_if_Link();
  }
}

static void rf_thread_entry(void *parameter) {
  rt_mutex_init(&rf_zxbee_mutex, "rf zxbee mutex", RT_IPC_FLAG_FIFO);
  rt_mutex_init(&rf_send_mutex, "rf send mutex", RT_IPC_FLAG_FIFO);
  rf_if_init();
  rt_thread_startup(&rf_link_thread);
  while(1) {
    rf_if_run();
  }
}

void rf_thread_init(void) {
  rt_err_t result;
  result = rt_thread_init(&rf_link_thread, "rf link wait", rf_link_thread_entry, RT_NULL,
                          rf_link_stack, sizeof(rf_link_stack), RT_RF_LINK_THREAD_PRIORITY, RT_RF_LINK_THREAD_TICK);
  result = rt_thread_init(&rf_thread, "rf comm", rf_thread_entry, RT_NULL,
                          rf_stack, sizeof(rf_stack), RT_RF_THREAD_PRIORITY, RT_RF_THREAD_TICK);
  if(result == RT_EOK) {
    result = rt_thread_startup(&rf_thread);
    if(result != RT_EOK) {
      rt_kprintf("Error: rf thread started failed!\n");
    }
  } else {
    rt_kprintf("Error: rf thread create failed!\n");
  }
}

int16_t rf_send(uint8_t *pdata, uint16_t length, uint16_t timeout_ms) {
  if(rf_Link_On != 1 || rt_mutex_take(&rf_send_mutex, 1000) != RT_EOK) return -1;
  
  led_on(LED_DAT);
  int16_t res;
  res = rf_if_send(pdata, length, timeout_ms);
  led_off(LED_DAT);
  
  rt_mutex_release(&rf_send_mutex);
  return res;
}
