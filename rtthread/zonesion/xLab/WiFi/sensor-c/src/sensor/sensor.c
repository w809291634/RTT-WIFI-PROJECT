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
#include "Flame.h"
#include "grating.h"
#include "hall.h"
#include "infrared.h"
#include "MP-4.h"
#include "Touch.h"
#include "vibration.h"
#include "drv_relay/relay.h"
#include "math.h"
#include "rf_thread.h"
#include "sensor_thread.h"

static struct rt_timer sensor_poll_timer;
static struct rt_semaphore sensor_poll_sem;
static void sensor_poll_timer_cb(void *parameter);

//存储A0-A7数据转成的字符串；每个字符串不超过11个有效字符；不存在的传感器应初始化为""
static char sensor_data_A[8][12] = {"0", "0", "0", "0", "0", "0", "", ""}; 
t_sensor_value sensor_value;      //传感器原始数据结构体
static uint8_t mode = 1; 

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
  hall_init();                                                // 霍尔传感器初始化
  infrared_init();                                            // 人体红外传感器初始化
  vibration_init();                                           // 震动传感器初始化
  flame_init();                                               // 火焰传感器初始化
  combustiblegas_init();                                      // 燃气传感器初始化
  grating_init();                                             // 光栅传感器初始化
  relay_init();                                               // 继电器初始化
  sensorControl(sensor_value.D1);
  rt_timer_start(&sensor_poll_timer);       //开启定时器，每隔V0主动上报一次数据
  rt_thread_startup(&sensor_check_thread);  //存在安防类传感器时，开启安防类传感器线程
}

static void updateA0(void) {
  static uint32_t ct = 0;
  uint8_t  A0 = 0;
   
  if (mode == 1) {                                               // 更新人体红外/触摸传感器的值
    A0 = get_infrared_status();
    if (A0 != 0) {
      ct = rt_tick_get();
    } else if (rt_tick_get() > ct+1000) {
      ct = 0;
      A0 = 0;
    } else {
      A0 = 1;
    }
  }
  sensor_value.A0 = A0;
  sprintf(sensor_data_A[0], "%u", sensor_value.A0);
}

static void updateA1(void) {
  static uint32_t ct = 0;
  uint8_t  A1 = 0;
  
  if (mode == 1) {                                              // 更新振动传感器的值
    A1 = get_vibration_status();
    if (A1 != 0) {
      ct = rt_tick_get();
    } else if (rt_tick_get() > ct+2000) {
      ct = 0;
      A1 = 0;
    } else {
      A1 = 1;
    }
  }
  sensor_value.A1 = A1;
  sprintf(sensor_data_A[1], "%u", sensor_value.A1);
}

static void updateA2(void) {
  if (mode == 1) {
    sensor_value.A2 = get_hall_status();                                     // 更新霍尔传感器的值
    sprintf(sensor_data_A[2], "%u", sensor_value.A2);
  }
}

static void updateA3(void) {
  static uint32_t ct = 0;
  uint8_t  A3 = 0;
   if (mode == 1) {
    A3 = get_flame_status();
    if (A3 != 0) {
      ct = rt_tick_get();
    } else if (rt_tick_get() > ct+2000) {
      ct = 0;
      A3 = 0;
    } else {
      A3 = 1;
    }
    sensor_value.A3 = A3;
    sprintf(sensor_data_A[3], "%u", sensor_value.A3);
  }
}

static void updateA4(void) {
  sensor_value.A4 = get_combustiblegas_data();
  sprintf(sensor_data_A[4], "%u", sensor_value.A4);
}

static void updateA5(void) {
  sensor_value.A5 = get_grating_status();
  sprintf(sensor_data_A[5], "%u", sensor_value.A5);
}

static void updateA6(void) {
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
  
  static char lastA0 = 0,lastA1=0,lastA2=0,lastA3=0,lastA4=0,lastA5=0;
  static uint32_t ct0=0, ct1=0, ct2=0, ct3=0, ct4=0, ct5=0;
 
  if(mode == 1) {
    updateA1();
    updateA2();
    updateA3();
  }
  updateA0();
  updateA4();
  updateA5();
  updateA6();
  updateA7();
  
  if (lastA0 != sensor_value.A0 
      || ( //模式1，人体红外传感器选通，报警后循环3S上报；模式2：触摸传感器选通，状态反转后上报1次，  
         (mode == 1)&& ct0 != 0 && rt_tick_get() > (ct0+3000)) //超过3秒上报
      ) {
    if (sensor_value.D0 & 0x01) {
      sprintf(temp, "A0=%s,", sensor_data_A[0]);
      strcat(data_send, temp);
      send_flag = 1;
      ct0 = rt_tick_get();
    }
    if (sensor_value.A0 == 0) {
        ct0 = 0;
    }
    lastA0 = sensor_value.A0;
  }
  if (mode == 1) {   // 模式1：振动、霍尔、火焰传感器选通
    if (lastA1 != sensor_value.A1 || (ct1 != 0 && rt_tick_get() > (ct1+3000))) {  // 振动传感器监测
      if (sensor_value.D0 & 0x02) {
        sprintf(temp, "A1=%s,", sensor_data_A[1]);
        strcat(data_send, temp);
        send_flag = 1;
        ct1 = rt_tick_get();
      }
      if (sensor_value.A1 == 0) {
        ct1 = 0;
      }
      lastA1 = sensor_value.A1;
    }
    if (lastA2 != sensor_value.A2 || (ct2 != 0 && rt_tick_get() > (ct2+3000))) {  // 霍尔传感器监测
      if (sensor_value.D0 & 0x04) {
        sprintf(temp, "A2=%s,", sensor_data_A[2]);
        strcat(data_send, temp);
        send_flag = 1;
        ct2 = rt_tick_get();
      }
      if (sensor_value.A2 == 0) {
        ct2 = 0;
      }
      lastA2 = sensor_value.A2;
    }
    
    if (lastA3 != sensor_value.A3 || (ct3 != 0 && rt_tick_get() > (ct3+3000))) {  // 火焰传感器监测
      if (sensor_value.D0 & 0x08) {
        sprintf(temp, "A3=%s,", sensor_data_A[3]);
        strcat(data_send, temp);
        send_flag = 1;
        ct3 = rt_tick_get();
      }
      if (sensor_value.A3 == 0) {
        ct3 = 0;
      }
      lastA3 = sensor_value.A3;
    }
  }
  if (lastA4 != sensor_value.A4 || (ct4 != 0 && rt_tick_get() > (ct4+3000))) {  // 燃气传感器监测
    if (sensor_value.D0 & 0x10) {
      sprintf(temp, "A4=%s,", sensor_data_A[4]);
      strcat(data_send, temp);
      send_flag = 1;
      ct4 = rt_tick_get();
    }
    if (sensor_value.A4 == 0) {
      ct4 = 0;
    }
    lastA4 = sensor_value.A4;
  }
  if (lastA5 != sensor_value.A5 || (ct5 != 0 && rt_tick_get() > (ct5+3000))) {  // 光栅传感器监测
    if (sensor_value.D0 & 0x20) {
      sprintf(temp, "A5=%s,", sensor_data_A[5]);
      strcat(data_send, temp);
      send_flag = 1;
      ct5 = rt_tick_get();
    }
    if (sensor_value.A5 == 0) {
      ct5 = 0;
    }
    lastA5 = sensor_value.A5;
  }

  //有异常时，发送一次异常数据
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
