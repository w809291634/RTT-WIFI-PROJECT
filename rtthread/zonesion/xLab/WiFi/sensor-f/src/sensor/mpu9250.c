/*********************************************************************************************
* 文件：mpu9250.c
* 作者：zonesion
* 说明：陀螺仪驱动程序
* 修改：Chenkm 2017.01.05 增加了注释
* 注释：
*********************************************************************************************/
/*********************************************************************************************
* 头文件
*********************************************************************************************/
#include <rtthread.h>
#include "stm32f10x.h"	
#include "bsp_i2c/iic.h"
#include "stdio.h"

/*********************************************************************************************
* 宏定义
*********************************************************************************************/
#define MPU9250_ADDR    0x68                                    //a0 -->0
#define AK8963_ADDR     0x0c
#define MPU6515_ADDR    0x68                                    //a0 -->0
#define ACCEL_ADDRESS   MPU6515_ADDR
#define MAG_ADDRESS     AK8963_ADDR
// 定义MPU9250内部地址
#define	SMPLRT_DIV	0x19	                                //陀螺仪采样率，典型值：0x07(125Hz)
#define	CONFIG		0x1A	                                //低通滤波频率，典型值：0x06(5Hz)
#define	GYRO_CONFIG	0x1B	                                //陀螺仪自检及测量范围，典型值：0x18(不自检，2000deg/s)
#define	ACCEL_CONFIG	0x1C	                                //加速计自检、测量范围及高通滤波频率，典型值：0x01(不自检，2G，5Hz)
#define	ACCEL_XOUT_H	0x3B
#define	ACCEL_XOUT_L	0x3C
#define	ACCEL_YOUT_H	0x3D
#define	ACCEL_YOUT_L	0x3E
#define	ACCEL_ZOUT_H	0x3F
#define	ACCEL_ZOUT_L	0x40
#define	TEMP_OUT_H	0x41
#define	TEMP_OUT_L	0x42
#define	GYRO_XOUT_H	0x43
#define	GYRO_XOUT_L	0x44	
#define	GYRO_YOUT_H	0x45
#define	GYRO_YOUT_L	0x46
#define	GYRO_ZOUT_H	0x47
#define	GYRO_ZOUT_L	0x48		
#define MAG_XOUT_L	0x03
#define MAG_XOUT_H	0x04
#define MAG_YOUT_L	0x05
#define MAG_YOUT_H	0x06
#define MAG_ZOUT_L	0x07
#define MAG_ZOUT_H	0x08
#define	PWR_MGMT_1	0x6B	                                //电源管理，典型值：0x00(正常启用)
#define	WHO_AM_I	0x75	                                //IIC地址寄存器(默认数值0x68，只读)

/*********************************************************************************************
* 内部原形函数
*********************************************************************************************/
 
/*********************************************************************************************
* 名称: Single_Write()
* 功能: 写数据
* 参数: a -- 地址，r -- 发送数据，v -- 数组名
* 返回: 发送数字长度
* 修改:
* 注释: 
**********************************************************************************************/
int Single_Write(char a, char r, char v)
{
  return iic_write_buf(a, r, &v, 1);
}

/*********************************************************************************************
* 名称: Single_Read()
* 功能: 读数据
* 参数: 无
* 返回: 无
* 修改:
* 注释: 
**********************************************************************************************/
int Single_Read(char a, char r)
{
  char v;
  int ret; 
  ret = iic_read_buf(a, r, &v, 1);
  if (ret == 1) {
    return v;
  }
  return -1;
}
/*********************************************************************************************
* 名称: mpu9250_init()
* 功能: mpu9250初始化
* 参数: 无
* 返回: 无
* 修改:
* 注释: 
**********************************************************************************************/
void mpu9250_init(void)
{
  int id;
  rt_thread_mdelay(10);
  iic_init();
  id = Single_Read(MPU9250_ADDR,0x75);
  if (id > 0){
    //printf("mpu9250 id %02x\r\n", id);
  }else{
//    printf("mpu9250 error: can't find ic\r\n");
  }
  Single_Write(MPU9250_ADDR,PWR_MGMT_1, 0x00);	                //解除休眠状态

  Single_Write(MPU9250_ADDR,0x37,0x02);//turn on Bypass Mode
  Single_Write(MPU9250_ADDR,SMPLRT_DIV, 0x07);
  Single_Write(MPU9250_ADDR,CONFIG, 0x05);                      //10Hz
  Single_Write(MPU9250_ADDR,GYRO_CONFIG, 0x18);
  Single_Write(MPU9250_ADDR,ACCEL_CONFIG, 0x01);                //0x01--2G   0x04 -- 4G
}

/*********************************************************************************************
* 名称: mpu9250_accel()
* 功能: 读取xyz三轴的数据
* 参数: x -- x轴数据 y -- y轴数据  z -- z轴数据
* 返回: 0
* 修改:
* 注释: 
**********************************************************************************************/
int mpu9250_accel(float *x, float *y, float *z)
{ 
  char BUF[6];
  int16_t T_X, T_Y, T_Z;   
   BUF[0]=Single_Read(MPU6515_ADDR,ACCEL_XOUT_L); 
   BUF[1]=Single_Read(MPU6515_ADDR,ACCEL_XOUT_H);
   T_X=	(BUF[1]<<8)|BUF[0];						   
   *x = T_X/1640.0f;                                            //读取计算X轴数据
   BUF[2]=Single_Read(MPU6515_ADDR,ACCEL_YOUT_L);
   BUF[3]=Single_Read(MPU6515_ADDR,ACCEL_YOUT_H);
   T_Y=	(BUF[3]<<8)|BUF[2];					        
   *y = T_Y/1640.0f;                                            //读取计算Y轴数据   
   BUF[4]=Single_Read(MPU6515_ADDR,ACCEL_ZOUT_L);
   BUF[5]=Single_Read(MPU6515_ADDR,ACCEL_ZOUT_H);
   T_Z=	(BUF[5]<<8)|BUF[4];				
   *z = T_Z/1640.0f;                                            //读取计算Z轴数据
   return 0;
}

/*********************************************************************************************
* 名称: mpu9250_gyro()
* 功能: 计算xyz三轴的数据
* 参数: 无
* 返回: 0
* 修改:
* 注释:  gyro 范围2000°/s ， 32768 / 2000 = 16.384；
**********************************************************************************************/
int mpu9250_gyro(int16_t *x, int16_t *y, int16_t *z)
{ 
  char BUF[6];
  int16_t T_X, T_Y, T_Z;
  BUF[0]=Single_Read(MPU6515_ADDR,GYRO_XOUT_L); 
  BUF[1]=Single_Read(MPU6515_ADDR,GYRO_XOUT_H);
  T_X=	(BUF[1]<<8)|BUF[0];
  T_X/=16.4; 						        //读取计算X轴数据
  BUF[2]=Single_Read(MPU6515_ADDR,GYRO_YOUT_L);
  BUF[3]=Single_Read(MPU6515_ADDR,GYRO_YOUT_H);
  T_Y=	(BUF[3]<<8)|BUF[2];
  T_Y/=16.4; 						        //读取计算Y轴数据
  BUF[4]=Single_Read(MPU6515_ADDR,GYRO_ZOUT_L);
  BUF[5]=Single_Read(MPU6515_ADDR,GYRO_ZOUT_H);
  T_Z=	(BUF[5]<<8)|BUF[4];
  T_Z/=16.4; 					                //读取计算Z轴数据
  //printf("gyro %d, %d, %d \r\n", T_X, T_Y, T_Z);
  
  *x = T_X;
  *y = T_Y;
  *z = T_Z;
  return 0;
}


int mpu9250_mga(int16_t *x, int16_t *y, int16_t *z)
{ 
  char BUF[6];
  int16_t T_X, T_Y, T_Z;
  
  
  Single_Write(AK8963_ADDR,0x0A,0x01);  
  rt_thread_mdelay(10);
  
  BUF[0]=Single_Read (AK8963_ADDR,MAG_XOUT_L);
  BUF[1]=Single_Read (AK8963_ADDR,MAG_XOUT_H);
  T_X=(BUF[1]<<8)|BUF[0];  
  BUF[2]=Single_Read(AK8963_ADDR,MAG_YOUT_L);
  BUF[3]=Single_Read(AK8963_ADDR,MAG_YOUT_H);
  T_Y=	(BUF[3]<<8)|BUF[2];
  //读取计算Y轴数据  
  BUF[4]=Single_Read(AK8963_ADDR,MAG_ZOUT_L);
  BUF[5]=Single_Read(AK8963_ADDR,MAG_ZOUT_H);
  T_Z=	(BUF[5]<<8)|BUF[4];
  //读取计算Z轴数据
  //printf("mga %d, %d, %d \r\n", T_X, T_Y, T_Z);
  *x = T_X;
  *y = T_Y;
  *z = T_Z;
  
  return 0;
}