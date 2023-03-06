/* ͨѶģ��1�Žӿ�Ӧ�� */
#include <rtthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "stm32f10x.h"
#include "bsp_flash/flash.h"
#include "rf_if_hw.h"
#include "rf_if_app.h"
#include "zxbee/zxbee.h"
#include "wifi/wifi_drv.h"
#include "sensor/sensor.h"
#include "rf_thread.h"
#include "apl_at/at_app.h"
#include "apl_at/at_hw.h"

t_rf_info rf_info;      //ͨѶģ����Ϣ

e_rf_if_dev rf_if_dev = RF_IF_NULL;
extern uint8_t rf_Link_On;          /* ����ͨѶģ�鵽���ص�ͨѶ״̬��0--δ���ӣ�1--������ */
static uint16_t rf_if_heart_try;    //���������Դ���

static int16_t rf_if_read(uint8_t *pdata, uint16_t timeout_ms);
static e_rf_if_dev rf_if_Detect(void);

/* Ӧ������ */
void rf_if_run(void) {
  extern char finsh_zxbee_recv_data[];
  rt_err_t err;
  char recv[101];
  int16_t length = 0;
  uint32_t mb_recv;
  
  if(rf_Link_On == 1) {
    err = rt_mb_recv(&rf_if_data_mb, (rt_ubase_t*)&mb_recv, RT_WAITING_FOREVER);
    if(err == RT_EOK) {
      if(mb_recv == 1) {  //�ʼ���Դ��ͨѶ�����ж�
        length = rf_if_read((uint8_t *)recv, 100);
        if(length <= 0) return;
        if(strstr(recv, "CLOSED") != NULL) {  //���յ�TCP���ӶϿ�����Ϣ
          rf_Link_On = 0;
          rf_if_NetStatus(RESET);
          at_uart_send("+LINK:0\r\nOK\r\n");
          rt_kprintf("RF Link: OFF\n");
        } else {
          at_recv(recv);
#ifndef WIFI_SERIAL      /*�ն˽ڵ�*/
          if(recv[0]=='{' && recv[length-1]=='}') { //control������
            recv[length] = 0;
            zxbee_onrecv_fun(recv, length);
          }
#endif
        }
      }
    }
  } else {
    rt_thread_mdelay(1000);
  }
}

void rf_info_init(void) {
  //��ȡͨѶģ����Ϣ
  flash_read(FLASH_PARAM_START_ADDR, (uint16_t *)(&rf_info), sizeof(t_rf_info) / 2);
  if(strncmp(rf_info.mac, "WIFI:", 5) != 0) {
    strcpy(rf_info.mac, "WIFI:");
  }
  if(rf_info.zhiyun_id[0] == 0xFF || strlen(rf_info.zhiyun_id) >= sizeof(rf_info.zhiyun_id)) {
    rf_info.zhiyun_id[0] = 0;
  }
  if(rf_info.zhiyun_key[0] == 0xFF || strlen(rf_info.zhiyun_key) >= sizeof(rf_info.zhiyun_key)) {
    rf_info.zhiyun_key[0] = 0;
  }
  if(rf_info.ip[0] == 0xFF || strlen(rf_info.ip) >= sizeof(rf_info.ip)) {
    strcpy(rf_info.ip, ZHIYUN_DEFAULT_IP);
    rf_info.port = ZHIYUN_DEFAULT_PORT;
  }
  if(rf_info.wifi_ssid[0] == 0xFF || strlen(rf_info.wifi_ssid) >= sizeof(rf_info.wifi_ssid)) {
    rf_info.wifi_ssid[0] = 0;
  }
  if(rf_info.wifi_key[0] == 0xFF || strlen(rf_info.wifi_key) >= sizeof(rf_info.wifi_key)) {
    rf_info.wifi_key[0] = 0;
  }
}

/* Ӧ�ó�ʼ�� */
void rf_if_init(void) {
  char mac[40] = "WIFI:";
  rf_if_HW_Init();
  rf_if_dev = rf_if_Detect();
  switch(rf_if_dev) {  //��ȡͨѶģ����Ϣ
    case RF_IF_WIFI:
      wifi_Get_IMEI(mac+5);
      if(strcmp(rf_info.mac, mac) != 0) {
        strcpy(rf_info.mac, mac);
        flash_write(FLASH_PARAM_START_ADDR, (uint16_t *)(&rf_info), sizeof(t_rf_info) / 2);
      }
    break;
  }
}

/* ͨѶ�ӿڷ������ݵ�����/������������ֵ��-1--����ʧ�ܣ�>=0�����ͳ����ֽ��� */
int16_t rf_if_send(uint8_t *pdata, uint16_t length, uint16_t timeout_ms) {
  if(length==0 || rf_if_dev==RF_IF_NULL) return 0;
  
  char send[300];
  
  //�������ݴ��
  if(strstr((char *)pdata, "\"method\":\"echo\"") != NULL || strstr((char *)pdata, "\"method\":\"authenticate\"") != NULL) {   //����������ָ֤������Ѵ��
    strncpy(send, (char *)pdata, length);
  } else {  //����ָ֤���Ҫ�������
    length = sprintf(send, "{\"method\":\"sensor\", \"data\":\"%s\"}", pdata);
  }
  send[length] = 0;
  
  //�������ݵ�������
  if(rf_if_dev == RF_IF_WIFI) {        //WIFI͸�����ͣ�����״̬Ϊ������
    rf_if_uart_send((uint8_t*)send, length);
    return length;
  }
  return -1;    //�豸�����쳣
}

/* ͨѶ�ӿ�1��ȡ����/���������������ݣ�����ֵ��-1--δ�ܻ�ȡ���������������ݣ�>=0����ȡ�����ֽ��� */
static int16_t rf_if_read(uint8_t *pdata, uint16_t timeout_ms) {
  char recv[201];
  char *phead = NULL, *ptail = NULL;
  uint16_t recv_len;
  
  //��ȡ��������
  if(rf_if_dev == RF_IF_WIFI) {
    recv_len = rf_if_uart_read((uint8_t*)recv, 200, timeout_ms);
  } else {
    return -1;
  }
  
  //�������ݽ���
  recv[recv_len] = 0;
  recv_len = strlen(recv);
  if(recv_len == 0 || recv[recv_len-1] != '}') return -1;
  phead = strstr(recv, "method");
  if(phead == NULL || *(phead+8) != '"') return -1;
  phead = phead+9;
  if(strncmp(phead, "authenticate_rsp", strlen("authenticate_rsp")) == 0) { //��ָ֤��ظ�
    phead = strstr(phead, "status");
    if(phead == NULL || *(phead+8) != '"') return -1;
    ptail = strchr(phead+9, '"');
    recv_len = ptail - phead - 9;
    strncpy((char *)pdata, phead+9, recv_len);
    pdata[recv_len] = 0;
  } else if(strncmp(phead, "control", strlen("control")) == 0) {            //�������·�ָ��
    phead = strstr(phead, "data");
    if(phead == NULL || *(phead+6) != '"') return -1;
    ptail = strchr(phead+7, '"');
    recv_len = ptail - phead - 7;
    strncpy((char *)pdata, phead+7, recv_len);
    pdata[recv_len] = 0;
  } else if(strncmp(phead, "echo", strlen("echo")) == 0) {                  //�������ظ�
    rf_if_heart_try = 0;
    recv_len = 0;
  }
  return recv_len;
}

/* RF�ӿ��豸��⡣����ֵ����⵽���豸���� */
static e_rf_if_dev rf_if_Detect(void) {
  uint8_t data[201]; //���ڽ�������
  uint16_t len;//��ȡ���Ĵ������ݳ���
  uint8_t wait_time = 30;//�豸���ȴ�ʱ��
  
  rf_quit_trans();  //�˳�͸��ģʽ��������EC20/WIFI����Ƭ����λ��ģ������͸��ģʽ�£�
  while(wait_time>0) {
    rf_if_uart_flush(0);
    rf_if_uart_send((uint8_t*)"AT\r\n", strlen("AT\r\n"));
    len = rf_if_uart_read(data, 50, 100);
    if(len>0) {
      data[len] = 0;
      if(strstr((const char*)data, "OK") != NULL) break;
    }
    rt_thread_mdelay(3000);
    wait_time -= 3;
  }
  if(wait_time == 0) {//δ��⵽�豸
    return RF_IF_NULL;
  }
  rt_thread_mdelay(1000);
  rf_if_uart_send((uint8_t*)"ATE0\r\n", strlen("ATE0\r\n"));
  len = rf_if_uart_read(data, 50, 100);
  rf_if_uart_send((uint8_t*)"AT+CWMODE=1\r\n", strlen("AT+CWMODE=1\r\n"));
  len = rf_if_uart_read(data, 50, 100);
  rf_if_uart_send((uint8_t*)"AT+GMR\r\n", strlen("AT+GMR\r\n"));
  len = rf_if_uart_read(data, 200, 100);
  rf_if_uart_flush(100);//�������sem
  if(len>0) {
    data[len] = 0;
    if(strstr((const char*)data, "ESP8266") != NULL) {
      return RF_IF_WIFI;
    }
  }
  return RF_IF_NULL;
}

/* RF�ȴ��豸����--���߳������� */
void rf_if_Link(void) {
  rt_err_t err;
  uint32_t mb_recv;
  char recv[201];
  char send[200];//���ڷ�������
  int16_t length = 0;
  int8_t server_flag = 0;  //���������ӱ�־���ɹ�Ϊ1��ʧ��Ϊ0
  uint8_t try_times = 0;
  uint32_t echo_seq = 0;
  
  rf_if_NetStatus(RESET);
  //ִ���豸���ӵ��������Ĳ���
  if(rf_if_dev == RF_IF_WIFI) {
    while(server_flag == 0) {
      if(wifi_param_error == 0) {
        server_flag = wifi_TCP_Connect(rf_info.wifi_ssid, rf_info.wifi_key, rf_info.ip, rf_info.port);
        if(++try_times > 3) {
          //����3���޷��ɹ����ӵ����Ʒ�������ģ�鸴λ
          wifi_reset();
          try_times = 0;
        }
      } else {
        try_times = 0;
        rt_thread_mdelay(3000);
      }
    }
  } else {
    rf_Link_On = 0;
    rt_thread_mdelay(3000);
    return;
  }
  
  //����id��key����Ԥ��
  if(rf_info.zhiyun_id[0] == 0xFF) rf_info.zhiyun_id[0] = 0;
  rf_info.zhiyun_id[sizeof(rf_info.zhiyun_id)-1] = 0;
  if(rf_info.zhiyun_key[0] == 0xFF) rf_info.zhiyun_key[0] = 0;
  rf_info.zhiyun_key[sizeof(rf_info.zhiyun_key)-1] = 0;
  
  //ִ�нڵ�ע�ᵽ���ƵĲ���
  try_times = 0;
  sprintf(send, "{\"method\":\"authenticate\",\"uid\":\"%s\",\"key\":\"%s\", \"addr\":\"%s\", \"version\":\"0.1.0\", \"autodb\":true}", rf_info.zhiyun_id, rf_info.zhiyun_key, rf_info.mac);
  while(try_times++ < 3) {
    rf_if_send((uint8_t *)send, strlen(send), 100);
    err = rt_mb_recv(&rf_if_data_mb, (rt_ubase_t*)&mb_recv, 5000);
    if(err == RT_EOK) {
      if(mb_recv == 1) {            //�ʼ���Դ��ͨѶ�����жϣ����������
        length = rf_if_read((uint8_t *)recv, 200);
        if(length > 0) {
          recv[length] = 0;
          if(strstr(recv, "ok") != NULL) {
            rf_Link_On = 1;
            rf_if_NetStatus(SET);
            at_uart_send("+LINK:1\r\nOK\r\n");
            rt_kprintf("RF Link: ON\n");
            #ifndef WIFI_SERIAL      /*�ն˽ڵ�*/
              sensorLinkOn();
            #endif
            break;
          } else if(strstr(recv, "error") != NULL) {
            rf_Link_On = 0;
            break;
          }
        }
      }
      rt_thread_mdelay(1000);
    }
  }
  
  //��ʱ����������������Ƿ���ֶ���
  while(rf_Link_On == 1) {
    if(rf_if_heart_try++ >= 3) {    //����������3���޻ظ��������ѶϿ�����������
      rf_if_heart_try = 0;
      rf_Link_On = 0;
      rf_if_NetStatus(RESET);
      at_uart_send("+LINK:0\r\nOK\r\n");
      rt_kprintf("RF Link: OFF\n");
      break;
    }
    sprintf(send, "{\"method\":\"echo\",\"timestamp\":%u,\"seq\":%u}", rt_tick_get(), echo_seq++);
    rf_send((uint8_t *)send, strlen(send), 100);
    rt_thread_mdelay(30000);        //30�뷢��1��������
  }
  
  //����ʱ�����ж��ߴ���Ȼ���˳��˺�������rf_link_thread_entry����ִ�б�����
  if(rf_Link_On == 0) {
    if(rf_if_dev == RF_IF_WIFI) {
      wifi_disconnect_TCP();
    }
  }
}

//����ģ���˳�͸��ģʽ��EC20/WIFI��Ҫ��
void rf_quit_trans(void) {
  rt_thread_mdelay(100);
  rf_if_uart_send((uint8_t*)"+++", strlen("+++"));
  rt_thread_mdelay(100);
  rf_if_uart_send((uint8_t*)"\r\n", strlen("\r\n"));
  rf_if_uart_flush(100);
}
