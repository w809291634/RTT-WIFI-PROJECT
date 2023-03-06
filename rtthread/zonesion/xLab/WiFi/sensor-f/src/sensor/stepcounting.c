/*********************************************************************************************
* 文件：stepcounting.c
* 作者：zonesion
* 说明：步数计算程序
* 修改：Meixin 2017.05.31 增加了注释
* 注释：
*********************************************************************************************/

/*********************************************************************************************
* 头文件
*********************************************************************************************/
//#include "arm_math.h"
//#include "arm_const_structs.h"
#include <stdio.h>
#include <stdint.h>
#include "stdlib.h"

void mpu9250_init(void);
int mpu9250_accel(float *x, float *y, float *z);

static long total_step_cnt = 0;                                       //总步数记数
/*********************************************************************************************
* 定义
*********************************************************************************************/
//频率分辨率 0.390625Hz 范围 0 - 12.5 Hz
//调用周期 6.4秒

#define N       64                                              //采样个数
#define Fs      10                                              //采样频率
#define F_P     (((float)Fs)/N)

int stepGet(void)
{
  return total_step_cnt;
}
/*********************************************************************************************
* 名称：int stepcounting(float32_t* test_f32)
* 功能：步数计算函数
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
int stepcounting(float* test_f32)
{
  uint32_t ifftFlag = 0;                                        //傅里叶逆变换标志位
  uint32_t doBitReverse = 1;                                    //翻转标志位
  float testOutput[N/2];                                    //输出数组
  uint32_t i; 
//  arm_cfft_f32(&arm_cfft_sR_f32_len64, test_f32, ifftFlag, doBitReverse);//傅里叶变换
//  arm_cmplx_mag_f32(test_f32, testOutput, N/2);  
  float max = 0; 
  uint32_t mi = 0;  
  for (i=0; i<N/2; i++) {
    float a = testOutput[i];
    if (i == 0) a = testOutput[i]/(N);
    else a = testOutput[i]/(N/2);
    if (i != 0 && a > max && i*F_P <= 5.4f) {
        mi = i;
        max = a;
    }
  }
  if (max > 1.5) {
      int sc = 0;
      sc = (int)(mi * F_P * (1.0/Fs)*N);
      if (sc >= 3 && sc < 30) {
       return sc;
      }
  }
  return 0;
}

void step_run(void)
{
  static float acc_input[64*2];
  static uint32_t acc_len = 0;
  static int step_cnt = 0;                                             //步数记数
  static char tick = 9;
  
  if (++tick == 10) {
    tick = 0;
       /* 计步数据存储*/
      if (step_cnt > 10) {
        total_step_cnt += step_cnt;
        step_cnt = 0;                                               //步数清零
      }
  }
  float x, y, z;
  mpu9250_accel(&x, &y, &z);                                        //读取xyz三轴的数据
  float a = sqrt(x*x + y*y + z*z);                                 //开平方根
  acc_input[acc_len*2] = a;
  acc_input[acc_len*2+1] = 0;
  acc_len ++;
  if (acc_len == 64) acc_len = 0;
  if (acc_len == 0) {           
    step_cnt += stepcounting(acc_input);          //总步数累加
  }
}

void step_init(void) {
  mpu9250_init();                                                       //MPU9250初始化 
}
