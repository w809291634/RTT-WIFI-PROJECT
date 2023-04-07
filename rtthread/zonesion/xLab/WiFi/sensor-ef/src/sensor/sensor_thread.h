#ifndef __SENSOR_THREAD_H_
#define __SENSOR_THREAD_H_

#define RT_SENSOR_THREAD_STACK_SIZE   1024  /* 线程栈大小 */
#define RT_SENSOR_THREAD_PRIORITY     3     /* 线程优先级 */
#define RT_SENSOR_THREAD_TICK         20    /* 在相同优先级的线程调度中占用的时间片 */

#define RT_SENSOR_CHECK_THREAD_STACK_SIZE   1024  /* 线程栈大小 */
#define RT_SENSOR_CHECK_THREAD_PRIORITY     2     /* 线程优先级 */
#define RT_SENSOR_CHECK_THREAD_TICK         20    /* 在相同优先级的线程调度中占用的时间片 */

extern struct rt_thread sensor_check_thread;

void sensor_thread_init(void);

#endif // __SENSOR_THREAD_H_