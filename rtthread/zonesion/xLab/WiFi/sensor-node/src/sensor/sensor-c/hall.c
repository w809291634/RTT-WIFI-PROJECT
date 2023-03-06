/*********************************************************************************************
* 文件：Hall.c
* 作者：Lixm 2017.10.17
* 说明：霍尔驱动代码
* 修改：
* 注释：
*********************************************************************************************/

/*********************************************************************************************
* 头文件
*********************************************************************************************/
#include "hall.h"

/*********************************************************************************************
* 名称：hall_init()
* 功能：霍尔传感器初始化
* 参数：无
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
void hall_init(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;
 	
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	 //使能PA端口时钟
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
}

/*********************************************************************************************
* 名称：unsigned char get_hall_status(void)
* 功能：获取霍尔传感器状态
* 参数：无
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
unsigned char get_hall_status(void)
{ 
  if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_3))                                                      //霍尔传感器检测引脚
    return 0;                                                   //没有检测到信号返回 0
  else
    return 1;                                                   //检测到信号返回 1
}