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
#include "oled.h"
#include "7941w.h"
#include "buzzer.h"
#include "drv_key/key.h"
#include "drv_relay/relay.h"
#include "math.h"
#include "rf_thread.h"
#include "rf_if_app.h"
#include "sensor_thread.h"

#define KEY     "\xFF\xFF\xFF\xFF\xFF\xFF"

static struct rt_timer sensor_poll_timer;
static struct rt_semaphore sensor_poll_sem;
static void sensor_poll_timer_cb(void *parameter);

//存储A0-A7数据转成的字符串；每个字符串不超过11个有效字符；不存在的传感器应初始化为""
static char sensor_data_A[8][16] = {"", "", "", "0", "0", "0", "", ""}; 
t_sensor_value sensor_value;      //传感器原始数据结构体
static int work_mod = 2; //1 设备充值模式, 2 读卡模式,上位机可以给卡片充值
static char A0[16];                                              // A0存储卡号
static int V1 = 0, V2 = 0, V3 = 0, V4 = 0; 
static uint16_t seed = 0;

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
  //随机数种子初始化
  extern t_rf_info rf_info;
  for(uint8_t i = 0; i < strlen(rf_info.mac); i++) {
    seed += rf_info.mac[i];
  }
  srand(seed);
  //传感器初始化
  RFID7941Init();
  key_init();
  relay_init();
  buzzer_init();
  OLED_Init();
  if(work_mod == 1){
    OLED_ShowCHinese(21+13*3,0,6);
    OLED_ShowCHinese(21+13*4,0,7);
  }
  else{
    OLED_ShowCHinese(21+13*3,0,10);
    OLED_ShowCHinese(21+13*4,0,11);
  }
  char buf[16];
  sprintf(buf, "%d.%d", sensor_value.A3/100, sensor_value.A3%100);
  OLED_ShowString(21+13*3,2,(unsigned char *)buf,12); 
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
  V1 = (int)(v * 100);
}

static void updateV2(char *pval) {
  float v = atof(pval);
  V2 = (int)(v * 100);
  if (V2 > sensor_value.A2) {
    V2 = 0;
  }
}

static void updateV3(char *pval) {
  float v = atof(pval); 
  sensor_value.A3 =  sensor_value.A3 + (int)(v * 100);
  sensor_value.D1 |= 2;
  V3 = 1;
  relay_control(sensor_value.D1);
}

static void updateV4(char *pval) {
  float v = atof(pval);
  int x = (int)(v * 1000);
  if (sensor_value.A3 >= x) {
    sensor_value.A3 =  sensor_value.A3 - (int)(v * 100);
    V4 = 1;
    if (sensor_value.A3 == 0) {
      sensor_value.D1 &= ~2;
      relay_control(sensor_value.D1);
    }
  } else {
    V4 = 0;
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
  rt_err_t err = rt_sem_take(&sensor_poll_sem, 1000);
  if(err == RT_EOK) {//通过定时器，每隔V0时间，收到定时器发来的信号，发送一次数据
    if (sensor_value.A3 > 0) {
      sensor_value.A4 = 1 + rand()%500;                         // 设备本次消费扣款金额，通过随机数产生
      if (sensor_value.A4 > sensor_value.A3) sensor_value.A4 = sensor_value.A3;
      sensor_value.A3 = (sensor_value.A3 - sensor_value.A4);    // 设备余额
      sensor_value.A5 = (sensor_value.A5 + sensor_value.A4);    // 设备累计消费金额
      
      sprintf(sensor_data_A[3], "%d.%d", sensor_value.A3/100, sensor_value.A3%100);   // 气表余额
      sprintf(sensor_data_A[4], "%d.%d", sensor_value.A4/100, sensor_value.A4%100);   // 气表本次消费扣款金额
      sprintf(sensor_data_A[5], "%d.%d", sensor_value.A5/100, sensor_value.A5%100);   // 气表累积消费金额
      OLED_ShowString(21+13*3,2,(unsigned char *)sensor_data_A[3],12);  // oLED显示气表余额
      if(sensor_value.A3 == 0) {    //余额不足扣费，继电器断开（模拟气表设备断开）
        sensor_value.D1 &= ~2;
        relay_control(sensor_value.D1);
      }
    }
    
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
  static uint8_t check_init_flag = 0;
  char data_send[100] = "{";
  char temp[20];
  char buff[16];
  
  if(check_init_flag == 0) {
    srand(seed);
    check_init_flag = 1;
  }
  
  uint8_t k = get_key_status();
  static uint8_t last_key = 0;
  if (k & 0x01 == 0x01) {
    if ((last_key & 0x01) == 0) {
      if (work_mod == 1) {
          OLED_ShowCHinese(21+13*3,0,10);
          OLED_ShowCHinese(21+13*4,0,11);
          work_mod = 2;
      } else {
        OLED_ShowCHinese(21+13*3,0,6);
        OLED_ShowCHinese(21+13*4,0,7);
        work_mod = 1;
      }
    }
  } else if ((k & 0x02) == 0x02) {
    if ((last_key & 0x02) == 0) {
      //模拟消费
      if (sensor_value.A3 > 0) {
        sensor_value.A4 = rand()%500;                                          // 设备本次消费扣款金额，通过随机数产生
        if (sensor_value.A4 > sensor_value.A3) sensor_value.A4 = sensor_value.A3;
        sensor_value.A3 = sensor_value.A3 - sensor_value.A4;
        sensor_value.A5 = sensor_value.A5 + sensor_value.A4;
        sprintf(temp, "A3=%d.%d,", sensor_value.A3/100, sensor_value.A3%100);   // 气表余额
        strcat(data_send, temp);
        sprintf(temp, "A4=%d.%d,", sensor_value.A4/100, sensor_value.A4%100);   // 气表本次消费扣款金额
        strcat(data_send, temp);
        sprintf(temp, "A5=%d.%d,", sensor_value.A5/100, sensor_value.A5%100);   // 气表累积消费金额
        strcat(data_send, temp);
        sprintf(buff, "%d.%d   ", sensor_value.A3/100, sensor_value.A3%100);
        OLED_ShowString(21+13*3,2,(unsigned char *)buff,12);
        if(sensor_value.A3 == 0) {   //余额不足扣费，继电器断开（模拟气表设备断开）
          sensor_value.D1 &= ~2;
          relay_control(sensor_value.D1);
        }
      }
    }
  }
  last_key = k;
  //  按键处理完毕
  
  // 读卡处理
  static char status = 1;
  static char last_cid[7];
  static char card_cnt = 0;                                     // 卡片检测计数
  static char cid[7];
  static char write = 0;
  
  if (status == 1) {                                            // 检测125k读卡结果
    buzzer_off();
    if (RFID7941CheckCard125kHzRsp(cid) > 0) {
      if (memcmp(last_cid, cid, 5) != 0) {
        sprintf(A0, "%02X%02X%02X%02X%02X", cid[0],cid[1],cid[2],cid[3],cid[4]);
        sprintf(temp, "A0=%s,", A0);
        strcat(data_send, temp);
        sensor_value.A1 = 0;
        sprintf(temp, "A1=%d,", sensor_value.A1);
        strcat(data_send, temp);
        memcpy(last_cid, cid, 5);
        card_cnt = 5;
        buzzer_on(); 
      } else { //同一张卡片
        card_cnt = 5;
      }
      RFID7941CheckCard125kHzReq();
    } else {
      if (card_cnt > 0) {
        card_cnt -= 1;
      }
      if (card_cnt == 0) {
        memset(last_cid, 0, sizeof last_cid);
        RFID7941CheckCard1356MHzReq();                          // 检测13.56M卡片
        status = 2;
      } else {
        RFID7941CheckCard125kHzReq();
      }
    }
  } else if (status == 2) {                                     // 13.56卡片检测处理
    buzzer_off();
    if (RFID7941CheckCard1356MHzRsp(cid) > 0) {
      if (memcmp(last_cid, cid, 4) != 0) {
        RFID7941ReadCard1356MHzReq(8, KEY);
        status = 3;
      } else {
        card_cnt = 5;                                           // 刷新卡片计算
        if (work_mod == 1) {                                    // 充值设备
          if (write == 3 && sensor_value.A2 > 0) {
            memset(buff, 0, 16);
            RFID7941WriteCard1356MHzReq(8, KEY, buff);
            status = 4;
          } else {
            RFID7941CheckCard1356MHzReq();
          }
        } else
        if (work_mod == 2) {                                    // 读卡模式
          int money;
          if (V1 != 0) {                                        // 充值
            money = sensor_value.A2 + V1;    
            write = 1;
          } else if (V2 != 0) {
            money = sensor_value.A2 - V2;
            write = 2;
          }
          if (V1 != 0 || V2 != 0) {
            memset(buff, 0, 16);
            buff[12] = (money>>24) & 0xff;
            buff[13] = (money>>16) & 0xff;
            buff[14] = (money>>8) & 0xff;
            buff[15] = money&0xff;
            RFID7941WriteCard1356MHzReq(8, KEY, buff);
            status = 5;
          } else {
            RFID7941CheckCard1356MHzReq();
          }
        } else {
          RFID7941CheckCard1356MHzReq();
        }
      }
    } else {
      if (card_cnt > 0) {
        card_cnt -= 1;
      }
      if (card_cnt == 0) {
        memset(last_cid, 0, sizeof last_cid);
        RFID7941CheckCard125kHzReq();
        status = 1;
        sensor_value.A2 = 0;                                                 // 余额清零
      } else {
        RFID7941CheckCard1356MHzReq();
      }
    }
  } else if (status == 3) {                                     // 处理卡片读取结果
    if (RFID7941ReadCard1356MHzRsp(buff) > 0) {                 // 读取到余额，保存当前卡片id
      memcpy(last_cid, cid, 4);
      card_cnt = 5;      
      sprintf(A0, "%02X%02X%02X%02X", cid[0],cid[1],cid[2],cid[3]);  
      int money = ((uint32_t)buff[12]<<24) | ((uint32_t)buff[13]<<16) | (buff[14]<<8) | (buff[15]);
      sensor_value.A2 = money;
      if (work_mod == 1) {                                      // 设备充值模式
        if (money > 0) {
          buff[12] = buff[13] = buff[14] = buff[15] = 0;
          write = 3;                                            // 再次检测卡片后通过write = 3 来写卡
          RFID7941CheckCard1356MHzReq();
          status = 2;
        } else {
          buzzer_on();                          
          RFID7941CheckCard1356MHzReq();
          status = 2;
        }
      } else {  //work_mod == 2;
        sprintf(temp, "A0=%s,", A0);
        strcat(data_send, temp);
        sprintf(temp, "A1=1,");
        strcat(data_send, temp);
        sprintf(temp, "A2=%d.%d,", sensor_value.A2/100, sensor_value.A2%100);
        strcat(data_send, temp);
        V1 = 0;
        V2 = 0;
        buzzer_on();                          //
        RFID7941CheckCard1356MHzReq();
        status = 2;
      }
    } else {
      RFID7941CheckCard1356MHzReq();                            // 重新检测卡片
      status = 2;
    }
  } else if (status == 4) {                                     // 处理充值设备写卡结果
      if (RFID7941WriteCard1356MHzRsp() > 0) {
        //充值设备成功
        sensor_value.A3 += sensor_value.A2;
        sensor_value.A2 = 0;
        sprintf(buff, "%d.%d   ", sensor_value.A3/100,sensor_value.A3%100);
        OLED_ShowString(21+13*3,2,(unsigned char *)buff,12);
        
        write = 0;
        sensor_value.D1 |= 2;
        relay_control(sensor_value.D1);
        
        buzzer_on();
      } else { //写卡失败
        //memset(last_cid, 0, sizeof last_cid);
        //card_cnt = 0;
      }
      RFID7941CheckCard1356MHzReq();
      status = 2;
  } else if (status == 5) {                                     // 卡片充值扣费结果检测
    if (RFID7941WriteCard1356MHzRsp() > 0) {
      if (write == 1) {
        sprintf(temp, "V1=1,");
        strcat(data_send, temp);
        sensor_value.A2 += V1;
        V1 = 0;
        sprintf(temp, "A2=%d.%d,", sensor_value.A2/100, sensor_value.A2%100);
        strcat(data_send, temp);
        write = 0;
      } else if (write == 2) {
        sprintf(temp, "V2=1,");
        strcat(data_send, temp);
        sensor_value.A2 -= V2;
        V2 = 0;
        sprintf(temp, "A2=%d.%d,", sensor_value.A2/100, sensor_value.A2%100);
        strcat(data_send, temp);
        write  = 0;
      }
      buzzer_on();
    }
    RFID7941CheckCard1356MHzReq();
    status = 2;
  }
  
  //有异常时，每3秒发送一次异常数据
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
  relay_control(cmd);
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
    if (ptag[1] >= '0' && ptag[1] <= '4') {
      uint8_t tag = atoi(&ptag[1]);
      if (pval[0] == '?') { //{Vx=?}
        switch(tag) {
          case 0: sprintf(obuf, "%u", sensor_value.V0); break;
          default: return -1;
        }
        return strlen(obuf);
      } else {  //{Vx=y}
        switch(tag) {
          case 0: updateV0(pval); break;
          case 1: updateV1(pval); break;
          case 2:
            updateV2(pval);
            sprintf(obuf, "%u", V2);
            break;
          case 3:
            updateV3(pval);
            sprintf(obuf, "%u", V3);
            break;
          case 4: 
            updateV4(pval); 
            sprintf(obuf, "%u", V4);
            break;
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
  sensor_value.A3 = 10000;
  sensor_value.D0 = 0xFF;
  sensor_value.D1 = 2;
  sensor_value.V0 = 30;
  strcpy(sensor_value.V1, "0");
  strcpy(sensor_value.V2, "0");
}

/* 传感器校准 */
void sensor_cali(uint8_t ch, int value) {
}

static void sensor_poll_timer_cb(void *parameter) {
  //定时器达到V0时间间隔后，发送信号，让sensorUpdate函数发送数据
  rt_sem_release(&sensor_poll_sem);
}
