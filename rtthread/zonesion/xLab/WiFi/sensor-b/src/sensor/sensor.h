/*********************************************************************************************
* 文件：sensor.h
* 作者：
* 说明：通用传感器控制接口程序头文件
* 修改：
* 注释：
*********************************************************************************************/
#ifndef _sensor_h_
#define _sensor_h_

#include <stdint.h>
#define SENSOR_TYPE  602

typedef struct {
  float     A0;     //
  float     A1;     //
  float     A2;     //
  int       A3;     //
  float     A4;     //
  int       A5;     //
  float     A6;     //
  int       A7;     //保留占位
  uint8_t   D0;     //A0~A7的主动上报使能
  uint8_t   D1;     //控制量
  uint16_t  V0;     //主动上报时间间隔（s）
  char      V1[20]; //保留占位
  char      V2[20]; //保留占位
  char      V3[28]; //保留占位
  float     cali[8];//传感器校准值
} t_sensor_value;

extern t_sensor_value sensor_value;

void sensor_init(void);
void sensorLinkOn(void);
void sensorUpdate(void);
void sensorCheck(void);
void sensorControl(uint8_t cmd);
int z_process_command_call(char* ptag, char* pval, char* obuf);
void sensor_Keypressed(void);
void sensor_para_init(void);
void sensor_cali(uint8_t ch, int value);

#endif
