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

//�洢A0-A7����ת�ɵ��ַ�����ÿ���ַ���������11����Ч�ַ��������ڵĴ�����Ӧ��ʼ��Ϊ""
static char sensor_data_A[8][16] = {"", "", "", "0", "0", "0", "", ""}; 
t_sensor_value sensor_value;      //������ԭʼ���ݽṹ��
static int work_mod = 2; //1 �豸��ֵģʽ, 2 ����ģʽ,��λ�����Ը���Ƭ��ֵ
static char A0[16];                                              // A0�洢����
static int V1 = 0, V2 = 0, V3 = 0, V4 = 0; 
static uint16_t seed = 0;

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
  //��������ӳ�ʼ��
  extern t_rf_info rf_info;
  for(uint8_t i = 0; i < strlen(rf_info.mac); i++) {
    seed += rf_info.mac[i];
  }
  srand(seed);
  //��������ʼ��
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
  sensorControl(sensor_value.D1);   //�����ָ���ǰ����״̬
  rt_timer_start(&sensor_poll_timer);       //������ʱ����ÿ��V0�����ϱ�һ������
  rt_thread_startup(&sensor_check_thread);  //���ڰ����ഫ����ʱ�����������ഫ�����߳�
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
    if (sensor_value.A3 > 0) {
      sensor_value.A4 = 1 + rand()%500;                         // �豸�������ѿۿ��ͨ�����������
      if (sensor_value.A4 > sensor_value.A3) sensor_value.A4 = sensor_value.A3;
      sensor_value.A3 = (sensor_value.A3 - sensor_value.A4);    // �豸���
      sensor_value.A5 = (sensor_value.A5 + sensor_value.A4);    // �豸�ۼ����ѽ��
      
      sprintf(sensor_data_A[3], "%d.%d", sensor_value.A3/100, sensor_value.A3%100);   // �������
      sprintf(sensor_data_A[4], "%d.%d", sensor_value.A4/100, sensor_value.A4%100);   // ���������ѿۿ���
      sprintf(sensor_data_A[5], "%d.%d", sensor_value.A5/100, sensor_value.A5%100);   // �����ۻ����ѽ��
      OLED_ShowString(21+13*3,2,(unsigned char *)sensor_data_A[3],12);  // oLED��ʾ�������
      if(sensor_value.A3 == 0) {    //����۷ѣ��̵����Ͽ���ģ�������豸�Ͽ���
        sensor_value.D1 &= ~2;
        relay_control(sensor_value.D1);
      }
    }
    
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
      //ģ������
      if (sensor_value.A3 > 0) {
        sensor_value.A4 = rand()%500;                                          // �豸�������ѿۿ��ͨ�����������
        if (sensor_value.A4 > sensor_value.A3) sensor_value.A4 = sensor_value.A3;
        sensor_value.A3 = sensor_value.A3 - sensor_value.A4;
        sensor_value.A5 = sensor_value.A5 + sensor_value.A4;
        sprintf(temp, "A3=%d.%d,", sensor_value.A3/100, sensor_value.A3%100);   // �������
        strcat(data_send, temp);
        sprintf(temp, "A4=%d.%d,", sensor_value.A4/100, sensor_value.A4%100);   // ���������ѿۿ���
        strcat(data_send, temp);
        sprintf(temp, "A5=%d.%d,", sensor_value.A5/100, sensor_value.A5%100);   // �����ۻ����ѽ��
        strcat(data_send, temp);
        sprintf(buff, "%d.%d   ", sensor_value.A3/100, sensor_value.A3%100);
        OLED_ShowString(21+13*3,2,(unsigned char *)buff,12);
        if(sensor_value.A3 == 0) {   //����۷ѣ��̵����Ͽ���ģ�������豸�Ͽ���
          sensor_value.D1 &= ~2;
          relay_control(sensor_value.D1);
        }
      }
    }
  }
  last_key = k;
  //  �����������
  
  // ��������
  static char status = 1;
  static char last_cid[7];
  static char card_cnt = 0;                                     // ��Ƭ������
  static char cid[7];
  static char write = 0;
  
  if (status == 1) {                                            // ���125k�������
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
      } else { //ͬһ�ſ�Ƭ
        card_cnt = 5;
      }
      RFID7941CheckCard125kHzReq();
    } else {
      if (card_cnt > 0) {
        card_cnt -= 1;
      }
      if (card_cnt == 0) {
        memset(last_cid, 0, sizeof last_cid);
        RFID7941CheckCard1356MHzReq();                          // ���13.56M��Ƭ
        status = 2;
      } else {
        RFID7941CheckCard125kHzReq();
      }
    }
  } else if (status == 2) {                                     // 13.56��Ƭ��⴦��
    buzzer_off();
    if (RFID7941CheckCard1356MHzRsp(cid) > 0) {
      if (memcmp(last_cid, cid, 4) != 0) {
        RFID7941ReadCard1356MHzReq(8, KEY);
        status = 3;
      } else {
        card_cnt = 5;                                           // ˢ�¿�Ƭ����
        if (work_mod == 1) {                                    // ��ֵ�豸
          if (write == 3 && sensor_value.A2 > 0) {
            memset(buff, 0, 16);
            RFID7941WriteCard1356MHzReq(8, KEY, buff);
            status = 4;
          } else {
            RFID7941CheckCard1356MHzReq();
          }
        } else
        if (work_mod == 2) {                                    // ����ģʽ
          int money;
          if (V1 != 0) {                                        // ��ֵ
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
        sensor_value.A2 = 0;                                                 // �������
      } else {
        RFID7941CheckCard1356MHzReq();
      }
    }
  } else if (status == 3) {                                     // ����Ƭ��ȡ���
    if (RFID7941ReadCard1356MHzRsp(buff) > 0) {                 // ��ȡ�������浱ǰ��Ƭid
      memcpy(last_cid, cid, 4);
      card_cnt = 5;      
      sprintf(A0, "%02X%02X%02X%02X", cid[0],cid[1],cid[2],cid[3]);  
      int money = ((uint32_t)buff[12]<<24) | ((uint32_t)buff[13]<<16) | (buff[14]<<8) | (buff[15]);
      sensor_value.A2 = money;
      if (work_mod == 1) {                                      // �豸��ֵģʽ
        if (money > 0) {
          buff[12] = buff[13] = buff[14] = buff[15] = 0;
          write = 3;                                            // �ٴμ�⿨Ƭ��ͨ��write = 3 ��д��
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
      RFID7941CheckCard1356MHzReq();                            // ���¼�⿨Ƭ
      status = 2;
    }
  } else if (status == 4) {                                     // �����ֵ�豸д�����
      if (RFID7941WriteCard1356MHzRsp() > 0) {
        //��ֵ�豸�ɹ�
        sensor_value.A3 += sensor_value.A2;
        sensor_value.A2 = 0;
        sprintf(buff, "%d.%d   ", sensor_value.A3/100,sensor_value.A3%100);
        OLED_ShowString(21+13*3,2,(unsigned char *)buff,12);
        
        write = 0;
        sensor_value.D1 |= 2;
        relay_control(sensor_value.D1);
        
        buzzer_on();
      } else { //д��ʧ��
        //memset(last_cid, 0, sizeof last_cid);
        //card_cnt = 0;
      }
      RFID7941CheckCard1356MHzReq();
      status = 2;
  } else if (status == 5) {                                     // ��Ƭ��ֵ�۷ѽ�����
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
  
  //���쳣ʱ��ÿ3�뷢��һ���쳣����
  if(strlen(data_send) > 2) {
    data_send[strlen(data_send)-1] = '}';
    rf_send((uint8_t *)data_send, strlen(data_send), 200);
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
  relay_control(cmd);
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
  sensor_value.A3 = 10000;
  sensor_value.D0 = 0xFF;
  sensor_value.D1 = 2;
  sensor_value.V0 = 30;
  strcpy(sensor_value.V1, "0");
  strcpy(sensor_value.V2, "0");
}

/* ������У׼ */
void sensor_cali(uint8_t ch, int value) {
}

static void sensor_poll_timer_cb(void *parameter) {
  //��ʱ���ﵽV0ʱ�����󣬷����źţ���sensorUpdate������������
  rt_sem_release(&sensor_poll_sem);
}
