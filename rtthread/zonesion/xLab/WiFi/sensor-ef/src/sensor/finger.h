/*********************************************************************************************
* 文件：finger.h
* 作者：Zhouchj 2020.07.23
* 说明：指纹模块驱动头文件
* 修改：
* 注释：
*********************************************************************************************/
#ifndef __FINGER_H_
#define __FINGER_H_

#define PREFIX              0xAA55
#define SID                 0x00
#define DID                 0x00
        
#define ENROLL_NUM          0x03                                // 注册采集指纹次数 2/3
#define TOUCH               0x20                                // 触摸
#define ENROLL_T            0x40                                // 注册成功
#define ENROLL_F            0x80                                // 注册失败
#define IDENTIFY_T          0x40                                // 指纹验证成功
#define IDENTIFY_F          0x80                                // 指纹验证失败

#define SEARCH_NUM          0x05                                // 指纹验证模板数量
#define IDENTIFY_INTERVAL   0x03                                // 验证间隔时长 单位：S

/* LED */                    
#define LED_ALL             0x07                                // 全部灯
//#define LED_OFF             0x00                                // 关闭
//#define LED_ON              0x01 
extern unsigned int LED_G, LED_R, LED_B;
extern unsigned int LED_BREATHE,LED_F_BLINK,LED_S_BLINK, LED_ON,LED_OFF;

#if 0
#define OLD_F 0
#if OLD_F
#define LED_G               0x01                                // 绿灯
#define LED_R               0x02                                // 红灯
#define LED_B               0x04                                // 蓝灯

#define LED_ON              0x01                                // 打开
#define LED_BREATHE         0x01                                // 呼吸
#define LED_F_BLINK         0x02                                // 快闪烁
#define LED_LONG_ON         0x03                                // 常开
#define LED_LONG_OFF        0x04                                // 常闭
#define LED_GRADUALLY_ON    0x05                                // 渐开
#define LED_GRADUALLY_OFF   0x06                                // 渐关
#define LED_S_BLINK         0x07                                // 慢闪烁
#else
#define LED_B               0x01                                // 蓝灯
#define LED_R               0x02                                // 红灯
#define LED_G               0x04                                // 绿灯

#define LED_ON              0x01                                // 打开
#define LED_BREATHE         0x02                                // 呼吸

#define LED_S_BLINK         0x03                                // 慢闪烁
#define LED_F_BLINK         0x04                                // 快闪烁

#define LED_LONG_ON         0x01                                // 常开
#define LED_LONG_OFF        0x00                                // 常闭
#define LED_GRADUALLY_ON    1                                // 渐开
#define LED_GRADUALLY_OFF   0                                // 渐关

#endif
#endif

/* 指令 */
// CMD              
#define GET_PARAM           0x0003                              // 获取参数
#define GET_IMAGE           0x0020                              // 获取图像
#define FINGER_DETECT       0x0021                              // 检测指纹
#define SLED_CTRL           0x0024                              // 背光控制
#define STORE_CHAR          0x0040                              // 注册到编号库
#define GET_STATUS          0x0046                              // 获取注册状态
#define GENERATE            0X0060                              // 生成模板
#define MERGE               0x0061                              // 融合模块
#define SEARCH              0x0063                              // 1:N 验证
#define VERIFY              0x0064                              // 1:1 验证
  
// RET  
#define E_SUCCESS           0x00
#define E_FAIL              0x01
#define E_VERIFY            0x10
#define E_IDENTIFY          0x11
#define E_TMPL_EMPTY        0x12
#define E_MERGE_FAIL        0x1A

/* 接口函数 */
//int finger_recvData(char ch);
//void set_fingerSend(void (*fun)(char));

void finger_init(void);

#endif
