#ifndef __AT_THREAD_H_
#define __AT_THREAD_H_

#include "at_app.h"

#define RT_AT_THREAD_STACK_SIZE   1024  /* �߳�ջ��С */
#define RT_AT_THREAD_PRIORITY     3     /* �߳����ȼ� */
#define RT_AT_THREAD_TICK         20    /* ����ͬ���ȼ����̵߳�����ռ�õ�ʱ��Ƭ */

void at_thread_init(void);

#endif // __AT_THREAD_H_