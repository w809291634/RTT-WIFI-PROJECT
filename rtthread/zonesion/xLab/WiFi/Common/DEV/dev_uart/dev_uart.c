#include "dev_uart.h"

rt_device_t debug_dev = RT_NULL;

void dev_debug_init(unsigned long baud)
{
  uart1_init(baud);
}








