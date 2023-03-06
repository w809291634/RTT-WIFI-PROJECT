/*********************************************************************************************
* �ļ���sensor.h
* ���ߣ�
* ˵����ͨ�ô��������ƽӿڳ���ͷ�ļ�
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
#ifndef _sensor_h_
#define _sensor_h_

#include <stdint.h>
#define SENSOR_TYPE  606

typedef struct {
  int       A0;     //
  int       A1;     //
  int       A2;     //�����
  int       A3;     //
  int       A4;     //
  int       A5;     //
  int       A6;     //
  int       A7;     //
  uint8_t   D0;     //A0~A7�������ϱ�ʹ��
  uint8_t   D1;     //�������ƣ�bit0--�ܵ�������bit1--�뻧����
  uint16_t  V0;     //�����ϱ�ʱ������s��
  char      V1[20]; //����ռλ
  char      V2[20]; //����ռλ
  char      V3[28]; //��γ�ȣ�dddmm.mmmmmm&ddmm.mmmmmm
  float     cali[8];//������У׼ֵ
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
