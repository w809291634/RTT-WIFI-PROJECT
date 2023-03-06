/*********************************************************************************************
* 文件：sensor.c
* 作者：
* 说明：通用传感器控制接口程序
* 修改：
* 注释：
*********************************************************************************************/
#include <rtthread.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "sensor.h"
#include "htu21d.h"
#include "fbm320.h"
#include "BH1750.h"
#include "lis3dh.h"
#include "MP-503.h"
#include "stadiometry.h"
#include "drv_relay/relay.h"
#include "math.h"
#include "rf_thread.h"
#include "sensor_thread.h"

static struct rt_timer sensor_poll_timer;
static struct rt_semaphore sensor_poll_sem;
static void sensor_poll_timer_cb(void *parameter);

//存储A0-A7数据转成的字符串；每个字符串不超过11个有效字符；不存在的传感器应初始化为""
static char sensor_data_A[8][12] = {"0", "0", "0", "0", "0", "0", "0", ""}; 
t_sensor_value sensor_value;      //传感器原始数据结构体
static uint8_t lis3dh_status = 0;                               // 三轴传感器状态

/*********************************************************************************************
* 名称：sensor_init()
* 功能：
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void sensor_init(void) {
  /*******创建定时发送定时器，定时间隔为V0*****/
  rt_timer_init(&sensor_poll_timer, "sensor poll tim",
                sensor_poll_timer_cb, RT_NULL,
                sensor_value.V0*1000, RT_TIMER_FLAG_PERIODIC);
  rt_sem_init(&sensor_poll_sem, "sensor poll sem", 0, RT_IPC_FLAG_FIFO);
  /***************初始化各传感器***************/
  sensorControl(sensor_value.D1);   //开机恢复此前控制状态
  rt_timer_start(&sensor_poll_timer);       //开启定时器，每隔V0主动上报一次数据
  rt_thread_startup(&sensor_check_thread);  //存在安防类传感器时，开启安防类传感器线程
  htu21d_init();                            // 温湿度传感器初始化
  bh1750_init();                            // 光强度传感器初始化
  airgas_init();                            // 空气质量传感器初始化
  fbm320_init();                            // 大气压力传感器初始化
  lis3dh_status = lis3dh_init();            // 三轴传感器初始化
  stadiometry_init();                       // 红外测距传感器初始化
  relay_init();                             // 继电器初始化
}

static void updateA0(void) {
  // 读取温度值，并更新A0
  sensor_value.A0 = (htu21d_get_data(TEMP)/100.0f);
  sprintf(sensor_data_A[0], "%.1f", sensor_value.A0);
}

static void updateA1(void) {
  // 读取湿度值，并更新A1
  sensor_value.A1 = (htu21d_get_data(HUMI)/100.0f);
  sprintf(sensor_data_A[1], "%.1f", sensor_value.A1);
}

static void updateA2(void) {
  // 读取光照值，并更新A2
  sensor_value.A2 = bh1750_get_data();
  sprintf(sensor_data_A[2], "%.1f", sensor_value.A2);
}

static void updateA3(void) {
  // 读取空气有害气体浓度值，并更新A3
  sensor_value.A3 = get_airgas_data();
  sprintf(sensor_data_A[3], "%d", sensor_value.A3);
}

static void updateA4(void) {
  // 读取大气压力值，并更新A4
  float temperature = 0;                                        // 存储温度数据变量
  long pressure = 0;                                            // 存储压力数据变量

  fbm320_data_get(&temperature,&pressure);                      // 获取温度、压力数据
  sensor_value.A4 = pressure/100.0f;                            // 转换压力单位为百帕
  sprintf(sensor_data_A[4], "%.1f", sensor_value.A4);
}

static void updateA5(void) {
  static uint32_t ct = 0;
  float accx,accy,accz;
  
  if (sensor_value.A5 == 0) {
    lis3dh_read_data(&accx,&accy,&accz);
    if(!(accx==0&&accy==0&&accz==0)) {
      sensor_value.A5 = fallDect(accx,accy,accz);
      ct = rt_tick_get();
    } else {
      sensor_value.A5 = 0;
    }
  } else if (sensor_value.A5!=0 && rt_tick_get()>(ct+2000)) {
    sensor_value.A5 = 0;
  }
  sprintf(sensor_data_A[5], "%d", sensor_value.A5);
}

static void updateA6(void) {
  // 读取红外测距值，并更新A6
  sensor_value.A6 = get_stadiometry_data();
  sprintf(sensor_data_A[6], "%.1f", sensor_value.A6);
}

static void updateA7(void) {
}

/* 开关量操作-D0
 * 参数：type：1--OD（设置位），2--CD（清除位）；value：要设置或清除的位 */
static void updateD0(uint8_t type, uint8_t value) {
  uint8_t temp;
  if(type == 1) {//OD
    temp = sensor_value.D0 | value;
  } else if(type == 2) {//CD
    temp = sensor_value.D0 & (~value);
  }
  if(sensor_value.D0 != temp) {
    sensor_value.D0 = temp;
  }
}

/* 开关量操作-D1
 * 参数：type：1--OD（设置位），2--CD（清除位）；value：要设置或清除的位 */
static void updateD1(uint8_t type, uint8_t value) {
  uint8_t temp;
  if(type == 1) {//OD
    temp = sensor_value.D1 | value;
  } else if(type == 2) {//CD
    temp = sensor_value.D1 & (~value);
  }
  if(sensor_value.D1 != temp) {
    sensor_value.D1 = temp;
    sensorControl(sensor_value.D1);//执行D1相关控制操作
  }
}

static void updateV0(char *pval) {
  uint32_t value = atoi(pval);
  if(value == 0 || value > 65535) return; //数据超出限制：不保存
  if(sensor_value.V0 != value) {
    sensor_value.V0 = value;
    //更新定时器间隔
    value *= 1000;
    rt_timer_control(&sensor_poll_timer, RT_TIMER_CTRL_SET_TIME, (void *)&value);
    rt_timer_start(&sensor_poll_timer);//重启定时器
  }
}

static void updateV1(char *pval) {
}

static void updateV2(char *pval) {
}

static void updateV3(char *pval) {
}

/*********************************************************************************************
* 名称：sensorLinkOn()
* 功能：传感器节点入网成功调用函数
* 参数：无
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
void sensorLinkOn(void) {
  rt_sem_release(&sensor_poll_sem);         //发送信号，令sensorUpdate主动上报一次数据
}

/*查询各传感器值，达到V0设定时间时上报*/
void sensorUpdate(void) {
  updateA0();
  updateA1();
  updateA2();
  updateA3();
  updateA4();
  updateA5();
  updateA6();
  updateA7();
  
  rt_err_t err = rt_sem_take(&sensor_poll_sem, 1000);
  if(err == RT_EOK) {//通过定时器，每隔V0时间，收到定时器发来的信号，发送一次数据
    //生成发送字符串
    char data_send[160] = "{";
    char temp[20];
    for(uint8_t i = 0; i < 8; i++) {
      if(strlen(sensor_data_A[i]) > 0) {//是本节点上存在的传感器
        if((sensor_value.D0 & (1<<i)) != 0) { //D0中使能该传感器自动上报
          sprintf(temp, "A%u=%s,", i, sensor_data_A[i]);
          strcat(data_send, temp);
        }
      }
    }
    sprintf(temp, "D1=%u,", sensor_value.D1);
    strcat(data_send, temp);
    //发送数据到网关/服务器
    if(strlen(data_send) > 1) {
      data_send[strlen(data_send)-1] = '}';
      //发送数据到网关/服务器
      rf_send((uint8_t *)data_send, strlen(data_send), 200);
    }
  }
}

/*********************************************************************************************
* 名称：sensorCheck()
* 功能：传感器监测，安防类传感器异常时快速发送消息
* 参数：无
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
void sensorCheck(void) {
  char data_send[100] = "{";
  char temp[20];
  uint8_t send_flag = 0;
  
  static int lastA5 = 0;
 
  if(lis3dh_status == 0) {
    updateA5();                             // 跌倒检测
  }
  if (sensor_value.A5 != lastA5) {
    sprintf(temp, "A5=%s,", sensor_data_A[5]);
    strcat(data_send, temp);
    lastA5 = sensor_value.A5;
    send_flag = 1;
  }
  
//  static uint16_t A6_count = 0;
//  static uint32_t A6_last;
//  if((sensor_value.D0 & (1<<6)) != 0) {     //堵塞状态
//    if(sensor_value.A6 == 1 && ++A6_count >= 100) {
//      A6_last = sensor_value.A6;
//      sprintf(temp, "A6=%s,", sensor_data_A[6]);
//      strcat(data_send, temp);
//      A6_count = 0;
//      send_flag = 1;
//    } else if(sensor_value.A6 == 0) {
//      A6_count = 0; //A6变为0时，清空计数
//      if(A6_last == 1) {
//        A6_last = sensor_value.A6;
//        sprintf(temp, "A6=%s,", sensor_data_A[6]);
//        strcat(data_send, temp);
//        send_flag = 1;
//      }
//    }
//  }

  //有异常时，每3秒发送一次异常数据
  if(strlen(data_send) > 2) {
    if(send_flag == 1) {
      data_send[strlen(data_send)-1] = '}';
      rf_send((uint8_t *)data_send, strlen(data_send), 200);
    }
  }
  rt_thread_mdelay(100);
}

/*********************************************************************************************
* 名称：sensorControl()
* 功能：传感器控制
* 参数：cmd - 控制命令
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
void sensorControl(uint8_t cmd) {
  relay_control(cmd>>6);
}

/*********************************************************************************************
* 名称：z_process_command_call()
* 功能：处理上层应用发过来的指令
* 参数：ptag: 指令标识 D0，D1， A0 ...
*       pval: 指令值， “?”表示读取，
*       obuf: 指令处理结果存放地址
* 返回：>0指令处理结果返回数据长度，0：没有返回数据，<0：不支持指令。
* 修改：
* 注释：
*********************************************************************************************/
int z_process_command_call(char* ptag, char* pval, char* obuf) {
  if (ptag[0] == 'A') {
    if (ptag[1] >= '0' && ptag[1] <= '7') {
      uint8_t tag = atoi(&ptag[1]);
      if (pval[0] == '?') { //{Ax=?}
        uint8_t len = strlen(sensor_data_A[tag]);
        if(len > 0) {
          strcpy(obuf, sensor_data_A[tag]);
        }
        return len;
      }
    }
  } else if (ptag[0] == 'D') {
    if (ptag[1] >= '0' && ptag[1] <= '1') {
      uint8_t tag = atoi(&ptag[1]);
      if (pval[0] == '?') { //{Dx=?}
        switch(tag) {
          case 0: sprintf(obuf, "%u", sensor_value.D0); break;
          case 1: sprintf(obuf, "%u", sensor_value.D1); break;
          default: return -1;
        }
        return strlen(obuf);
      }
    }
  } else if (ptag[0] == 'V') {
    if (ptag[1] >= '0' && ptag[1] <= '3') {
      uint8_t tag = atoi(&ptag[1]);
      if (pval[0] == '?') { //{Vx=?}
        switch(tag) {
          case 0: sprintf(obuf, "%u", sensor_value.V0); break;
          case 3: strcpy(obuf, sensor_value.V3); break;
          default: return -1;
        }
        return strlen(obuf);
      } else {  //{Vx=y}
        switch(tag) {
          case 0: updateV0(pval); break;
          case 1: updateV1(pval); break;
          case 2: updateV2(pval); break;
          case 3: updateV3(pval); break;
          default: return -1;
        }
        return 0;
      }
    }
  } else if(strncmp(ptag, "OD", 2) == 0) {  //{ODx=y}
    if (ptag[2] >= '0' && ptag[2] <= '1') {
      uint8_t tag = atoi(&ptag[2]);
      uint8_t value = atoi(pval);//*pval为数字时，value为要设置的值；*pval不为数字时，atoi返回0，也不会改变当前值
      switch(tag) {
        case 0: 
          updateD0(1, value);
          sprintf(obuf, "%u", sensor_value.D0);
        break;
        case 1: 
          updateD1(1, value);
          sprintf(obuf, "%u", sensor_value.D1);
        break;
        default: return -1;
      }
      return strlen(obuf);
    }
  } else if(strncmp(ptag, "CD", 2) == 0) {  //{CDx=y}
    if (ptag[2] >= '0' && ptag[2] <= '1') {
      uint8_t tag = atoi(&ptag[2]);
      uint8_t value = atoi(pval);//*pval为数字时，value为要设置的值；*pval不为数字时，atoi返回0，也不会改变当前值
      switch(tag) {
        case 0: 
          updateD0(2, value);
          sprintf(obuf, "%u", sensor_value.D0);
        break;
        case 1: 
          updateD1(2, value);
          sprintf(obuf, "%u", sensor_value.D1);
        break;
        default: return -1;
      }
      return strlen(obuf);
    }
  }
  
  return -1;
}

/* K3按键触发后的处理 */
void sensor_Keypressed(void) {
  rt_sem_release(&sensor_poll_sem);
}

/* 传感参数初始化--用于未读取到存储的设置值的情况 */
void sensor_para_init(void) {
  sensor_value.D0 = 0xFF;
  if(sensor_value.D1 == 0xFF) sensor_value.D1 = 0x0;
  if(sensor_value.V0 == 0xFFFF || sensor_value.V0 == 0) sensor_value.V0 = 30;
}

/* 传感器校准 */
void sensor_cali(uint8_t ch, int value) {
}

static void sensor_poll_timer_cb(void *parameter) {
  //定时器达到V0时间间隔后，发送信号，让sensorUpdate函数发送数据
  rt_sem_release(&sensor_poll_sem);
}
