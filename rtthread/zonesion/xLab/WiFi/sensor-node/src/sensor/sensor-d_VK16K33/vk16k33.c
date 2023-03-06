/*********************************************************************************************
* �ļ���vk16k33.c
* ���ߣ�xuzhy
* ˵����vk16k33��������
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/

/*********************************************************************************************
* ͷ�ļ�
*********************************************************************************************/

#include <math.h>
#include <stdio.h>
#include <string.h>
#include "bsp_i2c/iic2.h"
#include "apl_delay/delay.h"


#define REPLACE_ZLG7290   

#define VK16K33_ADDRESS 0x70

#define VK16K33_CMD_OSC_OFF 0x20
#define VK16K33_CMD_OSC_ON 0x21

#define VK16K33_CMD_DISPLAY_OFF 0x80
#define VK16K33_CMD_DISPLAY_ON 0x81

#define VK16K33_INT_LOW 0xA1

#define VK16K33_CMD_BRIGHT(x) (0xE0|(x&0x0f))

#define wk16k33_write_one_byte(cmd)  iic2_write_buf(VK16K33_ADDRESS,cmd, NULL,0)
#define wk16k33_write_two_byte(cmd1,cmd2) do{   \
    char c = cmd2;                              \
    iic2_write_buf(VK16K33_ADDRESS,cmd1, &c,1); \
  }while(0)
    

static char onoff = 0;
static char ledbuf[16];
/*********************************************************************************************
* ���ƣ�vk16k33_init
* ���ܣ�vk16k33��ʼ��
* ��������
* ���أ���
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
#ifdef REPLACE_ZLG7290
void zlg7290_init(void)
#else
void vk16k33_init(void)
#endif
{
  iic2_init();		                                        //I2C��ʼ��
  wk16k33_write_one_byte(VK16K33_CMD_OSC_ON);
  wk16k33_write_one_byte(VK16K33_CMD_BRIGHT(4));
  //wk16k33_write_one_byte(VK16K33_CMD_DISPLAY_ON);
  wk16k33_write_one_byte(VK16K33_INT_LOW);
  //ExtInt_init(Ext_P0,Ext_Pin4,Ext_dowm);
}


/*********************************************************************************************
* ���ƣ�segment_display()
* ���ܣ��������ʾ����
* ������num -- ���ݣ����9999��
* ���أ�
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
void segment_display(unsigned int num)
{
  unsigned char h = 0,j = 0,k = 0,l = 0;
  if(num > 9999)
    num = 0;
  h = num % 10;
  j = num % 100 /10;
  k = num % 1000 / 100;
  l = num /1000;

  char map[10] = {0x3f,0x06, 0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f};
  ledbuf[0] = map[l];
  ledbuf[2] = map[k];
  ledbuf[4] = map[j];
  ledbuf[6] = map[h];
  iic2_write_buf(VK16K33_ADDRESS, 0, ledbuf,16);
  if (onoff == 0) {
    wk16k33_write_one_byte(VK16K33_CMD_DISPLAY_ON);
    onoff = 1;
  }
}
void display_off(void)
{
  wk16k33_write_one_byte(VK16K33_CMD_DISPLAY_OFF);
  onoff = 0;
}

#define	   UP                   0x01		                    //��
#define	   LEFT                 0x02		                    //��
#define	   DOWN                 0x03		                    //��
#define	   RIGHT                0x04		                    //��
#define	   CENTER               0x05		                    //��

#ifdef REPLACE_ZLG7290
unsigned char zlg7290_get_keyval(void)
#else
unsigned char vk16k33_get_keyval(void)
#endif
{
  static char lastBuf[1];
  char buf[1] = {0};
  char __buf[6];

    if (6 == iic2_read_buf(VK16K33_ADDRESS, 0x40, __buf, 6)){
      if (lastBuf[0] == 0 && __buf[0] != 0) {
        buf[0] = __buf[0];
      } 
      lastBuf[0] = __buf[0];
      if (buf[0] & 0x01) {
          return DOWN;
      }
      if (buf[0] & 0x02) {
        return RIGHT;
      }
      if (buf[0] & 0x04){
        return CENTER;
      }
      if (buf[0] & 0x08){
        return UP;
      }
      if (buf[0] & 0x10){
        return LEFT;
      }
    }
  return 0;
}

#ifdef REPLACE_ZLG7290
char zlg7290_read_reg(char reg)
{
  return 0;
}
#endif