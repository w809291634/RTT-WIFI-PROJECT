/*********************************************************************************************
* 文件：stadiometry.c
* 作者：Lixm 2017.10.17
* 说明：红外测距驱动代码
* 修改：
* 注释：
*********************************************************************************************/

/*********************************************************************************************
* 头文件
*********************************************************************************************/
#include "stadiometry.h"
#include "apl_delay/delay.h"
#include <math.h>
#include <stm32f10x.h>
/*********************************************************************************************
* 名称：stadiometry_init()
* 功能：红外测距传感器初始化
* 参数：无
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
void stadiometry_init(void)
{
  ADC_InitTypeDef ADC_InitStructure; 
  GPIO_InitTypeDef GPIO_InitStructure;  
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |RCC_APB2Periph_ADC1, ENABLE );//使能ADC1通道时钟   
  RCC_ADCCLKConfig(RCC_PCLK2_Div6);                             //设置ADC分频因子6 72M/6=12,ADC最大时间不能超过14M 
  
  //PA1 作为模拟通道输入引脚                         
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;		        //模拟输入引脚
  GPIO_Init(GPIOA, &GPIO_InitStructure);	 
  ADC_DeInit(ADC1);                                             //复位ADC1   
  ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;	        //ADC工作模式:ADC1和ADC2工作在独立模式
  ADC_InitStructure.ADC_ScanConvMode = DISABLE;	                //模数转换工作在单通道模式
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;	        //模数转换工作在单次转换模式
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;//转换由软件而不是外部触发启动
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;	//ADC数据右对齐
  ADC_InitStructure.ADC_NbrOfChannel = 1;	                //顺序进行规则转换的ADC通道的数目
  ADC_Init(ADC1, &ADC_InitStructure);	                        //根据ADC_InitStruct中指定的参数初始化外设ADCx的寄存器   
    
  ADC_Cmd(ADC1, ENABLE);	                                //使能指定的ADC1  
  ADC_ResetCalibration(ADC1);	                                //使能复位校准    
  while(ADC_GetResetCalibrationStatus(ADC1));	                //等待复位校准结束  
  ADC_StartCalibration(ADC1);	                                //开启AD校准
  while(ADC_GetCalibrationStatus(ADC1));	                //等待校准结束
}

/*********************************************************************************************
* 名称：float get_stadiometry_data(void)
* 功能：获取红外测距传感器状态
* 参数：无
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
float get_stadiometry_data(void)
{ 
  unsigned int  value = 0;

  //设置指定ADC的规则组通道，一个序列，采样时间
  ADC_RegularChannelConfig(ADC1, 7, 1, ADC_SampleTime_239Cycles5 );//ADC1,ADC通道,采样时间为239.5周期	  			    
  ADC_SoftwareStartConvCmd(ADC1, ENABLE);		        //使能指定的ADC1的软件转换启动功能	
  while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC ));               //等待转换结束
  value = ADC_GetConversionValue(ADC1);	                        //返回最近一次ADC1规则组的转换结果
  //A6：红外测距；根据传感器手册的曲线图，拟合出的公式为：dis=27.092*(vol^-1.273)
  double vol = value / 4096.0 * 3.3;    //12位分辨率，最高位为符号位，所以除以2048
  return (float)(27.092 * pow(vol, -1.273));
  
}