/*********************************************************************************************
* 文件：led.h
* 作者：Meixin 2017.09.15
* 说明：led灯驱动代码头文件  
* 修改：liutong 20171027 修改了LED控制引脚初始化函数名称、优化了注释
* 注释：
*********************************************************************************************/

/*********************************************************************************************
* 宏条件编译
*********************************************************************************************/
#ifndef __LED_H__
#define __LED_H__

/*********************************************************************************************
* 头文件
*********************************************************************************************/
#include <rtthread.h>

/*********************************************************************************************
* 宏定义
*********************************************************************************************/
#define LED1                      GPIO_Pin_7                    //宏定义LED1灯控制引脚P1_1
#define LED2                      GPIO_Pin_6                    //宏定义LED2灯控制引脚P1_0

#define LED_port                  GPIOA      

/*********************************************************************************************
* 函数声明
*********************************************************************************************/
void ext_led_init(void);                                            //LED控制引脚初始化函数
signed char ext_led_on(unsigned char led);                          //LED打开控制函数
signed char ext_led_off(unsigned char led);                         //LED关闭控制函数

#endif /*__LED_H_*/

