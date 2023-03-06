/*********************************************************************************************
* 文件：rgb.c
* 作者：Meixin 2017.09.15
* 说明：rgb灯驱动代码     
* 修改：liutong 20171027 修改了rgb控制引脚初始化函数名称、优化了注释、增加了rgb打开和关闭函数
* 注释：
*********************************************************************************************/

/*********************************************************************************************
* 头文件
*********************************************************************************************/
#include "rgb.h"
#include "stm32f10x.h"
/*********************************************************************************************
* 名称：rgb_init()
* 功能：rgb控制引脚初始化
* 参数：无
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
void rgb_init(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;
  
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	        //使能PA端口时钟
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	        //使能PB端口时钟
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_0;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  rgb_off(0x01);                                                //初始状态为关闭
  rgb_off(0x02);    
  rgb_off(0x04);    
}
  
/*********************************************************************************************
* 名称：rgb_on()
* 功能：rgb控制打开函数
* 参数：rgb号，在rgb.h中宏定义为RGB_R，RGB_G，RGB_B
* 返回：0，打开rgb成功，-1，参数错误
* 修改：
* 注释：参数只能填入RGB_R，RGB_G，RGB_B否则会返回-1
*********************************************************************************************/
signed char rgb_on(unsigned char rgb)
{
  if(rgb & 0x01){                                                //如果要打开RGB_R
    GPIO_ResetBits(RGB_R_port,RGB_R);
    return 0;
  }
    
  if(rgb & 0x02){                                                //如果要打开RGB_G
    GPIO_ResetBits(RGB_G_port,RGB_G);
    return 0;
  }
  
  if(rgb & 0x04){                                                //如果要打开RGB_B
    GPIO_ResetBits(RGB_B_port,RGB_B);
    return 0;
  }
   
  return -1;                                                    //参数错误，返回-1
}

/*********************************************************************************************
* 名称：rgb_off()
* 功能：rgb控制关闭函数
* 参数：rgb号，在rgb.h中宏定义为RGB_R，RGB_G，RGB_B
* 返回：0，关闭rgb成功，-1，参数错误
* 修改：
* 注释：参数只能填入RGB_R，RGB_G，RGB_B否则会返回-1
*********************************************************************************************/
signed char rgb_off(unsigned char rgb)
{
  if(rgb & 0x01){                                                //如果要关闭RGB_R
    GPIO_SetBits(RGB_R_port,RGB_R);
    return 0;
  }
    
  if(rgb & 0x02){                                                //如果要关闭RGB_G
    GPIO_SetBits(RGB_G_port,RGB_G);
    return 0;
  }

  if(rgb & 0x04){                                                //如果要关闭RGB_B
    GPIO_SetBits(RGB_B_port,RGB_B);
    return 0;
  }
  
  return -1;                                                    //参数错误，返回-1
}