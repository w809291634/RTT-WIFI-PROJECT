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
#define SENSOR_TYPE  601

typedef struct {
  float     A0;     //�¶�ֵ��0.1����
  float     A1;     //ʪ��ֵ��0.1����
  float     A2;     //���նȣ�0.1����
  int       A3;     //��������������
  float     A4;     //����ѹ����0.1����
  int       A5;     //����״̬��������
  float     A6;     //�����࣬0.1����
  int       A7;     //����ռλ
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
