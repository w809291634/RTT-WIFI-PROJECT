#include <rtthread.h>
#include "at_thread.h"

static struct rt_thread at_thread;      /* �߳̽ṹ�� */
ALIGN(8) static rt_uint8_t at_stack[RT_AT_THREAD_STACK_SIZE]; /* �߳�ջ */
static void at_thread_entry(void *parameter); /* �߳���ں��� */

static void at_thread_entry(void *parameter) {
  at_init();
  while(1) {
    at_run();
  }
}

void at_thread_init(void) {
  rt_err_t result;
  result = rt_thread_init(&at_thread, "at comm", at_thread_entry, RT_NULL,
                          at_stack, sizeof(at_stack), RT_AT_THREAD_PRIORITY, RT_AT_THREAD_TICK);
  if(result == RT_EOK) {
    result = rt_thread_startup(&at_thread);
    if(result != RT_EOK) {
      rt_kprintf("Error: at thread started failed!\n");
    }
  } else {
    rt_kprintf("Error: at thread create failed!\n");
  }
}
