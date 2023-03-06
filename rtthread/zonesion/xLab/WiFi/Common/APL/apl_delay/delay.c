#include "delay.h"

void delay_ms(unsigned short times)
{
  rt_thread_mdelay(times);
}

void delay_us(unsigned short times)
{
  rt_hw_udelay(times);
}
