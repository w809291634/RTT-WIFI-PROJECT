#ifndef __AT_THREAD_H_
#define __AT_THREAD_H_

#include "at_app.h"

#define RT_AT_THREAD_STACK_SIZE   1024  /* 线程栈大小 */
#define RT_AT_THREAD_PRIORITY     3     /* 线程优先级 */
#define RT_AT_THREAD_TICK         20    /* 在相同优先级的线程调度中占用的时间片 */

void at_thread_init(void);

#endif // __AT_THREAD_H_