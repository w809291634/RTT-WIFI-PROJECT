#include <rtthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "finsh_ex.h"

#define DEBUG_EN
#define RF_EN
#define SENSOR_EN
#define IAP_EN
#define REBOOT_EN

#ifdef DEBUG_EN
uint8_t debug_flag = 0x07;
int debug(int argc, char **argv) {
  if(argc == 3) {
    if(!rt_strcmp(argv[1], "rf")) {
      if(!rt_strcmp(argv[2], "on")) {
        debug_flag |= 0x01;
        rt_kprintf("RF Debug info on.\n");
      } else if(!rt_strcmp(argv[2], "off")) {
        debug_flag &= ~0x01;
        rt_kprintf("RF Debug info off.\n");
      }
    } else if(!rt_strcmp(argv[1], "at")) {
      if(!rt_strcmp(argv[2], "on")) {
        debug_flag |= 0x02;
        rt_kprintf("AT Debug info on.\n");
      } else if(!rt_strcmp(argv[2], "off")) {
        debug_flag &= ~0x02;
        rt_kprintf("AT Debug info off.\n");
      }
    } else if(!rt_strcmp(argv[1], "sensor")) {
      if(!rt_strcmp(argv[2], "on")) {
        debug_flag |= 0x04;
        rt_kprintf("Sensor Debug info on.\n");
      } else if(!rt_strcmp(argv[2], "off")) {
        debug_flag &= ~0x04;
        rt_kprintf("Sensor Debug info off.\n");
      }
    }
  }
  return 0;
}
MSH_CMD_EXPORT(debug, System debug infomation set.);
#endif  //DEBUG_EN

#ifdef RF_EN
#include "rf_thread.h"
#include "rf_if_app.h"
#include "rf_if_hw.h"
#include "bsp_flash/flash.h"
static void cat1_ctrl(int argc, char **argv) {
  extern t_rf_info rf_info;      //通讯模块信息
  if(argc == 1) {
    rt_kprintf("CAT1 info:\n");
    rt_kprintf("\tMAC: %s\n", rf_info.mac);
    rt_kprintf("\tAID: %s\n", rf_info.zhiyun_id);
    rt_kprintf("\tAKey: %s\n", rf_info.zhiyun_key);
    rt_kprintf("\tSIP: %s\n", rf_info.ip);
    rt_kprintf("\tPort: %u\n", rf_info.port);
  } else if(argc == 3) {
    if(!rt_strcmp(argv[1], "aid")) {
      strcpy(rf_info.zhiyun_id, argv[2]);
      rt_kprintf("CAT1 aid set: %s\n", rf_info.zhiyun_id);
    } else if(!rt_strcmp(argv[1], "akey")) {
      strcpy(rf_info.zhiyun_key, argv[2]);
      rt_kprintf("CAT1 akey set: %s\n", rf_info.zhiyun_key);
    }
  } else if(argc == 2) {
    if(!rt_strcmp(argv[1], "save")) {
      flash_write(FLASH_PARAM_START_ADDR, (uint16_t *)(&rf_info), sizeof(t_rf_info) / 2);
      rt_kprintf("CAT1 config save successed!\n");
    }
  }
}

int rf(int argc, char **argv) {
  extern uint8_t rf_Link_On;
  char rf_device[3][10] = {"NULL", "CAT1"};
  extern int cat1_rssi;
  if(argc == 1) {
    rt_kprintf("Version: %s\n", FIRMWARE_VER);
    rt_kprintf("RF interface device: %s\n", rf_device[rf_if_dev]);
    rt_kprintf("RF Link: %s\n", rf_Link_On==1 ? "ON" : "OFF");
    rt_kprintf("RF RSSI: %d\n", cat1_rssi);
  } else if(argc == 3 && !rt_strcmp(argv[1], "trans")) {    //透传命令
    rf_if_uart_send((uint8_t*)argv[2], strlen(argv[2]));
  }
  //按设备进行进一步处理
  switch(rf_if_dev) {
    case RF_IF_CAT1:
      cat1_ctrl(argc, argv);
    break;
    default: break;
  }
  return 0;
}
MSH_CMD_EXPORT(rf, RF Device Control.);
#endif

#ifdef REBOOT_EN
int reboot(int argc, char **argv) {
  //系统重启
  NVIC_SystemReset();
  while(1);
}
MSH_CMD_EXPORT(reboot, System Reboot.);
#endif
