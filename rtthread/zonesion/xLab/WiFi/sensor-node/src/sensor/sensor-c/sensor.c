/*********************************************************************************************
* �ļ���sensor.c
* ���ߣ�
* ˵����ͨ�ô��������ƽӿڳ���
* �޸ģ�
* ע�ͣ�
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

//�洢A0-A7����ת�ɵ��ַ�����ÿ���ַ���������11����Ч�ַ��������ڵĴ�����Ӧ��ʼ��Ϊ""
static char sensor_data_A[8][12] = {"0", "0", "0", "0", "0", "0", "", ""}; 
t_sensor_value sensor_value;      //������ԭʼ���ݽṹ��
static uint8_t mode = 1; 

/*********************************************************************************************
* ���ƣ�sensor_init()
* ���ܣ�
* ������
* ���أ�
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
void sensor_init(void) {
  /*******������ʱ���Ͷ�ʱ������ʱ���ΪV0*****/
  rt_timer_init(&sensor_poll_timer, "sensor poll tim",
                sensor_poll_timer_cb, RT_NULL,
                sensor_value.V0*1000, RT_TIMER_FLAG_PERIODIC);
  rt_sem_init(&sensor_poll_sem, "sensor poll sem", 0, RT_IPC_FLAG_FIFO);
  /***************��ʼ����������***************/
  hall_init();                                                // ������������ʼ��
  infrared_init();                                            // ������⴫������ʼ��
  vibration_init();                                           // �𶯴�������ʼ��
  flame_init();                                               // ���洫������ʼ��
  combustiblegas_init();                                      // ȼ����������ʼ��
  grating_init();                                             // ��դ��������ʼ��
  relay_init();                                               // �̵�����ʼ��
  sensorControl(sensor_value.D1);
  rt_timer_start(&sensor_poll_timer);       //������ʱ����ÿ��V0�����ϱ�һ������
  rt_thread_startup(&sensor_check_thread);  //���ڰ����ഫ����ʱ�����������ഫ�����߳�
}

static void updateA0(void) {
  static uint32_t ct = 0;
  uint8_t  A0 = 0;
   
  if (mode == 1) {                                               // �����������/������������ֵ
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
  
  if (mode == 1) {                                              // �����񶯴�������ֵ
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
    sensor_value.A2 = get_hall_status();                                     // ���»�����������ֵ
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

/* ����������-D0
 * ������type��1--OD������λ����2--CD�����λ����value��Ҫ���û������λ */
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

/* ����������-D1
 * ������type��1--OD������λ����2--CD�����λ����value��Ҫ���û������λ */
static void updateD1(uint8_t type, uint8_t value) {
  uint8_t temp;
  if(type == 1) {//OD
    temp = sensor_value.D1 | value;
  } else if(type == 2) {//CD
    temp = sensor_value.D1 & (~value);
  }
  if(sensor_value.D1 != temp) {
    sensor_value.D1 = temp;
    sensorControl(sensor_value.D1);//ִ��D1��ؿ��Ʋ���
  }
}

static void updateV0(char *pval) {
  uint32_t value = atoi(pval);
  if(value == 0 || value > 65535) return; //���ݳ������ƣ�������
  if(sensor_value.V0 != value) {
    sensor_value.V0 = value;
    //���¶�ʱ�����
    value *= 1000;
    rt_timer_control(&sensor_poll_timer, RT_TIMER_CTRL_SET_TIME, (void *)&value);
    rt_timer_start(&sensor_poll_timer);//������ʱ��
  }
}

static void updateV1(char *pval) {
}

static void updateV2(char *pval) {
}

static void updateV3(char *pval) {
}

/*********************************************************************************************
* ���ƣ�sensorLinkOn()
* ���ܣ��������ڵ������ɹ����ú���
* ��������
* ���أ���
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
void sensorLinkOn(void) {
  rt_sem_release(&sensor_poll_sem);         //�����źţ���sensorUpdate�����ϱ�һ������
}

/*��ѯ��������ֵ���ﵽV0�趨ʱ��ʱ�ϱ�*/
void sensorUpdate(void) {
  rt_err_t err = rt_sem_take(&sensor_poll_sem, 1000);
  if(err == RT_EOK) {//ͨ����ʱ����ÿ��V0ʱ�䣬�յ���ʱ���������źţ�����һ������
    //���ɷ����ַ���
    char data_send[160] = "{";
    char temp[20];
    for(uint8_t i = 0; i < 8; i++) {
      if(strlen(sensor_data_A[i]) > 0) {//�Ǳ��ڵ��ϴ��ڵĴ�����
        if((sensor_value.D0 & (1<<i)) != 0) { //D0��ʹ�ܸô������Զ��ϱ�
          sprintf(temp, "A%u=%s,", i, sensor_data_A[i]);
          strcat(data_send, temp);
        }
      }
    }
    sprintf(temp, "D1=%u,", sensor_value.D1);
    strcat(data_send, temp);
    //�������ݵ�����/������
    if(strlen(data_send) > 1) {
      data_send[strlen(data_send)-1] = '}';
      //�������ݵ�����/������
      rf_send((uint8_t *)data_send, strlen(data_send), 200);
    }
  }
}

/*********************************************************************************************
* ���ƣ�sensorCheck()
* ���ܣ���������⣬�����ഫ�����쳣ʱ���ٷ�����Ϣ
* ��������
* ���أ���
* �޸ģ�
* ע�ͣ�
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
      || ( //ģʽ1��������⴫����ѡͨ��������ѭ��3S�ϱ���ģʽ2������������ѡͨ��״̬��ת���ϱ�1�Σ�  
         (mode == 1)&& ct0 != 0 && rt_tick_get() > (ct0+3000)) //����3���ϱ�
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
  if (mode == 1) {   // ģʽ1���񶯡����������洫����ѡͨ
    if (lastA1 != sensor_value.A1 || (ct1 != 0 && rt_tick_get() > (ct1+3000))) {  // �񶯴��������
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
    if (lastA2 != sensor_value.A2 || (ct2 != 0 && rt_tick_get() > (ct2+3000))) {  // �������������
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
    
    if (lastA3 != sensor_value.A3 || (ct3 != 0 && rt_tick_get() > (ct3+3000))) {  // ���洫�������
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
  if (lastA4 != sensor_value.A4 || (ct4 != 0 && rt_tick_get() > (ct4+3000))) {  // ȼ�����������
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
  if (lastA5 != sensor_value.A5 || (ct5 != 0 && rt_tick_get() > (ct5+3000))) {  // ��դ���������
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

  //���쳣ʱ������һ���쳣����
  if(strlen(data_send) > 2) {
    if(send_flag == 1) {
      data_send[strlen(data_send)-1] = '}';
      rf_send((uint8_t *)data_send, strlen(data_send), 200);
    }
  }
  rt_thread_mdelay(100);
}

/*********************************************************************************************
* ���ƣ�sensorControl()
* ���ܣ�����������
* ������cmd - ��������
* ���أ���
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
void sensorControl(uint8_t cmd) {
  relay_control(cmd>>6);
}

/*********************************************************************************************
* ���ƣ�z_process_command_call()
* ���ܣ������ϲ�Ӧ�÷�������ָ��
* ������ptag: ָ���ʶ D0��D1�� A0 ...
*       pval: ָ��ֵ�� ��?����ʾ��ȡ��
*       obuf: ָ�������ŵ�ַ
* ���أ�>0ָ������������ݳ��ȣ�0��û�з������ݣ�<0����֧��ָ�
* �޸ģ�
* ע�ͣ�
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
      uint8_t value = atoi(pval);//*pvalΪ����ʱ��valueΪҪ���õ�ֵ��*pval��Ϊ����ʱ��atoi����0��Ҳ����ı䵱ǰֵ
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
      uint8_t value = atoi(pval);//*pvalΪ����ʱ��valueΪҪ���õ�ֵ��*pval��Ϊ����ʱ��atoi����0��Ҳ����ı䵱ǰֵ
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

/* K3����������Ĵ��� */
void sensor_Keypressed(void) {
  rt_sem_release(&sensor_poll_sem);
}

/* ���в�����ʼ��--����δ��ȡ���洢������ֵ����� */
void sensor_para_init(void) {
  sensor_value.D0 = 0xFF;
  if(sensor_value.D1 == 0xFF) sensor_value.D1 = 0x0;
  if(sensor_value.V0 == 0xFFFF || sensor_value.V0 == 0) sensor_value.V0 = 30;
}

/* ������У׼ */
void sensor_cali(uint8_t ch, int value) {
}

static void sensor_poll_timer_cb(void *parameter) {
  //��ʱ���ﵽV0ʱ�����󣬷����źţ���sensorUpdate������������
  rt_sem_release(&sensor_poll_sem);
}
