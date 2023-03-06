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
#include "rf_thread.h"
#include "sensor_thread.h"

static struct rt_timer sensor_poll_timer;
static struct rt_semaphore sensor_poll_sem;
static void sensor_poll_timer_cb(void *parameter);

//存储A0-A7数据转成的字符串；每个字符串不超过11个有效字符；不存在的传感器应初始化为""
static char sensor_data_A[8][12] = {"0", "0", "0", "", "", "0", "0", "0"}; 
t_sensor_value sensor_value;      //传感器原始数据结构体
extern uint8_t debug_flag;

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
}

static uint8_t gas_init_flag, gas_consume;
static void updateA0(void) {
  //历史用气量（m3）
  static float gas_last;
  eMBMasterReqErrCode errcode;
  errcode = Modbus_Master_Send(8, 2, 0, NULL, 2);//03指令,设备地址,寄存器起始地址,寄存器数量
  if(errcode == MB_MRE_NO_ERR) {
    float gas_read = *(float*)&usMRegHoldBuf[1][0];
    if(debug_flag & 0x04) rt_kprintf("A0 MB Data: %d.%02d\n", (int)(gas_read), (int)(gas_read*100.f)%100);
    if(gas_init_flag == 0) {
      gas_init_flag = 1;
      gas_last = gas_read;
    }
    if(gas_read - gas_last > 0.001f) {
      sensor_value.A0 += gas_read - gas_last;
      gas_last = gas_read;
      gas_consume = 1;
      saveData_Update(); //A0变化时，存储到SPIFlash
    } else {
      gas_consume = 0;
    }
  } else {
    if(debug_flag & 0x04) rt_kprintf("A0 MB ERROR: %s\n", modbus_err_string[errcode]);
  }
  sprintf(sensor_data_A[0], "%.2f", sensor_value.A0);
}

#define GAS_MAX 101.3f
static void updateA1(void) {
  //气压（kPa，-GAS_MAX~GAS_MAX）
  uint16_t ad_cur_data = ad_Get_Sensor_Value(AD_CH_CUR);
  if(ad_cur_data < 820) sensor_value.A1 = 0;
  else {
    float pressure = (ad_cur_data / 4096.f * 3.f-0.6f) / (3.f-0.6f) * (GAS_MAX-(-GAS_MAX)) + (-GAS_MAX);//气压值
    pressure *= sensor_value.cali[1];
    if(pressure < 0.f) pressure = 0.f; //只考虑正气压
    else if(pressure > GAS_MAX) pressure = GAS_MAX; //数据超出限制：设为最大值
    sensor_value.A1 = pressure;
  }
  sprintf(sensor_data_A[1], "%.1f", sensor_value.A1);
}

static void updateA2(void) {
  //燃气浓度（ppm，0-20000）
  float vol = ad_Get_Sensor_Value(AD_CH_GAS) / 4096.f * 3.f * 2.f;//电压值：0-5V
  float gas;
  //分段函数计算燃气浓度
  if(vol < 0.625f) {            //(0,0)→(0.625,518)
    gas = vol/0.625f*518.f;
  } else if(vol < 2.16f) {      //(0.625,518)→(2.16,1490)
    gas = (vol-0.625f)/(2.16f-0.625f)*(1490.f-518.f)+518.f;
  } else if (vol < 2.625f) {    //(2.16,1490)→(2.625,2518)
    gas = (vol-2.16f)/(2.625f-2.16f)*(2518.f-1490.f)+1490.f;
  } else if (vol < 2.91f) {     //(2.625,2518)→(2.91,3528)
    gas = (vol-2.625f)/(2.91f-2.625f)*(3528.f-2518.f)+2518.f;
  } else if (vol < 3.2f) {      //(2.91,3528)→(3.2,5518)
    gas = (vol-2.91f)/(3.2f-2.91f)*(5518.f-3528.f)+3528.f;
  } else if (vol < 3.59f) {     //(3.2,5518)→(3.59,11037)
    gas = (vol-3.2f)/(3.59f-3.2f)*(11037.f-5518.f)+5518.f;
  } else if (vol < 3.764f) {    //(3.59,11037)→(3.764,15056)
    gas = (vol-3.59f)/(3.764f-3.59f)*(15056.f-11037.f)+11037.f;
  } else if (vol < 4.f) {       //(3.764,15056)→(4,20000)
    gas = (vol-3.764f)/(4.f-3.764f)*(20000.f-15056.f)+15056.f;
  } else {                      //(4,20000)→(~,20000)
    gas = 20000.f;
  }
  if(sensor_value.A7 == 1) {    //通过其他手段检测到管道泄漏时，模拟此时燃气变化
    gas += 5000.f;
    if(gas > 20000.f) gas = 20000.f;
  }
  sensor_value.A2 = (int16_t)gas;
  sprintf(sensor_data_A[2], "%d", sensor_value.A2);
}

static void updateA3(void) {
  
}

static void updateA4(void) {
  
}

static void updateA5(void) {
  int16_t xyz[3];
  ADXL345_GetValue(xyz);
  
  if(xyz[FIX_ANGLE] > 256) xyz[FIX_ANGLE] = 256;
  if(xyz[FIX_ANGLE] < -256) xyz[FIX_ANGLE] = -256;
  sensor_value.A5 = (int)fabs(acos(xyz[FIX_ANGLE]/256.0)*180.0/PI - (int)sensor_value.cali[5]);
  if(sensor_value.A5 > 180) return; //数据超出限制：不保存
  sprintf(sensor_data_A[5], "%d", sensor_value.A5);
}

static void updateA6(void) {
  //堵塞（0/1）：管道阀门、入户阀门都打开，用气量不变
  if((sensor_value.D1&0x03) == 0x03 && gas_consume == 0) {
    sensor_value.A6 = 1;
  } else {
    sensor_value.A6 = 0;
  }
  sprintf(sensor_data_A[6], "%u", sensor_value.A6);
}

static void updateA7(void) {
  //漏气（0/1）：入户阀门关闭时，用气量有变化
  if((sensor_value.D1&0x02) == 0x00 && gas_consume > 0) {
    sensor_value.A7 = 1;
  } else {
    sensor_value.A7 = 0;
  }
  sprintf(sensor_data_A[7], "%u", sensor_value.A7);
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
    saveData_Update(); //D0发生改变时，存储
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
    saveData_Update(); //D1发生改变时，存储
    sensorControl(sensor_value.D1);//执行D1相关控制操作
  }
}

static void updateV0(char *pval) {
  uint32_t value = atoi(pval);
  if(value == 0 || value > 65535) return; //数据超出限制：不保存
  if(sensor_value.V0 != value) {
    sensor_value.V0 = value;
    saveData_Update();
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
  if(strcmp(sensor_value.V3, pval) != 0) {
    strncpy(sensor_value.V3, pval, sizeof(sensor_value.V3));
    sensor_value.V3[sizeof(sensor_value.V3)-1] = 0;
    saveData_Update();
  }
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
      rt_kprintf("%s\n", data_send);
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
  static uint16_t A6_count = 0, A7_count = 0;
  static uint32_t A6_last;
  static int A7_last;
  
  char data_send[100] = "{";
  char temp[20];
  uint8_t send_flag = 0;
  
  if((sensor_value.D0 & (1<<6)) != 0) {     //堵塞状态
    if(sensor_value.A6 == 1 && ++A6_count >= 100) {
      A6_last = sensor_value.A6;
      sprintf(temp, "A6=%s,", sensor_data_A[6]);
      strcat(data_send, temp);
      A6_count = 0;
      send_flag = 1;
    } else if(sensor_value.A6 == 0) {
      A6_count = 0; //A6变为0时，清空计数
      if(A6_last == 1) {
        A6_last = sensor_value.A6;
        sprintf(temp, "A6=%s,", sensor_data_A[6]);
        strcat(data_send, temp);
        send_flag = 1;
      }
    }
  }
  if((sensor_value.D0 & (1<<7)) != 0) {     //漏气状态
    if(sensor_value.A7 == 1 && ++A7_count >= 100) {
      A7_last = sensor_value.A7;
      sprintf(temp, "A7=%s,", sensor_data_A[7]);
      strcat(data_send, temp);
      A7_count = 0;
      send_flag = 1;
    } else if(sensor_value.A7 == 0) {
      A7_count = 0;
      if(A7_last == 1) {
        A7_last = sensor_value.A7;
        sprintf(temp, "A7=%s,", sensor_data_A[7]);
        strcat(data_send, temp);
        send_flag = 1;
      }
    }
  }
  
  //有异常时，每3秒发送一次异常数据
  if(strlen(data_send) > 2) {
    if(send_flag == 1) {
      data_send[strlen(data_send)-1] = '}';
      rf_send((uint8_t *)data_send, strlen(data_send), 200);
      rt_kprintf("%s\n", data_send);
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
  if((cmd&0x01) != 0) { //控制继电器1
    relay_on(1);
  } else {
    relay_off(1);
  }
  if((cmd&0x02) != 0) { //控制继电器2
    relay_on(2);
  } else {
    relay_off(2);
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
        case 0: updateD0(1, value); break;
        case 1: updateD1(1, value); break;
        default: return -1;
      }
      return 0;
    }
  } else if(strncmp(ptag, "CD", 2) == 0) {  //{CDx=y}
    if (ptag[2] >= '0' && ptag[2] <= '1') {
      uint8_t tag = atoi(&ptag[2]);
      uint8_t value = atoi(pval);//*pval为数字时，value为要设置的值；*pval不为数字时，atoi返回0，也不会改变当前值
      switch(tag) {
        case 0: updateD0(2, value); break;
        case 1: updateD1(2, value); break;
        default: return -1;
      }
      return 0;
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
  if(*(uint32_t *)&sensor_value.A0 == 0xFFFFFFFF) sensor_value.A0 = 0.f;//历史用气量
  sensor_value.A1 = 0;
  sensor_value.A2 = 0;
  sensor_value.A6 = 0;
  sensor_value.A7 = 0;
  if(sensor_value.D1 == 0xFF) sensor_value.D1 = 0x01;
  if(sensor_value.V0 == 0xFFFF || sensor_value.V0 == 0) sensor_value.V0 = 30;
  if(sensor_value.V3[0] == 0xFF) strcpy(sensor_value.V3, DEFAULT_POS);
  if(*(uint32_t *)&sensor_value.cali[1] == 0xFFFFFFFF) sensor_value.cali[1] = 1.f;
  if(*(uint32_t *)&sensor_value.cali[5] == 0xFFFFFFFF) sensor_value.cali[5] = 0.f;
}

/* 传感器校准 */
void sensor_cali(uint8_t ch, int value) {
  uint16_t ad_cur_data;
  float pressure;
  int16_t xyz[3];
  
  switch(ch) {
    case 1: //气压--记录(value/10)与测量值的比率
      ad_cur_data = ad_Get_Sensor_Value(AD_CH_CUR);
      pressure = (ad_cur_data / 4096.f * 3.f-0.6f) / (3.f-0.6f) * (GAS_MAX-(-GAS_MAX)) + (-GAS_MAX);//气压值
      if(value > 0 && pressure > 0.001f) {   //保证value是正整数;避免除数是0、是负数
        sensor_value.cali[1] = (float)value / 10.f / pressure;
        saveData_Update();
      }
    case 5: //倾倒角度--忽略value，将当前角度设为0度
      ADXL345_GetValue(xyz);
      sensor_value.cali[5] = fabs(acos(xyz[FIX_ANGLE]/256.0)*180.0/PI);
      saveData_Update();
    break;
    default:
    break;
  }
}

static void sensor_poll_timer_cb(void *parameter) {
  //定时器达到V0时间间隔后，发送信号，让sensorUpdate函数发送数据
  rt_sem_release(&sensor_poll_sem);
}
