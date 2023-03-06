/*********************************************************************************************
* 文件：sensor_node.h
* 作者：
* 说明：通用传感器控制接口程序头文件
* 修改：
* 注释：
*********************************************************************************************/
#ifndef _sensor_node_h_
#define _sensor_node_h_
#include <stdint.h>
#define DEFAULT_SENSOR_TYPE 601

/*sensor-a*/
void sensor_init_a(void);
void sensorLinkOn_a(void);
void sensorUpdate_a(void);
void sensorCheck_a(void);
void sensorControl_a(uint8_t cmd);
int z_process_command_call_a(char* ptag, char* pval, char* obuf) ;
void sensor_Keypressed_a(void);
void sensor_para_init_a(void);
void sensor_cali_a(uint8_t ch, int value);

/*sensor-b*/

/*sensor-c*/


typedef enum {
    SENSOR_A=DEFAULT_SENSOR_TYPE,
    SENSOR_B,
    SENSOR_C,
}t_sensor_type;

typedef struct {
  void (*sensor_init)(void);
  void (*sensorLinkOn)(void);
  void (*sensorUpdate)(void);
  void (*sensorCheck)(void);
  void (*sensorControl)(uint8_t cmd);
  int (*z_process_command_call)(char* ptag, char* pval, char* obuf);
  void (*sensor_Keypressed)(void);
  void (*sensor_para_init)(void);
  void (*sensor_cali)(uint8_t ch, int value);
} t_sensor_fun;

extern t_sensor_fun sensor_fun_p;
void sensor_node_select(void);

#endif
