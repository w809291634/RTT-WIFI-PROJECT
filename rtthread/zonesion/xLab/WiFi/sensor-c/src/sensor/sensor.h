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
#define SENSOR_TYPE  603

typedef struct {
  int       A0;     //人体红外/触摸状态值，0或者1变化
  int       A1;     //震动状态值，0或者1变化
  int       A2;     //霍尔状态值，0或者1变化
  int       A3;     //火焰状态值，0或者1变化
  int       A4;     //燃气报警状态值，0或者1变化
  int       A5;     //光栅状态值，0或者1变化
  int       A6;     //保留占位
  int       A7;     //保留占位
  uint8_t   D0;     //A0~A7的主动上报使能
  uint8_t   D1;     //Bit6、Bit7控制继电器状态，用OD1/CD1改变状态
  uint16_t  V0;     //主动上报时间间隔（s）
  char      V1[20]; //保留占位
  char      V2[20]; //保留占位
  char      V3[28]; //经纬度：dddmm.mmmmmm&ddmm.mmmmmm
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
