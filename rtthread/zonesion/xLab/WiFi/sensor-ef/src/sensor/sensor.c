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
#include "math.h"
#include "rf_thread.h"
#include "sensor_thread.h"

static struct rt_timer sensor_poll_timer;
static struct rt_semaphore sensor_poll_sem;
static void sensor_poll_timer_cb(void *parameter);

//�洢A0-A7����ת�ɵ��ַ�����ÿ���ַ���������11����Ч�ַ��������ڵĴ�����Ӧ��ʼ��Ϊ""
static char sensor_data_A[8][12] = {"", "", "", "", "", "", "", ""}; 
t_sensor_value sensor_value;      //������ԭʼ���ݽṹ��
static uint8_t  mode = 0;                                         // ���Ʊ�ʶ��

/*********************************************************************************************
* ���ƣ�sensor_init()
* ���ܣ�
* ������
* ���أ�
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
void sensor_init(void) {
//  /*******������ʱ���Ͷ�ʱ������ʱ���ΪV0*****/
//  rt_timer_init(&sensor_poll_timer, "sensor poll tim",
//                sensor_poll_timer_cb, RT_NULL,
//                sensor_value.V0*1000, RT_TIMER_FLAG_PERIODIC);
//  rt_sem_init(&sensor_poll_sem, "sensor poll sem", 0, RT_IPC_FLAG_FIFO);
//  /***************��ʼ����������***************/
//  if(mode_read() == 0){
//    mode = 1;
//    stepmotor_init(); 
//    FAN_init();
//  } else {
//    mode = 2;
//    rgb_init();
//    Beep_init();
//  }
//  ext_led_init();
//  relay_init();
//  sensorControl(sensor_value.D1);
//  rt_timer_start(&sensor_poll_timer);       //������ʱ����ÿ��V0�����ϱ�һ������
//  rt_thread_startup(&sensor_check_thread);  //���ڰ����ഫ����ʱ�����������ഫ�����߳�
}

static void updateA0(void) {
}

static void updateA1(void) {
}

static void updateA2(void) {
}

static void updateA3(void) {
}

static void updateA4(void) {
}

static void updateA5(void) {
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
  updateA0();
  updateA1();
  updateA2();
  updateA3();
  updateA4();
  updateA5();
  updateA6();
  updateA7();
  
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
  uint8_t send_flag = 0;

  //���쳣ʱ��ÿ3�뷢��һ���쳣����
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
  static uint8_t stepmotor_flag = 0;
  if(mode == 1){                                                  // ������ģʽһ����
    if ((cmd & 0x04) == 0x04) {                                   // �����������λ��bit2
      if(stepmotor_flag != 1) {                                   // ���������ת
        stepmotor_flag = 1; 
//        forward(5000);
      }
    } else {                                                      // ���������ת
      if(stepmotor_flag != 0) {
        stepmotor_flag = 0; 
//        reversion(5000);
      }
    }
    if(cmd & 0x08){                                   // ����cmd���������Ӧ�Ŀ��Ƴ���
//      FAN_on(0x01);                                             // ��������
    }
    else{
//      FAN_off(0x01);                                            // �رշ���       
    }
  }

  if(mode == 2){                                                  // ������ģʽ������
    if ((cmd & 0x01) == 0x01){                                    // RGB�ƿ���λ��bit0~bit1
      if ((cmd & 0x02) == 0x02){                                  // cmdΪ3��������
//        rgb_off(0x01);                                          
//        rgb_off(0x02);                                         
//        rgb_on(0x04);                                          
      }
      else{                                                       // cmdΪ1�������
//        rgb_on(0x01);                                         
//        rgb_off(0x02);                                          
//        rgb_off(0x04);                                        
        
      }
    }
    else if ((cmd & 0x02) == 0x02){                               // cmdΪ2�����̵�
//      rgb_off(0x01);                                           
//      rgb_on(0x02);                                            
//      rgb_off(0x04);                                           
    }
    else{                                                         // cmdΪ1����ȫ��
//      rgb_off(0x01);                                           
//      rgb_off(0x02);                                           
//      rgb_off(0x04);                                           
    }
    if(cmd & 0x08){                                             // ����������λ��bit3
//      Beep_on(0x01);                                            // ����������
    }
    else{
//      Beep_off(0x01);                                           // �رշ�����       
    }
  }
  if(cmd & 0x20){                                                // LED2�ƿ���λ��bit5
//    ext_led_on(0x02);                                                //����LED2
  }
  else{
//    ext_led_off(0x02);                                               //�ر�LED2        
  }
  if(cmd & 0x10){                                                // LED1�ƿ���λ��bit4
//    ext_led_on(0x01);                                                //����LED1
  }
  else{
//    ext_led_off(0x01);                                               //�ر�LED1
  }
  
//  relay_control(cmd>>6);
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
