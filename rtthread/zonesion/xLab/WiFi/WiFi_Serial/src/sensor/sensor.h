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

#define SENSOR_TYPE  504
#define DEFAULT_POS  "114.406319&30.462533"
#define PI 3.1415926535898
#define FIX_ANGLE   0  /* 安装方式：0--x轴与地面垂直；1--y轴与地面垂直；2--z轴与地面垂直 */

typedef struct {
  float     A0;     //历史用气量（m3，每次值有变化时存储到SPIFlash）
  float     A1;     //气压（kPa，-100~100）
  int       A2;     //燃气浓度（ppm，0-20000）
  int       A3;     //保留占位
  int       A4;     //保留占位
  int       A5;     //倾倒角度（整型，0-90度）
  int       A6;     //堵塞（0/1）
  int       A7;     //漏气（0/1）
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
