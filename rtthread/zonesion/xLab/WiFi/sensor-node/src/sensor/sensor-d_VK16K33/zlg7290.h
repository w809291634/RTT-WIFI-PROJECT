/*********************************************************************************************
* 文件：zlg7290.h
* 作者：zonesion
* 说明：zlg7290头文件
* 修改：Chenkm 2017.01.10 修改代码格式，增加代码注释和文件说明
* 注释：
*********************************************************************************************/
#ifndef __ZLG7290_H__
#define __ZLG7290_H__

/*********************************************************************************************
* 宏定义
*********************************************************************************************/
#define	   ZLG7290ADDR          0x70		                    //ZLG7290的IIC地址
#define	   SYETEM_REG           0x00		                    //系统寄存器
#define	   KEY_REG              0x01		                    //键值寄存器
#define	   REPEATCNT_REG        0x02		                    //连击次数寄存器
#define	   FUNCTIONKEY          0x03		                    //功能键寄存器
#define	   CMDBUF0              0x07		                    //命令缓冲器0
#define	   CMDBUF1              0x08		                    //命令缓冲器1
#define	   FALSH                0x0C		                    //闪烁控制寄存器
#define	   SCANNUM              0x0D		                    //扫描位数寄存器
#define	   DPRAM0               0x10		                    //显示缓存寄存器0
#define	   DPRAM1               0x11		                    //显示缓存寄存器1
#define	   DPRAM2               0x12		                    //显示缓存寄存器2
#define	   DPRAM3               0x13		                    //显示缓存寄存器3
#define	   DPRAM4               0x14		                    //显示缓存寄存器4
#define	   DPRAM5               0x15		                    //显示缓存寄存器5
#define	   DPRAM6               0x16		                    //显示缓存寄存器6
#define	   DPRAM7               0x17		                    //显示缓存寄存器7

#define	   UP                   0x01		                    //上
#define	   LEFT                 0x02		                    //左
#define	   DOWN                 0x03		                    //下
#define	   RIGHT                0x04		                    //右
#define	   CENTER               0x05		                    //中


/*********************************************************************************************
* 外部原型函数
*********************************************************************************************/
void  zlg7290_init(void);
unsigned char zlg7290_read_reg(unsigned char cmd);
void zlg7290_write_data(unsigned char cmd, unsigned char data);

void zlg7290_download(unsigned char addr, unsigned dp, unsigned char flash, unsigned char dat);
void segment_display(unsigned int num, unsigned char status);
unsigned char zlg7290_get_keyval(void);
void display_off(void);
#endif //__ZLG7290_H__