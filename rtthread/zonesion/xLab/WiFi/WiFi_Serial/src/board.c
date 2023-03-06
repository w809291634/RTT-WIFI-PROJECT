#include "board.h"
#include "dev_uart/usart.h"

#ifdef RT_USING_HEAP
#define HEAP_BUF_SIZE     1000
static rt_uint32_t heapBuf[HEAP_BUF_SIZE];

RT_WEAK void *heap_begin(void)
{
  return heapBuf;
}

RT_WEAK void *heap_end(void)
{
  return heapBuf + HEAP_BUF_SIZE;
}
#endif // RT_USING_HEAP

/*******************************************************************************
* Function Name  : NVIC_Configuration
* Description    : Configures Vector Table base location.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void NVIC_Configuration(void)
{
#ifdef  VECT_TAB_RAM
	/* Set the Vector Table base location at 0x20000000 */
	NVIC_SetVectorTable(NVIC_VectTab_RAM, 0x0);
#else  /* VECT_TAB_FLASH  */
	/* Set the Vector Table base location at 0x08000000 */
	NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0);
#endif
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
}

/*******************************************************************************
 * Function Name  : SysTick_Configuration
 * Description    : Configures the SysTick for OS tick.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void SysTick_Configuration(void)
{
	RCC_ClocksTypeDef  rcc_clocks;
	rt_uint32_t         cnts;

	RCC_GetClocksFreq(&rcc_clocks);

	cnts = (rt_uint32_t)rcc_clocks.HCLK_Frequency / RT_TICK_PER_SECOND;

	SysTick_Config(cnts);
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);
}

/**
 * This is the timer interrupt service routine.
 *
 */
void SysTick_Handler(void)
{
	/* enter interrupt */
	rt_interrupt_enter();

	rt_tick_increase();
  
	/* leave interrupt */
	rt_interrupt_leave();
}

void rt_hw_udelay(rt_uint32_t us)
{
  rt_uint32_t start, now, delta, reload, us_tick;
  start = SysTick->VAL;
  reload = SysTick->LOAD;
  us_tick = SystemCoreClock / 1000000UL;//比如系统时钟频率是80M 那么1us 需要80个周期
  do {
    now = SysTick->VAL;
    delta = start > now ? start - now : reload + start - now;//是倒数 还是正数
  } while(delta < us_tick * us);
}

/**
 * This function will initial STM32 board.
 */
void rt_hw_board_init()
{
	/* NVIC Configuration */
	NVIC_Configuration();

	/* Configure the SysTick */
	SysTick_Configuration();

#ifdef RT_USING_HEAP
  rt_system_heap_init(heap_begin(), heap_end());
#endif  
  
#ifdef RT_USING_COMPONENTS_INIT
  rt_components_board_init();
#endif

#ifdef RT_USING_CONSOLE
  rt_hw_usart_init();
  rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
#endif
}


