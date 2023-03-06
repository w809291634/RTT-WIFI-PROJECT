#include <rtthread.h>
#include "finsh_ex.h"
#include "apl_rf/rf_thread.h"
#include "apl_at/at_thread.h"
#include "drv_led/fml_led.h"
#include "rf_thread.h"
#include "sensor_thread.h"
#include "sensor_node.h"

static void sys_param_init(void);

int main(void) {
  sys_param_init();     //ϵͳ������ʼ��
  led_Init();           //LED��ʼ��
  sensor_thread_init(); //�������ɼ������߳�
  rf_thread_init();     //ͨѶ�߳�
  at_thread_init();     //ATָ����߳�
}

static void sys_param_init(void) {
  //��ʼ������ģ�����
  rf_info_init();
  // Sensor_node��ʼ��
  sensor_node_select();
  //��ʼ�����в���
  sensor_fun_p.sensor_para_init();
}
