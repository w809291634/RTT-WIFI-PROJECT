#include <rtthread.h>
#include "finsh_ex.h"
#include "apl_rf/rf_thread.h"
#include "apl_at/at_thread.h"
#include "drv_led/fml_led.h"
#include "rf_thread.h"
#include "sensor_thread.h"
#include "sensor/sensor.h"

static void sys_param_init(void);

int main(void) {
  sys_param_init();     //系统参数初始化
  led_Init();           //LED初始化
//  sensor_thread_init(); //传感器采集控制线程
  rf_thread_init();     //通讯线程
  at_thread_init();     //AT指令处理线程
}

static void sys_param_init(void) {
  //初始化无线模块参数
  rf_info_init();
  //初始化传感参数
//  sensor_para_init();
}
