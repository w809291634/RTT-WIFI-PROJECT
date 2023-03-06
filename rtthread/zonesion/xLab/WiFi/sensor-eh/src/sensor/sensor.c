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
#include "rfid900m.h"
#include "motor.h"
#include "drv_relay/relay.h"
#include "math.h"
#include "rf_thread.h"
#include "rf_if_app.h"
#include "sensor_thread.h"

static struct rt_timer sensor_poll_timer;
static struct rt_semaphore sensor_poll_sem;
static void sensor_poll_timer_cb(void *parameter);

//存储A0-A7数据转成的字符串；每个字符串不超过11个有效字符；不存在的传感器应初始化为""
t_sensor_value sensor_value;      //传感器原始数据结构体
static int motor = 0;
static char A0[32];                                              // A0存储卡号
static int V1;
static int V2;

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
  //传感器初始化
  motorInit();
  RFID900MInit();
  sensorControl(sensor_value.D1);   //开机恢复此前控制状态
  rt_timer_start(&sensor_poll_timer);       //开启定时器，每隔V0主动上报一次数据
  rt_thread_startup(&sensor_check_thread);  //存在安防类传感器时，开启安防类传感器线程
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
  float v = atof(pval);
  V1 = (long) (v *100);
}

static void updateV2(char *pval) {
  float v = atof(pval);
  V2 = (long) (v *100);
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
  char temp[40];
  
  static char last_epc[12];
  static int new_tag = 0;                       //检测到卡片并读取余额成功标记
  static char epc[12];                                    //记录当前检测到卡片epc
  static char write = 0;
  static char status = 1;                              //状态转换标识
  
  if (status == 1) {
    if (RFID900MCheckCardRsp(epc) > 0) {
      if (memcmp(last_epc, epc, 12) != 0)  {
        RFID900MReadReq(NULL, epc, BLK_USER, 0, 2); //发送读卡请求
        status = 2;       
      } else {
        
        new_tag = 8;
        if (V1 != 0) {
          long money = sensor_value.A2 + V1;
          RFID900MWriteReq(NULL, epc, BLK_USER, 0, 2, (char*)&money);
          write = 1;
          status = 3;
        } else if (V2 != 0) {
          long money = sensor_value.A2 - V2;
          RFID900MWriteReq(NULL, epc, BLK_USER, 0, 2, (char*)&money);
          write = 2;
          status = 3;
        } else {
          RFID900MCheckCardReq();
          status = 1;
        }
      }
    } else {
      //没有检测到卡片
      if (new_tag > 0) {
        new_tag -= 1;
        if (new_tag == 0) {
          memset(last_epc, 0, 12);
        }
      }
      RFID900MCheckCardReq();
      status = 1;
    }
  } else if (status == 2) {
    int money;
    if (RFID900MReadRsp((char*)&money) > 0) {  
      //读取到金额上报
      for (int j=0; j<12; j++) sprintf(&A0[j*2], "%02X", epc[j]);
      if (money < 0) money = 0;
      sprintf(temp, "A0=%s,", A0);
      strcat(data_send, temp);
      sensor_value.A2 = money;
      sprintf(temp, "A2=%ld.%ld,", sensor_value.A2/100, sensor_value.A2%100);
      strcat(data_send, temp);
      memcpy(last_epc, epc, 12);  //保存当前卡片id
      new_tag = 8;
      V1 = 0;  //初始化扣金额
      V2 = 0;
    }

    RFID900MCheckCardReq();
    status = 1;
    
  } else if (status == 3) {
    if (RFID900MWriteRsp() > 0) {
      if (write == 1) {
        sensor_value.A2 = sensor_value.A2 + V1;
        V1 = 0;
        sprintf(temp, "V1=1,");
        strcat(data_send, temp);
      } else if (write == 2) {
        sensor_value.A2 = sensor_value.A2 - V2;
        V2 = 0;
        sprintf(temp, "V2=1,");
        strcat(data_send, temp);
      }
      sprintf(temp, "A2=%d.%d,", sensor_value.A2/100, sensor_value.A2%100);
      strcat(data_send, temp);
    } 
    RFID900MCheckCardReq();
    status = 1;
  }
  
    /*增加马达状态检测*/
  static int tick = 0;
  if (tick == motor){
    motorSet(0);                //关闭马达
  }else if (tick > motor) {
    motorSet(2);                //开启闸机
    tick -= 1;
  } else if (tick < motor) {
    motorSet(1);                //关闭闸机
    tick += 1;
  }
  
  //发送数据
  if(strlen(data_send) > 2) {
    data_send[strlen(data_send)-1] = '}';
    rf_send((uint8_t *)data_send, strlen(data_send), 200);
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
  if (cmd & 0x01) {
    motor = 3; //闸机开启时间300ms
  } else {
    motor = 0;
  }
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
  if (0 == strcmp("A2", ptag)) {
    if (pval[0] == '?'){
      sprintf(obuf, "%d.%d", sensor_value.A2/100, sensor_value.A2%100);
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
  sensor_value.D1 = 0;
  sensor_value.V0 = 30;
}

/* 传感器校准 */
void sensor_cali(uint8_t ch, int value) {
}

static void sensor_poll_timer_cb(void *parameter) {
  //定时器达到V0时间间隔后，发送信号，让sensorUpdate函数发送数据
  rt_sem_release(&sensor_poll_sem);
}
