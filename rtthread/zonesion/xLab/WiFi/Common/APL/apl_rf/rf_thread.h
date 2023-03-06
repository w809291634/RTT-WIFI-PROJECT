#ifndef __RF_THREAD_H_
#define __RF_THREAD_H_

#include "rf_if_app.h"

#define RT_RF_THREAD_STACK_SIZE   1024  /* 线程栈大小 */
#define RT_RF_THREAD_PRIORITY     6     /* 线程优先级 */
#define RT_RF_THREAD_TICK         20    /* 在相同优先级的线程调度中占用的时间片 */

#define RT_RF_LINK_THREAD_STACK_SIZE   2048  /* 线程栈大小 */
#define RT_RF_LINK_THREAD_PRIORITY     5     /* 线程优先级 */
#define RT_RF_LINK_THREAD_TICK         20    /* 在相同优先级的线程调度中占用的时间片 */

extern struct rt_mutex rf_zxbee_mutex;

void rf_thread_init(void);
int16_t rf_send(uint8_t *pdata, uint16_t length, uint16_t timeout_ms);

#endif // __RF_THREAD_H_