#include <rtthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "rf_thread.h"
#include "sensor/sensor.h"
#include "zxbee.h"

#define WBUF_LEN    200
static char wbuf[WBUF_LEN];

static void zxbeeBegin(void) {
  wbuf[0] = '{';
  wbuf[1] = '\0';
}

static char* zxbeeEnd(void) {
  int offset = strlen(wbuf);
  wbuf[offset-1] = '}';
  wbuf[offset] = '\0';
  if (offset > 2) return wbuf;
  return NULL;
}

static int zxbeeAdd(char* tag, char* val) {
  int offset = strlen(wbuf);
  if(offset > WBUF_LEN-1) return -1;
  sprintf(&wbuf[offset],"%s=%s,", tag, val);
  return 0;
}

/* 获取节点类型；成功返回0，失败返回-1 */
static int8_t _node_type(char *pval) {
  uint8_t radio_type, device_type;
  
  switch(rf_if_dev) {
    case RF_IF_WIFI:
      radio_type = WIFI_CONFIG_RADIO_TYPE;
      device_type = WIFI_CONFIG_DEVICE_TYPE;
    break;
    default: return -1;
  }
  
  sprintf(pval, "%u%u%u", radio_type, device_type, SENSOR_TYPE);
  return 0;
}

static void sensor_process_tag(char *ptag, char *pval) {
  char buf[50];
  if(strcmp(ptag, "TYPE") == 0) {
    if (pval[0] == '?') {   //{TYPE=?}
      if(_node_type(buf) == 0) {
        zxbeeAdd(ptag, buf);
      }
    }
  }
  else 
  {
    if(z_process_command_call(ptag, pval, buf) > 0) {
      if(strlen(ptag) == 3) ptag += 1;
      zxbeeAdd(ptag, buf);
    }
  }
}

static void zxbee_decode(char *pkg, int len) {
  char *pnow = NULL;
  char *ptag = NULL, *pval = NULL;
  
  if (pkg[0] != '{' || pkg[len-1] != '}') return ;
  pkg[len-1] = 0;
  pnow = pkg+1; 
  do {
    ptag = pnow;
    pnow = (char *)strchr(pnow, '=');
    if (pnow != NULL) {
      *pnow++ = 0;
      pval = pnow;
      pnow = strchr(pnow, ',');
      if (pnow != NULL) *pnow++ = 0;
      sensor_process_tag(ptag, pval);
    }
  } while (pnow != NULL);
}

void zxbee_onrecv_fun(char *pkg, int len) {
  char *psnd;
  
  if(rt_mutex_take(&rf_zxbee_mutex, 1000) != RT_EOK) return;
  zxbeeBegin();
  zxbee_decode(pkg, len);
  psnd = zxbeeEnd();
  if(psnd != NULL){
    rf_send((uint8_t *)psnd, strlen(psnd), 200);
    rt_kprintf("%s\n", psnd);
  }
  rt_mutex_release(&rf_zxbee_mutex);
}
