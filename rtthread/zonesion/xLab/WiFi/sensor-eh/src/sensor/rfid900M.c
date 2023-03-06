#include "stm32f10x.h"
#include <stdlib.h>
#include <string.h>

#include "bsp_uart/usart.h"

#define DEV_TIMEOUT_MS          400

void clock_delay_ms(unsigned int ms);

 static char u8Buff[64];        //串口接收缓存
 static uint8_t rxLen = 0;        //串口实际接收到数据长度
 static uint8_t rxPackage = 0;
#pragma optimize=none

static void delay_ms(uint8_t ms)
{
  clock_delay_ms(ms);
}

static unsigned short CRC16_CCITT_FALSE(unsigned char *puchMsg, unsigned int usDataLen)  
{  
  unsigned short wCRCin = 0xFFFF;  
  unsigned short wCPoly = 0x1021;  
  unsigned char wChar = 0;  
  
  while (usDataLen--)     
  {  
    wChar = *(puchMsg++);  
    wCRCin ^= (wChar << 8);  
    for(int i = 0;i < 8;i++)  
    {  
      if(wCRCin & 0x8000)  
        wCRCin = (wCRCin << 1) ^ wCPoly;  
      else  
        wCRCin = wCRCin << 1;  
    }  
  }  
  return (wCRCin) ;  
}
static int uartRecvCallBack(char ch)
{
  if (rxLen == 0 && ch == 0xBB) {
    rxLen = 1;
    u8Buff[0] = ch;
  } else if (rxLen > 0) {
    if (rxLen < sizeof u8Buff) {
      u8Buff[rxLen++] = ch;
      if (rxLen >= 8 && rxLen == (((u8Buff[3]<<8) | u8Buff[4])+8)) {
        unsigned short crc = CRC16_CCITT_FALSE((unsigned char*)&u8Buff[1], rxLen-3);
        if (crc == ((u8Buff[rxLen-2]<<8)|u8Buff[rxLen-1])) {
            rxPackage = rxLen;
        }
        rxLen = 0;
      }
    } else rxLen = 0;
  }
  return 0;
}
static int RFID900MWriteWait(char *cmd, int wlen)
{
  rxLen = 0;
  rxPackage = 0;
  int to = DEV_TIMEOUT_MS;
  for (int i=0; i<wlen; i++) {
    uart2_putc(cmd[i]);
  }
  while(rxPackage == 0 && to-- > 0) delay_ms(1);
  
  return  rxPackage;
}

int RFID900MCheckCard(char *cid)
{
  /* read type c uii, response pc+epc*/
  char cmd[] =  {0xBB,0x00,0x22,0x00,0x00,0x7E,0x54,0x73};
  RFID900MWriteWait(cmd, sizeof cmd);
  if (rxPackage == 22 && u8Buff[2] == 0x22) {
    memcpy(cid, &u8Buff[7], 12);
    return 12;
  }
  return 0;
}
/*********************************************************************************************
* 名称：RFID900MRead
* 功能：读取type c标签内存
* 参数：ap: 4字节访问密码，NULL 默认0x00 0x00 0x00 0x00
*       epc: 标签epc，由RFID900MCheckCard读取的
*       mb：标签内存区域：RFU 0x00， EPC：0x01， TID 0x02， User 0x03
*       sa：起始地址
*       dl：数据长度字为单位
*      out：读取缓存
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
int RFID900MRead(char* ap, char* epc, int mb, int sa, int dl, char *out)
{
  /*read type c tag memory */
  char cmd[] = {0xBB,0x00,0x29,0x00,0x17,0x00,0x00,0x00,0x00,0x00,0x0C,0xE2,
  0x00,0x30,0x32,0x76,0x13,0x01,0x52,0x21,0x40,0x34,0x6D,0x03,0x00,0x00,0x00,
  0x04,0x7E,0x49,0xF2};
  if (ap != NULL) {
    cmd[5] = ap[0], cmd[6] = ap[1], cmd[7] = ap[2], cmd[8] = ap[3];
  }
  memcpy(&cmd[11], epc, 12); //epc
  cmd[23] = mb;
  cmd[24] = sa>>8;
  cmd[25] = sa & 0xff;
  cmd[26] = dl>>8;
  cmd[27] = dl & 0xff;
  unsigned int c = CRC16_CCITT_FALSE((unsigned char*)&cmd[1], 28);
  cmd[29] = c>>8;
  cmd[30] = c & 0xff;
  
  RFID900MWriteWait(cmd, sizeof cmd);
  int rlen = dl*2;
  if (rxPackage == (rlen+8) && u8Buff[2] == 0x29) {
    memcpy(out, &u8Buff[5], rlen);
    return rlen;
  }
  return 0;
}
/*********************************************************************************************
* 名称：RFID900MWrite
* 功能：写入type c标签内存
* 参数：ap: 4字节访问密码，NULL 默认0x00 0x00 0x00 0x00
*       epc: 标签epc，由RFID900MCheckCard读取的
*       mb：标签内存区域：RFU 0x00， EPC：0x01， TID 0x02， User 0x03
*       sa：起始地址
*       dl：数据长度，字为单位
*       in: 待写入数据
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
int RFID900MWrite(char* ap, char* epc, int mb, int sa, int dl, char *in)
{
  static char cmd[96] = {0xBB,0x00,0x46,0x00,0x17,0x00,0x00,0x00,0x00,0x00,0x0C,0xE2,
  0x00,0x30,0x32,0x76,0x13,0x01,0x52,0x21,0x40,0x34,0x6D,0x03,0x00,0x00,0x00,
  0x04,0x7E,0x49,0xF2};
  if (dl > 32) return -1;
  if (ap != NULL) {
    cmd[5] = ap[0], cmd[6] = ap[1], cmd[7] = ap[2], cmd[8] = ap[3];
  }
  memcpy(&cmd[11], epc, 12); //len(epc)+epc
  cmd[23] = mb;
  cmd[24] = sa>>8;
  cmd[25] = sa & 0xff;
  cmd[26] = dl>>8;
  cmd[27] = dl & 0xff;
  memcpy(&cmd[28], in, dl*2);
  int wlen = 28 + dl*2;
  cmd[wlen++] = 0x7e;
  unsigned int c = CRC16_CCITT_FALSE((unsigned char*)&cmd[1], wlen-1);
  cmd[wlen++] = c>>8;
  cmd[wlen++] = c & 0xff;
  
  cmd[3] = (wlen-8)>>8;
  cmd[4] = (wlen-8)&0xff;
  RFID900MWriteWait(cmd, wlen);

  if (rxPackage == 9 && u8Buff[2] == 0x46) {
    if (u8Buff[5] == 0) return dl;
    else return -2;
  }
  return 0;
}


int RFID900GetTxPower(void)
{
  char cmd[] = {0xBB, 0x00, 0x15, 0x00, 0x00, 0x7e, 0x00, 0x00};
  unsigned int c = CRC16_CCITT_FALSE((unsigned char*)&cmd[1], 5);
  cmd[6] = c >> 8;
  cmd[7] = c & 0xff;
  
  RFID900MWriteWait(cmd, sizeof cmd);
  if (rxPackage == 10 && u8Buff[2] == 0x15) {
    return u8Buff[5]<< 8 | u8Buff[6];
  }
  return -1;
}

int RFID900SetTxPower(int pow)
{
  char cmd[] = {0xBB, 0x00, 0x16, 0x00, 0x02, 0x00, 0x00, 0x7e, 0x00, 0x00};
  unsigned int c = CRC16_CCITT_FALSE((unsigned char*)&cmd[1], 5);
  cmd[5] = pow >> 8;
  cmd[6] = pow & 0xff;
  cmd[8] = c >> 8;
  cmd[9] = c & 0xff;
  
  RFID900MWriteWait(cmd, sizeof cmd);
  if (rxPackage == 9 && u8Buff[2] == 0x16) {
    return u8Buff[5];
  }
  return -1;
}

static int RFID900MReq(char *cmd, int wlen)
{
  rxLen = 0;
  rxPackage = 0;

  for (int i=0; i<wlen; i++) {
    uart2_putc(cmd[i]);
  }
  return  wlen;
}

void RFID900MCheckCardReq(void)
{
  /* read type c uii, response pc+epc*/
  char cmd[] =  {0xBB,0x00,0x22,0x00,0x00,0x7E,0x54,0x73};
  RFID900MReq(cmd, sizeof cmd);
  return ;
}
int RFID900MCheckCardRsp(char *cid)
{
  if (rxPackage == 22 && u8Buff[2] == 0x22) {
    memcpy(cid, &u8Buff[7], 12);
    return 12;
  }  
  return 0;
}

void RFID900MReadReq(char* ap, char* epc, int mb, int sa, int dl)
{
  /*read type c tag memory */
  char cmd[] = {0xBB,0x00,0x29,0x00,0x17,0x00,0x00,0x00,0x00,0x00,0x0C,0xE2,
  0x00,0x30,0x32,0x76,0x13,0x01,0x52,0x21,0x40,0x34,0x6D,0x03,0x00,0x00,0x00,
  0x04,0x7E,0x49,0xF2};
  if (ap != NULL) {
    cmd[5] = ap[0], cmd[6] = ap[1], cmd[7] = ap[2], cmd[8] = ap[3];
  }
  memcpy(&cmd[11], epc, 12); //epc
  cmd[23] = mb;
  cmd[24] = sa>>8;
  cmd[25] = sa & 0xff;
  cmd[26] = dl>>8;
  cmd[27] = dl & 0xff;
  unsigned int c = CRC16_CCITT_FALSE((unsigned char*)&cmd[1], 28);
  cmd[29] = c>>8;
  cmd[30] = c & 0xff;
  
  RFID900MReq(cmd, sizeof cmd);
  return ;
}

int RFID900MReadRsp(char *out)
{
  int rlen = (u8Buff[3]<<8 | u8Buff[4]);
  if (rxPackage == (rlen+8) && u8Buff[2] == 0x29) {
    memcpy(out, &u8Buff[5], rlen);
    return rlen;
  }
  return 0;
}

void RFID900MWriteReq(char* ap, char* epc, int mb, int sa, int dl, char *in)
{
  static char cmd[96] = {0xBB,0x00,0x46,0x00,0x17,0x00,0x00,0x00,0x00,0x00,0x0C,0xE2,
  0x00,0x30,0x32,0x76,0x13,0x01,0x52,0x21,0x40,0x34,0x6D,0x03,0x00,0x00,0x00,
  0x04,0x7E,0x49,0xF2};
  if (dl > 32) return;
  if (ap != NULL) {
    cmd[5] = ap[0], cmd[6] = ap[1], cmd[7] = ap[2], cmd[8] = ap[3];
  }
  memcpy(&cmd[11], epc, 12); //len(epc)+epc
  cmd[23] = mb;
  cmd[24] = sa>>8;
  cmd[25] = sa & 0xff;
  cmd[26] = dl>>8;
  cmd[27] = dl & 0xff;
  memcpy(&cmd[28], in, dl*2);
  int wlen = 28 + dl*2;
  cmd[wlen++] = 0x7e;
  unsigned int c = CRC16_CCITT_FALSE((unsigned char*)&cmd[1], wlen-1);
  cmd[wlen++] = c>>8;
  cmd[wlen++] = c & 0xff;
  
  cmd[3] = (wlen-8)>>8;
  cmd[4] = (wlen-8)&0xff;
  RFID900MReq(cmd, wlen); 
}
int RFID900MWriteRsp(void)
{
  if (rxPackage == 9 && u8Buff[2] == 0x46) {
    if (u8Buff[5] == 0) return 1;
    else return -2;
  }
  return 0;
}

void RFID900MInit(void)
{
  uart2_init(115200);
  uart2_set_input(uartRecvCallBack);
}