#include "stm32f10x.h"
#include <stdlib.h>
#include <string.h>

#include "bsp_uart/usart.h"

#define DEV_TIMEOUT_MS          250

void clock_delay_ms(unsigned int ms);

 static char u8Buff[64];        //串口接收缓存
 static uint8_t rxLen = 0;        //串口实际接收到数据长度
 static uint8_t rxPackage = 0;
#pragma optimize=none

static void delay_ms(uint8_t ms)
{
  clock_delay_ms(ms);
}

static char xor(char *buf, int len)
{
  char x = 0;
  for (int i=0; i<len; i++) {
    x ^= buf[i];
  }
  return x;
}
static int uartRecvCallBack(char ch)
{
  if (rxLen == 0 && ch == 0xCD) {
    u8Buff[0] = 0xCD;
    rxLen = 1;
  } else if (rxLen == 1 && ch == 0xDC) {
    u8Buff[1] = 0xDC;
    rxLen = 2;
  } else if (rxLen >= 2 && rxLen < sizeof u8Buff) {
    u8Buff[rxLen++] = ch;
    if (rxLen >= 6 && (u8Buff[4]+6) == rxLen) {
      //got package
      if (xor(&u8Buff[2], rxLen-2) == 0) {
        rxPackage = rxLen;
      } else {
        rxLen = 0; //无效数据包
      }
    }  
  } else {
    rxLen = 0;
  }
  return 0;
}
static int RFID7941WriteWait(char *cmd, int wlen)
{
  rxLen = 0;
  rxPackage = 0;
  uint8_t to = DEV_TIMEOUT_MS;
  for (int i=0; i<wlen; i++) {
    uart2_putc(cmd[i]);
  }
  while(rxPackage == 0 && to-- > 0) delay_ms(1);
  
  return  rxPackage;
}
int RFID7941CheckCard125kHz(char *cid)
{
  char cmd[] =  {0xAB,0xBA,0x00,0x15,0x00,0x15};
  RFID7941WriteWait(cmd, sizeof cmd);
  if (rxPackage == 11 && u8Buff[3] == 0x81) {
    memcpy(cid, &u8Buff[5], 5);
    return 5;
  }
  return 0;
}
int RFID7941CheckCard1356MHz(char *cid)
{
  char cmd[] =  {0xAB,0xBA,0x00,0x10,0x00,0x10};
  RFID7941WriteWait(cmd, sizeof cmd);
  if (rxPackage == 10 && u8Buff[3] == 0x81) {
    memcpy(cid, &u8Buff[5], 4);
    return 4;
  }
  return 0;
}
int RFID7941ReadCard1356MHz(char blk, char* key, char* buf)
{
  char cmd[] = {0xAB,0xBA,0x00,0x12,0x09,
    0x00,0x01,
    0x0A,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x10
  };
  cmd[6] = blk;
  memcpy(&cmd[8], key, 6);
  cmd[14] = xor(&cmd[2], 12);
  RFID7941WriteWait(cmd, sizeof cmd);
  if (rxPackage == 24 && u8Buff[3] == 0x81) {
      memcpy(buf, &u8Buff[7], 16);
      return 16;
  }
  return 0;
}

int RFID7941WriteCard1356MHz(char blk, char* key, char* buf)
{
  char cmd[] = {0xAB,0xBA,0x00,0x13,0x19,
    0x00,0x01,
    0x0A,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
  };
  cmd[6] = blk;
  memcpy(&cmd[8], key, 6);
  memcpy(&cmd[14], buf, 16);
  cmd[14] = xor(&cmd[2], sizeof cmd - 2);
  RFID7941WriteWait(cmd, sizeof cmd);
  if (rxPackage == 6 && u8Buff[3] == 0x81){
    return 1;
  }
  return 0;
}
static void RFID7941WriteReq(char *cmd, int len)
{
  rxLen = 0;
  rxPackage = 0;
  for (int i=0; i<len; i++) {
    uart2_putc(cmd[i]);
  }
}
void RFID7941CheckCard125kHzReq(void)
{
  char cmd[] =  {0xAB,0xBA,0x00,0x15,0x00,0x15};
  RFID7941WriteReq(cmd, sizeof cmd);
  return ;
}
int RFID7941CheckCard125kHzRsp(char *cid)
{
  if (rxPackage == 11 && u8Buff[3] == 0x81) {
    memcpy(cid, &u8Buff[5], 5);
    return 5;
  }
  return 0; 
}

void RFID7941CheckCard1356MHzReq(void)
{
  char cmd[] =  {0xAB,0xBA,0x00,0x10,0x00,0x10};
  RFID7941WriteReq(cmd, sizeof cmd);
}
int RFID7941CheckCard1356MHzRsp(char *cid)
{
  if (rxPackage == 10 && u8Buff[3] == 0x81) {
    memcpy(cid, &u8Buff[5], 4);
    return 4;
  }
  return 0;
}


void RFID7941ReadCard1356MHzReq(char blk, char* key)
{
  char cmd[] = {0xAB,0xBA,0x00,0x12,0x09,
    0x00,0x01,
    0x0A,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x10
  };
  cmd[6] = blk;
  memcpy(&cmd[8], key, 6);
  cmd[14] = xor(&cmd[2], 12);
  RFID7941WriteReq(cmd, sizeof cmd);
}
int RFID7941ReadCard1356MHzRsp(char* buf)
{
  if (rxPackage == 24 && u8Buff[3] == 0x81) {
      memcpy(buf, &u8Buff[7], 16);
      return 16;
  }
  return 0;
}


void RFID7941WriteCard1356MHzReq(char blk, char* key, char* buf)
{
  char cmd[] = {0xAB,0xBA,0x00,0x13,0x19,
    0x00,0x01,
    0x0A,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
  };
  cmd[6] = blk;
  memcpy(&cmd[8], key, 6);
  memcpy(&cmd[14], buf, 16);
  cmd[14] = xor(&cmd[2], sizeof cmd - 2);
  RFID7941WriteReq(cmd, sizeof cmd);
}
int RFID7941WriteCard1356MHzRsp(void)
{
  if (rxPackage == 6 && u8Buff[3] == 0x81){
    return 1;
  }
  return 0;
}
void RFID7941Init(void)
{
  uart2_init(115200);
  uart2_set_input(uartRecvCallBack);
}