#include <rtthread.h>
#include "rf_thread.h"
#include "drv_led/fml_led.h"

static struct rt_thread rf_thread;      /* 线程结构体 */
ALIGN(8) static rt_uint8_t rf_stack[RT_RF_THREAD_STACK_SIZE]; /* 线程栈 */
static void rf_thread_entry(void *parameter); /* 线程入口函数 */
static struct rt_thread rf_link_thread;      /* 线程结构体 */
ALIGN(8) static rt_uint8_t rf_link_stack[RT_RF_LINK_THREAD_STACK_SIZE]; /* 线程栈 */
static void rf_link_thread_entry(void *parameter); /* 线程入口函数 */

struct rt_mutex rf_zxbee_mutex;         /* zxbee处理锁 */
struct rt_mutex rf_send_mutex;          /* 无线发送锁 */

uint8_t rf_Link_On = 0;                 /* 无线通讯模块到网关的通讯状态。0--未连接，1--已连接 */

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
