/*********************************************************************************************
* �ļ���finger.h
* ���ߣ�Zhouchj 2020.07.23
* ˵����ָ��ģ������ͷ�ļ�
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
#ifndef __FINGER_H_
#define __FINGER_H_

#define PREFIX              0xAA55
#define SID                 0x00
#define DID                 0x00
        
#define ENROLL_NUM          0x03                                // ע��ɼ�ָ�ƴ��� 2/3
#define TOUCH               0x20                                // ����
#define ENROLL_T            0x40                                // ע��ɹ�
#define ENROLL_F            0x80                                // ע��ʧ��
#define IDENTIFY_T          0x40                                // ָ����֤�ɹ�
#define IDENTIFY_F          0x80                                // ָ����֤ʧ��

#define SEARCH_NUM          0x05                                // ָ����֤ģ������
#define IDENTIFY_INTERVAL   0x03                                // ��֤���ʱ�� ��λ��S

/* LED */                    
#define LED_ALL             0x07                                // ȫ����
//#define LED_OFF             0x00                                // �ر�
//#define LED_ON              0x01 
extern unsigned int LED_G, LED_R, LED_B;
extern unsigned int LED_BREATHE,LED_F_BLINK,LED_S_BLINK, LED_ON,LED_OFF;

#if 0
#define OLD_F 0
#if OLD_F
#define LED_G               0x01                                // �̵�
#define LED_R               0x02                                // ���
#define LED_B               0x04                                // ����

#define LED_ON              0x01                                // ��
#define LED_BREATHE         0x01                                // ����
#define LED_F_BLINK         0x02                                // ����˸
#define LED_LONG_ON         0x03                                // ����
#define LED_LONG_OFF        0x04                                // ����
#define LED_GRADUALLY_ON    0x05                                // ����
#define LED_GRADUALLY_OFF   0x06                                // ����
#define LED_S_BLINK         0x07                                // ����˸
#else
#define LED_B               0x01                                // ����
#define LED_R               0x02                                // ���
#define LED_G               0x04                                // �̵�

#define LED_ON              0x01                                // ��
#define LED_BREATHE         0x02                                // ����

#define LED_S_BLINK         0x03                                // ����˸
#define LED_F_BLINK         0x04                                // ����˸

#define LED_LONG_ON         0x01                                // ����
#define LED_LONG_OFF        0x00                                // ����
#define LED_GRADUALLY_ON    1                                // ����
#define LED_GRADUALLY_OFF   0                                // ����

#endif
#endif

/* ָ�� */
// CMD              
#define GET_PARAM           0x0003                              // ��ȡ����
#define GET_IMAGE           0x0020                              // ��ȡͼ��
#define FINGER_DETECT       0x0021                              // ���ָ��
#define SLED_CTRL           0x0024                              // �������
#define STORE_CHAR          0x0040                              // ע�ᵽ��ſ�
#define GET_STATUS          0x0046                              // ��ȡע��״̬
#define GENERATE            0X0060                              // ����ģ��
#define MERGE               0x0061                              // �ں�ģ��
#define SEARCH              0x0063                              // 1:N ��֤
#define VERIFY              0x0064                              // 1:1 ��֤
  
// RET  
#define E_SUCCESS           0x00
#define E_FAIL              0x01
#define E_VERIFY            0x10
#define E_IDENTIFY          0x11
#define E_TMPL_EMPTY        0x12
#define E_MERGE_FAIL        0x1A

/* �ӿں��� */
//int finger_recvData(char ch);
//void set_fingerSend(void (*fun)(char));

void finger_init(void);

#endif
