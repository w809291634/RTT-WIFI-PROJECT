#include <rtthread.h>
#include "sensor_thread.h"
#include "sensor.h"

//���������߳�
static struct rt_thread sensor_thread;      /* �߳̽ṹ�� */
ALIGN(8) static rt_uint8_t sensor_stack[RT_SENSOR_THREAD_STACK_SIZE]; /* �߳�ջ */
static void sensor_thread_entry(void *parameter); /* �߳���ں��� */
//�����ഫ�����߳�
struct rt_thread sensor_check_thread;      /* �߳̽ṹ�� */
ALIGN(8) static rt_uint8_t sensor_check_stack[RT_SENSOR_CHECK_THREAD_STACK_SIZE]; /* �߳�ջ */
static void sensor_check_thread_entry(void *parameter); /* �߳���ں��� */

static void sensor_thread_entry(void *parameter) {
  sensor_init();
  while(1) {
    sensorUpdate();
  }
}

static void sensor_check_thread_entry(void *parameter) {
  while(1) {
    sensorCheck();
  }
}

void sensor_thread_init(void) {
  rt_err_t result;
  result = rt_thread_init(&sensor_check_thread, "sensor check", sensor_check_thread_entry, RT_NULL,
                          sensor_check_stack, sizeof(sensor_check_stack), RT_SENSOR_CHECK_THREAD_PRIORITY, RT_SENSOR_CHECK_THREAD_TICK);
  result = rt_thread_init(&sensor_thread, "sensor poll", sensor_thread_entry, RT_NULL,
                          sensor_stack, sizeof(sensor_stack), RT_SENSOR_THREAD_PRIORITY, RT_SENSOR_THREAD_TICK);
  if(result == RT_EOK) {
    result = rt_thread_startup(&sensor_thread);
    if(result != RT_EOK) {
      rt_kprintf("Error: sensor thread started failed!\n");
    }
  } else {
    rt_kprintf("Error: sensor thread create failed!\n");
  }
}
