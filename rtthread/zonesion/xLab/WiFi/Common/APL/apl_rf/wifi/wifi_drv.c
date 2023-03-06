#include <rtthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "rf_if_app.h"
#include "rf_if_hw.h"
#include "wifi_drv.h"

uint8_t wifi_param_error = 0;
int wifi_rssi = 0;

//ESP8266模组复位
void wifi_reset(void) {
  rf_if_uart_send((uint8_t*)"AT+RST\r\n", strlen("AT+RST\r\n"));
  rf_if_uart_flush(3000);
}

//获取WIFI的IMEI
void wifi_Get_IMEI(char *pimei) {
  uint16_t len;
  char recv[101];
  char *phead = NULL;
  
  rf_if_uart_send((uint8_t*)"AT+CIPSTAMAC?\r\n", strlen("AT+CIPSTAMAC?\r\n"));
  len = rf_if_uart_read((uint8_t*)recv, 100, 50);
  if(len > 15) {
    recv[len] = 0;
    phead = strstr((const char*)recv, "+CIPSTAMAC:");
    if(phead != NULL) {
      phead += 12;
      char *pend = strchr((const char*)phead, '"');
      if(pend != NULL) {
        len = pend - phead;
        for(uint16_t i = 0; i < len; i++) {
          if(phead[i] >= 'a' && phead[i] <= 'z') {
            pimei[i] = phead[i] - 32; //转为大写
          } else {
            pimei[i] = phead[i];
          }
        }
        pimei[len] = 0;
      }
    }
  }
  rf_if_uart_flush(50);
}

//WIFI等待串口就绪：成功返回1，失败返回0
static int8_t wifi_Wait4Uart(void) {
  uint8_t recv[51]; //串口接收数据
  uint16_t len;//读取到的串口数据长度
  rt_err_t err;
  uint32_t mb_recv;
  uint8_t wait_time = 30;//串口等待时间
  
  rf_quit_trans();  //退出透传模式（适用于EC20/WIFI，单片机复位而模组仍在透传模式下）
  while(wait_time>0) {
    rf_if_uart_flush(0);
    rf_if_uart_send((uint8_t*)"AT\r\n", strlen("AT\r\n"));
    err = rt_mb_recv(&rf_if_data_mb, (rt_ubase_t*)&mb_recv, 100);
    if(err == RT_EOK) {
      if(mb_recv == 1) {            //邮件来源：通讯串口中断，处理该数据
        len = rf_if_uart_read(recv, 50, 50);
        if(len>0) {
          recv[len] = 0;
          if(strstr((const char*)recv, "OK") != NULL) break;
        }
      }
    }
    rt_thread_mdelay(3000);
    wait_time -= 3;
  }
  
  if(wait_time == 0) {//未检测到设备
    return 0;
  }
  rt_thread_mdelay(1000);
  rf_if_uart_send((uint8_t*)"ATE0\r\n", strlen("ATE0\r\n"));
  len = rf_if_uart_read(recv, 50, 100);
  return 1;
}

//WIFI连接到路由器：成功返回1，失败返回0
static int8_t wifi_connect_AP(char *ssid, char *key) {
  uint8_t cmd[120];     //串口发送命令
  char recv[101];    //串口接收数据
  uint16_t len;         //读取到的串口数据长度
  uint8_t try_times = 0;
  rt_err_t err = RT_EOK;
  uint32_t mb_recv;
  
  if(strlen(ssid) == 0) return 0;
  
  sprintf((char *)cmd, "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, key);  //AT+CWJAP="iCity","12345678"
  while(try_times++ < 3) {
    rf_if_uart_send(cmd, strlen((char *)cmd));
    err = rt_mb_recv(&rf_if_data_mb, (rt_ubase_t*)&mb_recv, 20000);
    while(err == RT_EOK) {
      len = rf_if_uart_read((uint8_t *)recv, 100, 1000);
      if(len>0) {
        recv[len] = 0;
        if(strstr((const char*)recv, "OK") != NULL) {
          rf_if_uart_flush(0);
          return 1;
        } else if(strstr((const char*)recv, "ERROR") != NULL) {
          rf_if_uart_flush(0);
          wifi_param_error = 1;
          return 0;
        }
      }
      err = rt_mb_recv(&rf_if_data_mb, (rt_ubase_t*)&mb_recv, 20000);
    }
  }
  return 0;
}

//WIFI获取RSSI
static void wifi_get_RSSI(char *ssid) {
  uint16_t len;
  uint8_t cmd[120];     //串口发送命令
  char recv[101];
  char *phead = NULL;
  rt_err_t err;
  uint32_t mb_recv;
  
  if(strlen(ssid) == 0) return;
  sprintf((char *)cmd, "AT+CWLAP=\"%s\"\r\n", ssid);  //AT+CWLAP="iCity"
  while(1) {
    rf_if_uart_send(cmd, strlen((char *)cmd));
    err = rt_mb_recv(&rf_if_data_mb, (rt_ubase_t*)&mb_recv, 10000);
    if(err == RT_EOK) {
      if(mb_recv == 1) {            //邮件来源：通讯串口中断，处理该数据
        len = rf_if_uart_read((uint8_t *)recv, 100, 50);
        if(len > 7) {
          recv[len] = 0;
          //+CWLAP:(4,"iCity",-52,"50:fa:84:48:dd:c8",6,-1,-1,5,3,7,0)
          phead = strstr((const char*)recv, "+CWLAP:");
          if(phead == NULL) return;
          phead = strchr(phead, ',');
          if(phead == NULL) return;
          phead = strchr(phead+1, ',');
          if(phead == NULL) return;
          wifi_rssi = atoi(phead+1);
          rf_if_uart_flush(0);
          break;
        }
      }
    }
    rt_thread_mdelay(100);
  }
}

//WIFI从服务器断开
void wifi_disconnect_TCP(void) {
  rf_quit_trans();
  rf_if_uart_send((uint8_t*)"AT+CIPCLOSE\r\n", strlen("AT+CIPCLOSE\r\n"));
  rf_if_uart_flush(1000);
}

//WIFI连接到指定服务器端口：成功返回1，失败返回0
static int8_t wifi_connect_TCP(char *ip, uint16_t port) {
  uint8_t cmd[120];     //串口发送命令
  char recv[101];    //串口接收数据
  uint16_t len;         //读取到的串口数据长度
  uint8_t try_times = 0;
  rt_err_t err;
  uint32_t mb_recv;
  
  if(strlen(ip) == 0) return 0;
  
  rf_if_uart_flush(1000);
  sprintf((char *)cmd, "AT+CIPSTART=\"TCP\",\"%s\",%u\r\n", ip, port);  //AT+MIPODM=1,,"api.zhiyun360.com",28082,0
  while(try_times++ < 3) {
    //连接到指定端口
    rf_if_uart_send(cmd, strlen((char *)cmd));
    err = rt_mb_recv(&rf_if_data_mb, (rt_ubase_t*)&mb_recv, 10000);
    if(err == RT_EOK) {
      len = rf_if_uart_read((uint8_t *)recv, 100, 1000);
      if(len>0) {
        recv[len] = 0;
        if(strstr((const char*)recv, "CONNECT") != NULL) {
          rf_if_uart_send((uint8_t*)"AT+CIPMODE=1\r\n", strlen("AT+CIPMODE=1\r\n"));  //透传接收模式
          rf_if_uart_read((uint8_t*)recv, 50, 1000);
          rf_if_uart_send((uint8_t*)"AT+CIPSEND\r\n", strlen("AT+CIPSEND\r\n"));      //透传发送模式
          rf_if_uart_flush(1000);
          return 1;
        } else if(strstr((const char*)recv, "ERROR") != NULL) {
          rf_if_uart_flush(100);
          wifi_param_error = 1;
          return 0;
        }
      }
      //连接失败
      wifi_disconnect_TCP();
    }
    rt_thread_mdelay(1000);
  }
  return 0;
}

//WIFI连接到服务器：成功返回1，失败返回0
int8_t wifi_TCP_Connect(char *ssid, char *key, char *ip, uint16_t port) {
  if(strlen(ssid) == 0 || strlen(ip) == 0) {
    wifi_param_error = 1;
    return 0;
  }
  
  //等待串口就绪
  if(wifi_Wait4Uart() != 1) return 0;
  //连接到路由器
  if(wifi_connect_AP(ssid, key) != 1) return 0;
  //预防性断开
  wifi_disconnect_TCP();
  //更新RSSI
  wifi_get_RSSI(ssid);
  //连接到服务器指定端口
  if(wifi_connect_TCP(ip, port) == 0) {
    //连接失败：复位WIFI
    wifi_disconnect_TCP();
    wifi_reset();
    rf_if_uart_flush(5000);
    return 0;
  }
  return 1;
}
