#include <rtthread.h>
#include "finsh_ex.h"
#include "apl_rf/rf_thread.h"
#include "apl_at/at_thread.h"
#include "drv_led/fml_led.h"
#include "rf_thread.h"
#include "sensor_thread.h"
#include "sensor/sensor.h"
#include "apl_at/at_hw.h"

static void sys_param_init(void);

extern rt_event_t finger_event;

static struct rt_thread at_thread;      /* �߳̽ṹ�� */
ALIGN(8) static rt_uint8_t at_stack[RT_AT_THREAD_STACK_SIZE]; /* �߳�ջ */
static void at_thread_entry(void *parameter); /* �߳���ں��� */

static void at_thread_entry(void *parameter) {

  while(1) {
    if(RT_EOK == rt_event_recv(finger_event, 1,
                           RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                           RT_WAITING_FOREVER, RT_NULL) )
      {
        xprintf("here\r\n");
        rt_thread_mdelay(1000);
      }

  }
}

void app_thread_init(void) {
  rt_err_t result;
  result = rt_thread_init(&at_thread, "app", at_thread_entry, RT_NULL,
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

int main(void) {
//  sys_param_init();     //ϵͳ������ʼ��
//  led_Init();           //LED��ʼ��
//  sensor_thread_init(); //�������ɼ������߳�
//  rf_thread_init();     //ͨѶ�߳�
  at_thread_init();     //ATָ����߳�
  
  finger_init();
  app_thread_init();

  while(1){
    rt_event_control(finger_event, RT_IPC_CMD_RESET, RT_NULL);
    rt_thread_mdelay(1000);
  }
}





static void sys_param_init(void) {
  //��ʼ������ģ�����
  rf_info_init();
  //��ʼ�����в���
  sensor_para_init();
}
