#include <rtthread.h>
#include "sensor_node.h"
#include "rf_if_app.h"

t_sensor_fun sensor_fun_p;          // 选择具体节点

void sensor_node_select(void){
  extern t_rf_info rf_info;
  switch(rf_info.sensor_type){
    case(SENSOR_A):
      sensor_fun_p.sensor_init =sensor_init_a;
      sensor_fun_p.sensorLinkOn =sensorLinkOn_a;
      sensor_fun_p.sensorUpdate =sensorUpdate_a;
      sensor_fun_p.sensorCheck =sensorCheck_a;
      sensor_fun_p.sensorControl =sensorControl_a;
      sensor_fun_p.z_process_command_call =z_process_command_call_a;
      sensor_fun_p.sensor_Keypressed =sensor_Keypressed_a;
      sensor_fun_p.sensor_para_init =sensor_para_init_a;
      sensor_fun_p.sensor_cali =sensor_cali_a;
      break;
      
    case(SENSOR_B):
      break;
      
    case(SENSOR_C):
      break;
      
    default:
      sensor_fun_p.sensor_init =sensor_init_a;
      sensor_fun_p.sensorLinkOn =sensorLinkOn_a;
      sensor_fun_p.sensorUpdate =sensorUpdate_a;
      sensor_fun_p.sensorCheck =sensorCheck_a;
      sensor_fun_p.sensorControl =sensorControl_a;
      sensor_fun_p.z_process_command_call =z_process_command_call_a;
      sensor_fun_p.sensor_Keypressed =sensor_Keypressed_a;
      sensor_fun_p.sensor_para_init =sensor_para_init_a;
      sensor_fun_p.sensor_cali =sensor_cali_a;
      break;
  }
  
}