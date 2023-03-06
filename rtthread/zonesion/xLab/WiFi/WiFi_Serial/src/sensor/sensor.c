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
#include "rf_thread.h"
#include "sensor_thread.h"

static struct rt_timer sensor_poll_timer;
static struct rt_semaphore sensor_poll_sem;
static void sensor_poll_timer_cb(void *parameter);

//�洢A0-A7����ת�ɵ��ַ�����ÿ���ַ���������11����Ч�ַ��������ڵĴ�����Ӧ��ʼ��Ϊ""
static char sensor_data_A[8][12] = {"0", "0", "0", "", "", "0", "0", "0"}; 
t_sensor_value sensor_value;      //������ԭʼ���ݽṹ��
extern uint8_t debug_flag;

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
  sensorControl(sensor_value.D1);   //�����ָ���ǰ����״̬
  rt_timer_start(&sensor_poll_timer);       //������ʱ����ÿ��V0�����ϱ�һ������
  rt_thread_startup(&sensor_check_thread);  //���ڰ����ഫ����ʱ�����������ഫ�����߳�
}

static uint8_t gas_init_flag, gas_consume;
static void updateA0(void) {
  //��ʷ��������m3��
  static float gas_last;
  eMBMasterReqErrCode errcode;
  errcode = Modbus_Master_Send(8, 2, 0, NULL, 2);//03ָ��,�豸��ַ,�Ĵ�����ʼ��ַ,�Ĵ�������
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
      saveData_Update(); //A0�仯ʱ���洢��SPIFlash
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
  //��ѹ��kPa��-GAS_MAX~GAS_MAX��
  uint16_t ad_cur_data = ad_Get_Sensor_Value(AD_CH_CUR);
  if(ad_cur_data < 820) sensor_value.A1 = 0;
  else {
    float pressure = (ad_cur_data / 4096.f * 3.f-0.6f) / (3.f-0.6f) * (GAS_MAX-(-GAS_MAX)) + (-GAS_MAX);//��ѹֵ
    pressure *= sensor_value.cali[1];
    if(pressure < 0.f) pressure = 0.f; //ֻ��������ѹ
    else if(pressure > GAS_MAX) pressure = GAS_MAX; //���ݳ������ƣ���Ϊ���ֵ
    sensor_value.A1 = pressure;
  }
  sprintf(sensor_data_A[1], "%.1f", sensor_value.A1);
}

static void updateA2(void) {
  //ȼ��Ũ�ȣ�ppm��0-20000��
  float vol = ad_Get_Sensor_Value(AD_CH_GAS) / 4096.f * 3.f * 2.f;//��ѹֵ��0-5V
  float gas;
  //�ֶκ�������ȼ��Ũ��
  if(vol < 0.625f) {            //(0,0)��(0.625,518)
    gas = vol/0.625f*518.f;
  } else if(vol < 2.16f) {      //(0.625,518)��(2.16,1490)
    gas = (vol-0.625f)/(2.16f-0.625f)*(1490.f-518.f)+518.f;
  } else if (vol < 2.625f) {    //(2.16,1490)��(2.625,2518)
    gas = (vol-2.16f)/(2.625f-2.16f)*(2518.f-1490.f)+1490.f;
  } else if (vol < 2.91f) {     //(2.625,2518)��(2.91,3528)
    gas = (vol-2.625f)/(2.91f-2.625f)*(3528.f-2518.f)+2518.f;
  } else if (vol < 3.2f) {      //(2.91,3528)��(3.2,5518)
    gas = (vol-2.91f)/(3.2f-2.91f)*(5518.f-3528.f)+3528.f;
  } else if (vol < 3.59f) {     //(3.2,5518)��(3.59,11037)
    gas = (vol-3.2f)/(3.59f-3.2f)*(11037.f-5518.f)+5518.f;
  } else if (vol < 3.764f) {    //(3.59,11037)��(3.764,15056)
    gas = (vol-3.59f)/(3.764f-3.59f)*(15056.f-11037.f)+11037.f;
  } else if (vol < 4.f) {       //(3.764,15056)��(4,20000)
    gas = (vol-3.764f)/(4.f-3.764f)*(20000.f-15056.f)+15056.f;
  } else {                      //(4,20000)��(~,20000)
    gas = 20000.f;
  }
  if(sensor_value.A7 == 1) {    //ͨ�������ֶμ�⵽�ܵ�й©ʱ��ģ���ʱȼ���仯
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
  if(sensor_value.A5 > 180) return; //���ݳ������ƣ�������
  sprintf(sensor_data_A[5], "%d", sensor_value.A5);
}

static void updateA6(void) {
  //������0/1�����ܵ����š��뻧���Ŷ��򿪣�����������
  if((sensor_value.D1&0x03) == 0x03 && gas_consume == 0) {
    sensor_value.A6 = 1;
  } else {
    sensor_value.A6 = 0;
  }
  sprintf(sensor_data_A[6], "%u", sensor_value.A6);
}

static void updateA7(void) {
  //©����0/1�����뻧���Źر�ʱ���������б仯
  if((sensor_value.D1&0x02) == 0x00 && gas_consume > 0) {
    sensor_value.A7 = 1;
  } else {
    sensor_value.A7 = 0;
  }
  sprintf(sensor_data_A[7], "%u", sensor_value.A7);
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
    saveData_Update(); //D0�����ı�ʱ���洢
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
    saveData_Update(); //D1�����ı�ʱ���洢
    sensorControl(sensor_value.D1);//ִ��D1��ؿ��Ʋ���
  }
}

static void updateV0(char *pval) {
  uint32_t value = atoi(pval);
  if(value == 0 || value > 65535) return; //���ݳ������ƣ�������
  if(sensor_value.V0 != value) {
    sensor_value.V0 = value;
    saveData_Update();
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
  if(strcmp(sensor_value.V3, pval) != 0) {
    strncpy(sensor_value.V3, pval, sizeof(sensor_value.V3));
    sensor_value.V3[sizeof(sensor_value.V3)-1] = 0;
    saveData_Update();
  }
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
      rt_kprintf("%s\n", data_send);
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
  static uint16_t A6_count = 0, A7_count = 0;
  static uint32_t A6_last;
  static int A7_last;
  
  char data_send[100] = "{";
  char temp[20];
  uint8_t send_flag = 0;
  
  if((sensor_value.D0 & (1<<6)) != 0) {     //����״̬
    if(sensor_value.A6 == 1 && ++A6_count >= 100) {
      A6_last = sensor_value.A6;
      sprintf(temp, "A6=%s,", sensor_data_A[6]);
      strcat(data_send, temp);
      A6_count = 0;
      send_flag = 1;
    } else if(sensor_value.A6 == 0) {
      A6_count = 0; //A6��Ϊ0ʱ����ռ���
      if(A6_last == 1) {
        A6_last = sensor_value.A6;
        sprintf(temp, "A6=%s,", sensor_data_A[6]);
        strcat(data_send, temp);
        send_flag = 1;
      }
    }
  }
  if((sensor_value.D0 & (1<<7)) != 0) {     //©��״̬
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
  
  //���쳣ʱ��ÿ3�뷢��һ���쳣����
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
* ���ƣ�sensorControl()
* ���ܣ�����������
* ������cmd - ��������
* ���أ���
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
void sensorControl(uint8_t cmd) {
  if((cmd&0x01) != 0) { //���Ƽ̵���1
    relay_on(1);
  } else {
    relay_off(1);
  }
  if((cmd&0x02) != 0) { //���Ƽ̵���2
    relay_on(2);
  } else {
    relay_off(2);
  }
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
        case 0: updateD0(1, value); break;
        case 1: updateD1(1, value); break;
        default: return -1;
      }
      return 0;
    }
  } else if(strncmp(ptag, "CD", 2) == 0) {  //{CDx=y}
    if (ptag[2] >= '0' && ptag[2] <= '1') {
      uint8_t tag = atoi(&ptag[2]);
      uint8_t value = atoi(pval);//*pvalΪ����ʱ��valueΪҪ���õ�ֵ��*pval��Ϊ����ʱ��atoi����0��Ҳ����ı䵱ǰֵ
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

/* K3����������Ĵ��� */
void sensor_Keypressed(void) {
  rt_sem_release(&sensor_poll_sem);
}

/* ���в�����ʼ��--����δ��ȡ���洢������ֵ����� */
void sensor_para_init(void) {
  if(*(uint32_t *)&sensor_value.A0 == 0xFFFFFFFF) sensor_value.A0 = 0.f;//��ʷ������
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

/* ������У׼ */
void sensor_cali(uint8_t ch, int value) {
  uint16_t ad_cur_data;
  float pressure;
  int16_t xyz[3];
  
  switch(ch) {
    case 1: //��ѹ--��¼(value/10)�����ֵ�ı���
      ad_cur_data = ad_Get_Sensor_Value(AD_CH_CUR);
      pressure = (ad_cur_data / 4096.f * 3.f-0.6f) / (3.f-0.6f) * (GAS_MAX-(-GAS_MAX)) + (-GAS_MAX);//��ѹֵ
      if(value > 0 && pressure > 0.001f) {   //��֤value��������;���������0���Ǹ���
        sensor_value.cali[1] = (float)value / 10.f / pressure;
        saveData_Update();
      }
    case 5: //�㵹�Ƕ�--����value������ǰ�Ƕ���Ϊ0��
      ADXL345_GetValue(xyz);
      sensor_value.cali[5] = fabs(acos(xyz[FIX_ANGLE]/256.0)*180.0/PI);
      saveData_Update();
    break;
    default:
    break;
  }
}

static void sensor_poll_timer_cb(void *parameter) {
  //��ʱ���ﵽV0ʱ�����󣬷����źţ���sensorUpdate������������
  rt_sem_release(&sensor_poll_sem);
}
