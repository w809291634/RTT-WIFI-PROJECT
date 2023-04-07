/*********************************************************************************************
* �ļ���finger.c
* ���ߣ�Zhouchj 2020.07.23
* ˵����ָ��ģ����������
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
#include "finger.h"
#include <string.h>
#include <stdio.h>
#include "bsp_uart/usart.h"

/* ���ýӿ� */
#define FINGER_OS_SUPPORT                 1                     // OS����ϵͳ֧��
#include <rtthread.h>
//VERSION��0x02��ӦID_SEODU2_ID812_BYD160_Inne��0x03��ӦID_SYNONU2_ID811A1_FPC1021_Inner(80fp) V1.4
#define FINGER_VERSION                    0x03

#if FINGER_OS_SUPPORT == 1 
//#define FINGER_DEBUG                      rt_kprintf
#define FINGER_DEBUG                      xprintf
static struct rt_mutex finger_lock;
rt_event_t finger_event;
#else
#define FINGER_DEBUG                      printf
static unsigned char cmd_flags;                               // finger ָ����¼����
                                                                // λ0: ����� �ѷ���
                                                                // λ1: ��Ӧ�� �ѽ���
                                                                // λ2: ָ����� ָ�����ݰ�
                                                                // λ3: ָ����� ��Ӧ���ݰ�
                                                                // λ4: ָ�����ݰ��ѷ���
                                                                // λ5: ��Ӧ���ݰ��ѽ���
#define FLAG_CMD          {cmd_flags|=1<<0;}
#define FLAG_RES          {cmd_flags|=1<<1;}
//#define FLAG_CMD_EX       {cmd_flags|=1<<2;}
//#define FLAG_RES_EX       {cmd_flags|=1<<3;}
//#define FLAG_CMDD         {cmd_flags|=1<<4;}
#define FLAG_RESD         {cmd_flags|=1<<5;}
#define FLAG_RESET        {cmd_flags=0;}

#define FLAG_CMD_B        (1<<0)
#define FLAG_RES_B        (1<<1)
//#define FLAG_CMD_EX_B     (1<<2)
//#define FLAG_RES_EX_B     (1<<3)
//#define FLAG_CMDD_B       (1<<4)
#define FLAG_RESD_B       (1<<5)

#endif

// 2022��Э����ɫ�ƿ���
#if FINGER_VERSION==0x02
    unsigned int LED_G = 4, LED_R = 2, LED_B = 1;
    unsigned int LED_BREATHE = 2,LED_F_BLINK =4 ,LED_S_BLINK = 3 , LED_ON = 1,LED_OFF=0;
#elif FINGER_VERSION==0x03
    unsigned int LED_G = 1, LED_R = 2, LED_B = 4;
    unsigned int LED_BREATHE = 1,LED_F_BLINK =2 ,LED_S_BLINK = 7 , LED_ON = 3,LED_OFF=4;
#endif

//unsigned char fingerFlag = 0, enrollNum = 0, fingerId = 0, handleFlag = 0;
//static unsigned char lastFingerFlag = 0;
//void exti5_handle(void);

/* ���� */
static int finger_uartHandle(char ch);
void (*finger_sendByte_cb)(char data) = NULL;                     // finger �������ݵĺ���ָ��

static char Response_buf[26];                                     // ��Ӧ�� ����
static char Response_buf_len = 0;                                 // ����֡ͷ����
static char Data_buf[128];                                        // ָ��/��Ӧ�����ݰ� ����
static char Data_buf_len = 0;                                     // 

#define  RESP_CMD(resp) (resp[3]<<8 | resp[2]) 
#define  RESP_LEN(resp) (resp[5]<<8 | resp[4])
#define  RESP_DAT(resp) (&resp[6])


#define DEV_INFO_01 "PPM_BF5325_R160C_200Fp V1.2.1(Dec  9 2019)"
static char devinfo[64];

/* �����ⲿ�ӿ� */
/*********************************************************************************************
* ����: set_fingerSend()
* ����: ���� �������ݵ�ָ��ģ��� ����ָ��
* ����: �������ݵ� ���� ����
* ����: ��
*********************************************************************************************/
static void set_fingerSend(void (*fun)(char))
{
  finger_sendByte_cb = fun;
}

/*********************************************************************************************
* ����: finger_sendData()
* ����: �������ݵ�ָ��ģ��
* ����: data������  len������
* ����: ��
*********************************************************************************************/
static void finger_sendData(char *data, unsigned char len)
{
  for(unsigned char i=0; i<len; i++)
  {
    if(finger_sendByte_cb != NULL)
      finger_sendByte_cb(data[i]);
  }
}

/*********************************************************************************************
* ����: finger_recvData()
* ����: ����ָ��ģ�����е����� �жϴ�����
* ����: ch-����
* ����: ��
*********************************************************************************************/
int finger_recvData(char ch)
{
  finger_uartHandle(ch);
  return 0;
}

/* finger�ӿں��� */
/*********************************************************************************************
* ����: finger_check()
* ����: ָ��ģ�� ����У�����
* ����: *data-���� len-���ݳ���
* ����: У��ֵ
*********************************************************************************************/
unsigned short finger_check(char *data, unsigned char len)
{
  unsigned short checkVal = 0;
  for(unsigned char i=0; i<len; i++)
  {
    checkVal += data[i];
  }
  return checkVal;
}

/*********************************************************************************************
* ����: finger_request()
* ����: ��ͨ����
* ����: cmd: ָ��buf         clen:  ָ���
        resp���ظ�buf        rlen�� �ظ�����
        time����ʱʱ��
* ����: ��
*********************************************************************************************/
int finger_request(char *cmd, int clen, char* resp, int rlen, int time)
{
  int ret = -1;
#if FINGER_OS_SUPPORT == 1 
  rt_mutex_take(&finger_lock, RT_WAITING_FOREVER);
  rt_event_control(finger_event, RT_IPC_CMD_RESET, RT_NULL);
  Response_buf_len = 0;
  finger_sendData(cmd, clen);
  if(RT_EOK == rt_event_recv(finger_event, 1,
                           RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                           time, RT_NULL)) 
  {
    if (Response_buf_len > 0 && rlen >= Response_buf_len) {
      memcpy(resp, Response_buf, Response_buf_len);
      ret = Response_buf_len;
    }  
  }
  rt_mutex_release(&finger_lock);
#else
  if(cmd_flags==0){
    FLAG_CMD;
    Response_buf_len = 0;                                
    Data_buf_len = 0;      
    finger_sendData(cmd, clen);
//    if()
    
  }
#endif
  return ret;
}

/*********************************************************************************************
* ����: finger_request_with_data()
* ����: ����Ӧ���ݵ�����
* ����: ��
* ����: ��
*********************************************************************************************/
int finger_request_with_data(char *cmd, int clen, char* resp, int rlen, int time)
{
  int ret = -1;
#if FINGER_OS_SUPPORT == 1 
  rt_mutex_take(&finger_lock, RT_WAITING_FOREVER);
  rt_event_control(finger_event, RT_IPC_CMD_RESET, RT_NULL);
  Response_buf_len = 0;
  finger_sendData(cmd, clen);
  if(RT_EOK == rt_event_recv(finger_event, 1,
                           RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                           time, RT_NULL)) 
  {
    if (Response_buf_len > 0) {
      char *rbuf = &cmd[2];
      if (RESP_CMD(Response_buf) == RESP_CMD(rbuf)){
        if(RESP_DAT(Response_buf)[0] == E_SUCCESS)
        {
          /* ��ȡ��Ӧ���� */
          if(RT_EOK == rt_event_recv(finger_event, 1,
                         RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                         time, RT_NULL)) {
            if (Data_buf_len > 0 && rlen >= Data_buf_len) {
              memcpy(resp, Data_buf, Data_buf_len);
              ret = Data_buf_len;
            }
          }
        }
      }
    }
  }
  rt_mutex_release(&finger_lock);
#else

#endif
  return ret;
}


int finger_dev_info(char *buf, int len)
{
  char sendBuf[26] = {PREFIX&0xFF, PREFIX>>8, SID, DID, 4/*cmd L*/, 0/*cmd H*/, \
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  unsigned short checkVal = finger_check(sendBuf, 24);
  sendBuf[24] = checkVal & 0xFF;
  sendBuf[25] = checkVal >> 8;
   
  char recvBuf[64];
  int ret = finger_request_with_data(sendBuf, 26,  recvBuf, 64, 1000);
  FINGER_DEBUG("finger_request_with_data %d\r\n", ret);
  if (ret > 0) {
    if (RESP_CMD(recvBuf) == 0x04){
      if(RESP_DAT(recvBuf)[0] == E_SUCCESS) {
        memcpy(buf, &RESP_DAT(recvBuf)[2], RESP_LEN(recvBuf) - 2);
        buf[ RESP_LEN(recvBuf) - 2] =  '\0';
        ret = RESP_LEN(recvBuf) - 2;
      }
    }
 
  }
  return ret;
}


/*********************************************************************************************
* ����: finger_get_param()
* ����: ָ��ģ���ȡ����
* ����: type-��������(0-4)
*       0-���豸���(1-255)
*       1-��ȫ�ȼ�(1-5)
*       2-ָ���ظ����(0-1)
*       3-������(1-8) 1:9600 2:19200 3:38400 4:57600 5:115200 6:230400 7:460800 8:921600
*       4-ָ��ģ����ѧϰ(0-1)
* ����: ��
*********************************************************************************************/
void finger_get_param(unsigned char type)
{
  char sendBuf[26] = {PREFIX&0xFF, PREFIX>>8, SID, DID, GET_PARAM&0xFF, GET_PARAM>>8, \
                      0x01, 0x00, type, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  unsigned short checkVal = finger_check(sendBuf, 24);
  sendBuf[24] = checkVal & 0xFF;
  sendBuf[25] = checkVal >> 8;
  finger_sendData(sendBuf, 26);
}

/*********************************************************************************************
* ����: finger_set_param()
* ����: ָ��ģ�����ò���
* ����: type-��������(0-4)
*       0-���豸���(1-255)
*       1-��ȫ�ȼ�(1-5)
*       2-ָ���ظ����(0-1)
*       3-������(1-8) 1:9600 2:19200 3:38400 4:57600 5:115200 6:230400 7:460800 8:921600
*       4-ָ��ģ����ѧϰ(0-1)
*       val-����ֵ
* ����: ��
*********************************************************************************************/
void finger_set_param(unsigned char type, unsigned char val)
{
  char sendBuf[26] = {PREFIX&0xFF, PREFIX>>8, SID, DID, GET_PARAM&0xFF, GET_PARAM>>8, \
                      0x05, 0x00, type, val, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  unsigned short checkVal = finger_check(sendBuf, 24);
  sendBuf[24] = checkVal & 0xFF;
  sendBuf[25] = checkVal >> 8;
  finger_sendData(sendBuf, 26);
}

/*********************************************************************************************
* ����: finger_detect()
* ����: ָ��ģ�����Ƿ���ָ��
* ����: ��
* ����: ��
*********************************************************************************************/
int finger_detect(void)
{
  char sendBuf[26] = {PREFIX&0xFF, PREFIX>>8, SID, DID, FINGER_DETECT&0xFF, FINGER_DETECT>>8, \
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  unsigned short checkVal = finger_check(sendBuf, 24);
  sendBuf[24] = checkVal & 0xFF;
  sendBuf[25] = checkVal >> 8;
  //finger_sendData(sendBuf, 26);
  char recvBuf[26];
  int ret = finger_request(sendBuf, 26,  recvBuf, 26, 1000);
  if (ret > 0) {
      if (RESP_CMD(recvBuf) == FINGER_DETECT){
        if(RESP_DAT(recvBuf)[0] == E_SUCCESS)
        {
          if(RESP_DAT(recvBuf)[2] == 0x00)  
            return 0;
          else return 1;//��⵽��ָ
        }
      }
  }
  return -1;
}

/*********************************************************************************************
* ����: finger_get_status()
* ����: ָ��ģ����ָ���ı���Ƿ��ѱ�ע��
* ����: id-���
* ����: ��
*********************************************************************************************/
int finger_get_status(unsigned char id)
{
  char sendBuf[26] = {PREFIX&0xFF, PREFIX>>8, SID, DID, GET_STATUS&0xFF, GET_STATUS>>8, \
                      0x02, 0x00, id, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  unsigned short checkVal = finger_check(sendBuf, 24);
  sendBuf[24] = checkVal & 0xFF;
  sendBuf[25] = checkVal >> 8;
  //finger_sendData(sendBuf, 26);
  char recvBuf[26];
  int ret = finger_request(sendBuf, 26,  recvBuf, 26, 1000);
  if (ret > 0) {
      if (RESP_CMD(recvBuf) == GET_STATUS){
        if(RESP_DAT(recvBuf)[0] == E_SUCCESS)
        {
          if(RESP_DAT(recvBuf)[2] == 0x00) // δע��
            return 0;
          else return 1;                  // ��ע��
        }
      }
  }
  return -1;
}

/*********************************************************************************************
* ����: finger_get_image()
* ����: ָ��ģ��ɼ�ͼ��
* ����: ��
* ����: ��
*********************************************************************************************/
int finger_get_image(void)
{
  char sendBuf[26] = {PREFIX&0xFF, PREFIX>>8, SID, DID, GET_IMAGE&0xFF, GET_IMAGE>>8, \
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  unsigned short checkVal = finger_check(sendBuf, 24);
  sendBuf[24] = checkVal & 0xFF;
  sendBuf[25] = checkVal >> 8;
  //finger_sendData(sendBuf, 26);
  char recvBuf[26];
  int ret = finger_request(sendBuf, 26,  recvBuf, 26, 1000);
  if (ret > 0) {
      if (RESP_CMD(recvBuf) == GET_IMAGE){
        if(RESP_DAT(recvBuf)[0] == E_SUCCESS)
        {
          return 1;  //�ɼ��ɹ�
        }
      }
      return 0;
  }
  return -1;

}

/*********************************************************************************************
* ����: finger_rgb_ctrl()
* ����: ָ��ģ��RGB����
* ����: type-��ʾ״̬ start-��ʼ��ɫ end-������ɫ num-����
* ����: ��
*********************************************************************************************/
int finger_rgb_ctrl(unsigned char type, unsigned char start, unsigned char end, unsigned char num)
{
#if FINGER_VERSION==0x02
  char sendBuf[26] = {PREFIX&0xFF, PREFIX>>8, SID, DID, SLED_CTRL&0xFF, SLED_CTRL>>8, \
                      0x02, 0x00, type, 0x80|start, end, num, 0x00, 0x00, 0x00, 0x00, \
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#elif FINGER_VERSION==0x03
  char sendBuf[26] = {PREFIX&0xFF, PREFIX>>8, SID, DID, SLED_CTRL&0xFF, SLED_CTRL>>8, \
                      0x04, 0x00, type, start, end, num, 0x00, 0x00, 0x00, 0x00, \
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#endif
  if (strcmp(devinfo, DEV_INFO_01) == 0) {
    end = start;
    char __sendBuf[26] = {PREFIX&0xFF, PREFIX>>8, SID, DID, SLED_CTRL&0xFF, SLED_CTRL>>8, \
                      0x04, 0x00, type, start, end, num, 0x00, 0x00, 0x00, 0x00, \
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    memcpy(sendBuf, __sendBuf, 26);
    
  }  
 
  unsigned short checkVal = finger_check(sendBuf, 24);
  sendBuf[24] = checkVal & 0xFF;
  sendBuf[25] = checkVal >> 8;
  //finger_sendData(sendBuf, 26);
  char recvBuf[26];
  int ret = finger_request(sendBuf, 26,  recvBuf, 26, 1000);
  if (ret > 0) {
      if (RESP_CMD(recvBuf) == SLED_CTRL){
        if(RESP_DAT(recvBuf)[0] == E_SUCCESS)
        {
          return 1;  //���óɹ�
        }
      }
      return 0;
  }
  return -1;
}

/*********************************************************************************************
* ����: finger_generate()
* ����: ָ��ģ�����ɲ�����ģ������
* ����: num-armbuff���(0-2)
* ����: ��
*********************************************************************************************/
int finger_generate(unsigned char num)
{
  char sendBuf[26] = {PREFIX&0xFF, PREFIX>>8, SID, DID, GENERATE&0xFF, GENERATE>>8, \
                      0x02, 0x00, num, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  unsigned short checkVal = finger_check(sendBuf, 24);
  sendBuf[24] = checkVal & 0xFF;
  sendBuf[25] = checkVal >> 8;
  //finger_sendData(sendBuf, 26);
  char recvBuf[26];
  int ret = finger_request(sendBuf, 26,  recvBuf, 26, 1000);
  if (ret > 0) {
      if (RESP_CMD(recvBuf) == GENERATE){
        if(RESP_DAT(recvBuf)[0] == E_SUCCESS)
        {
          return 1;  //�ɹ�
        }
      }
      return 0;
  }
  return -1;
}

/*********************************************************************************************
* ����: finger_merge()
* ����: ָ��ģ��ϳ�ָ��ģ������
* ����: num-rambuff���(0-2) count-�ϳ�����(2-3)
* ����: ��
*********************************************************************************************/
int finger_merge(unsigned char num, unsigned char count)
{
  char sendBuf[26] = {PREFIX&0xFF, PREFIX>>8, SID, DID, MERGE&0xFF, MERGE>>8, \
                      0x03, 0x00, num, 0x00, count, 0x00, 0x00, 0x00, 0x00, 0x00, \
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  unsigned short checkVal = finger_check(sendBuf, 24);
  sendBuf[24] = checkVal & 0xFF;
  sendBuf[25] = checkVal >> 8;
  //finger_sendData(sendBuf, 26);
  char recvBuf[26];
  int ret = finger_request(sendBuf, 26,  recvBuf, 26, 1000);
  if (ret > 0) {
      if (RESP_CMD(recvBuf) == MERGE){
        if(RESP_DAT(recvBuf)[0] == E_SUCCESS)
        {
          return 1;  //�ɹ�
        }
      }
      return 0;
  }
  return -1;
}

/*********************************************************************************************
* ����: finger_store_char()
* ����: ָ��ģ�鱣��ָ��ģ��
* ����: num-rambuff���(0-2) id-template���(1-n)
* ����: ��
*********************************************************************************************/
int finger_store_char(unsigned char num, unsigned char id)
{
  char sendBuf[26] = {PREFIX&0xFF, PREFIX>>8, SID, DID, STORE_CHAR&0xFF, STORE_CHAR>>8, \
                      0x04, 0x00, id, 0x00, num, 0x00, 0x00, 0x00, 0x00, 0x00, \
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  unsigned short checkVal = finger_check(sendBuf, 24);
  sendBuf[24] = checkVal & 0xFF;
  sendBuf[25] = checkVal >> 8;
  //finger_sendData(sendBuf, 26);
  char recvBuf[26];
  int ret = finger_request(sendBuf, 26,  recvBuf, 26, 1000);
  if (ret > 0) {
      if (RESP_CMD(recvBuf) == STORE_CHAR){
        if(RESP_DAT(recvBuf)[0] == E_SUCCESS)
        {
          return 1;  //�ɹ�
        }
      }
      return 0;
  }
  return -1;
}

/*********************************************************************************************
* ����: finger_search()
* ����: ָ��ģ��ָ����Χ1:Nʶ��
* ����: num-rambuff���(0-2) startId-��ʼ��� endId-�������
* ����: ��
*********************************************************************************************/
int finger_search(unsigned char num, unsigned char startId, unsigned char endId)
{
  char sendBuf[26] = {PREFIX&0xFF, PREFIX>>8, SID, DID, SEARCH&0xFF, SEARCH>>8, \
                      0x06, 0x00, num, 0x00, startId, 0x00, endId, 0x00, 0x00, 0x00, \
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  unsigned short checkVal = finger_check(sendBuf, 24);
  sendBuf[24] = checkVal & 0xFF;
  sendBuf[25] = checkVal >> 8;
  //finger_sendData(sendBuf, 26);
  char recvBuf[26];
  int ret = finger_request(sendBuf, 26,  recvBuf, 26, 1000);
  if (ret > 0) {
      if (RESP_CMD(recvBuf) == SEARCH){
        if(RESP_DAT(recvBuf)[0] == E_SUCCESS)
        {
          return 1;  //�ɹ�
        }
      }
      return 0;
  }
  return -1;
}

/*********************************************************************************************
* ����: finger_init()
* ����: ָ��ģ���ʼ��
* ����: fun 
* ����: ��
*********************************************************************************************/
void finger_init(void)
{
#if FINGER_OS_SUPPORT == 1 
  /* ���ڽӿڳ�ʼ�� */
  set_fingerSend(uart2_putc);       
  uart2_set_input(finger_uartHandle);
  uart2_init(115200);
  /* �����¼� */
  if (finger_event == NULL) {
    finger_event = rt_event_create("finger_event", RT_IPC_FLAG_PRIO);
    rt_mutex_init(&finger_lock, "mutex_ginger", RT_IPC_FLAG_PRIO);
  }
#else
  set_fingerSend(Uart0_Send_char);       
  Uart0CallbackSet(finger_uartHandle);
  uart0_init(115200);
#endif

  if (finger_dev_info(devinfo, sizeof devinfo) > 0) {
    xprintf("finger mode info: %s\r\n", devinfo);
    if (strcmp(devinfo, DEV_INFO_01) == 0) {
      LED_G = 1, LED_R = 2, LED_B = 4; 
      LED_BREATHE = 1,LED_F_BLINK =2 ,LED_S_BLINK = 7, LED_ON = 3,LED_OFF=4;
    }
  }
}

/*********************************************************************************************
* ����: finger_uartHandle()
* ����: ָ��ģ�鴮�����ݴ���
* ����: ch-���յ����ַ�
* ����: ��
*********************************************************************************************/
int finger_uartHandle(char ch)
{
  static char recBuf[128] = {0};
  static unsigned char flag = 0, recLen = 0;
  unsigned short cmd = 0;
  unsigned char len = 0;
  switch(flag)
  {
  case 0:
    if(ch == 0xAA) flag = 1;
    if (ch == 0xA5) flag = 3;
    break;
    
  case 1:
    if(ch == 0x55) flag = 2;
    else flag = 0;
    break;
    
  case 2:
    /* ��Ӧ�� */
    recBuf[recLen++] = ch;
    if (recLen >= 24) {                             // ����֡ͷ2�ֽ�
      cmd = (recBuf[3] << 8) | recBuf[2];
      len = (recBuf[5] << 8) | recBuf[4];
      //finger_handle(cmd, recBuf+6, len);
      memcpy(Response_buf, recBuf, recLen);
      Response_buf_len = recLen;
#if FINGER_OS_SUPPORT == 1 
      rt_event_send(finger_event, 1);
#else
      FLAG_RES;    
#endif
      flag = 0;
      recLen = 0;
    }
    break;
    
  case 3:
    if (ch == 0x5A) flag = 4;
    else flag = 0;
    break;
    
  case 4:
    /* ��Ӧ���ݰ� */
    recBuf[recLen++] = ch;
    if (recLen > 6) {
       len = (recBuf[5] << 8) | recBuf[4];
       if (len == recLen+8) {
          memcpy(Data_buf, recBuf, recLen);
          Data_buf_len = recLen;
#if FINGER_OS_SUPPORT == 1 
          rt_event_send(finger_event, 1);
#else
          FLAG_RESD;
#endif
          flag = 0;
          recLen = 0;
       }
    } 
    if (recLen >= sizeof recBuf) {
      flag = 0;
      recLen = 0;
    }
    break;
    
  default:
    flag = 0;
    recLen = 0;
  }
}
