#ifndef __RF_THREAD_H_
#define __RF_THREAD_H_

#include "rf_if_app.h"

#define RT_RF_THREAD_STACK_SIZE   1024  /* �߳�ջ��С */
#define RT_RF_THREAD_PRIORITY     6     /* �߳����ȼ� */
#define RT_RF_THREAD_TICK         20    /* ����ͬ���ȼ����̵߳�����ռ�õ�ʱ��Ƭ */

#define RT_RF_LINK_THREAD_STACK_SIZE   2048  /* �߳�ջ��С */
#define RT_RF_LINK_THREAD_PRIORITY     5     /* �߳����ȼ� */
#define RT_RF_LINK_THREAD_TICK         20    /* ����ͬ���ȼ����̵߳�����ռ�õ�ʱ��Ƭ */

extern struct rt_mutex rf_zxbee_mutex;

void rf_thread_init(void);
int16_t rf_send(uint8_t *pdata, uint16_t length, uint16_t timeout_ms);

#endif // __RF_THREAD_H_