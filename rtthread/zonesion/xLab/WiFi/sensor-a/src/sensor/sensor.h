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
#define SENSOR_TYPE  601

typedef struct {
  float     A0;     //温度值，0.1精度
  float     A1;     //湿度值，0.1精度
  float     A2;     //光照度，0.1精度
  int       A3;     //空气质量，整型
  float     A4;     //大气压力，0.1精度
  int       A5;     //跌倒状态，开关量
  float     A6;     //红外测距，0.1精度
  int       A7;     //保留占位
  uint8_t   D0;     //A0~A7的主动上报使能
  uint8_t   D1;     //气阀控制：bit0--管道气阀；bit1--入户气阀
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
