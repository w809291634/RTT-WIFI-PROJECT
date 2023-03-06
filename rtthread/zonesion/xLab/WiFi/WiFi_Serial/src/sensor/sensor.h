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

#define SENSOR_TYPE  504
#define DEFAULT_POS  "114.406319&30.462533"
#define PI 3.1415926535898
#define FIX_ANGLE   0  /* ��װ��ʽ��0--x������洹ֱ��1--y������洹ֱ��2--z������洹ֱ */

typedef struct {
  float     A0;     //��ʷ��������m3��ÿ��ֵ�б仯ʱ�洢��SPIFlash��
  float     A1;     //��ѹ��kPa��-100~100��
  int       A2;     //ȼ��Ũ�ȣ�ppm��0-20000��
  int       A3;     //����ռλ
  int       A4;     //����ռλ
  int       A5;     //�㵹�Ƕȣ����ͣ�0-90�ȣ�
  int       A6;     //������0/1��
  int       A7;     //©����0/1��
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
