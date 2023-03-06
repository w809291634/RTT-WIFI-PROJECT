/*********************************************************************************************
* 文件：stepmotor.c
* 作者：zonesion 2016.4.13
* 说明：电机驱动程序
*       通过P0_0、P0_1端口控制电机的工作
* 修改：Chenkm 2017.01.10 修改代码格式，增加代码注释和文件说明
* 注释：
*********************************************************************************************/

/*********************************************************************************************
* 头文件
*********************************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "stepmotor.h"
#include "apl_delay/delay.h"
#include "stm32f10x.h"
/*********************************************************************************************
* 宏定义
*********************************************************************************************/
/*********************************************************************************************
* 全局变量
*********************************************************************************************/
static unsigned int dir = 0;

void stepmotor_init(void)
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
  GPIO_SetBits(PIN_EN_port,PIN_EN);
}

/*********************************************************************************************
* 名称：step(int dir,int steps)
* 功能：电机单步
* 参数：int dir,int steps
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
void step(int dir,int steps)
{
  int i;
  if (dir) 
    GPIO_SetBits(PIN_DIR_port,PIN_DIR);											//步进电机方向设置
  else 
    GPIO_ResetBits(PIN_DIR_port,PIN_DIR);	
  delay_us(5);						        //延时5us
  for (i=0; i<steps; i++){				        //步进电机旋转
   GPIO_ResetBits(PIN_STEP_port,PIN_STEP);
   delay_us(80);
   GPIO_SetBits(PIN_STEP_port,PIN_STEP);
   delay_us(80);
  }
}

/*********************************************************************************************
* 名称：forward()
* 功能：电机正转
* 参数：无
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
void forward(int data)
{
  dir = 0;
  GPIO_ResetBits(PIN_EN_port,PIN_EN);                           //步进电机方向设置
  step(dir, data);                              	        //启动步进电机
  GPIO_SetBits(PIN_EN_port,PIN_EN);
}

/*********************************************************************************************
* 名称：reversion()
* 功能：电机反转
* 参数：无
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
void reversion(int data)
{
  dir = 1;														//步进电机方向设置
  GPIO_ResetBits(PIN_EN_port,PIN_EN); 
  step(dir, data);												//启动步进电机

  GPIO_SetBits(PIN_EN_port,PIN_EN);

}