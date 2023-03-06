#ifndef __SENSOR_THREAD_H_
#define __SENSOR_THREAD_H_

#define RT_SENSOR_THREAD_STACK_SIZE   1024  /* �߳�ջ��С */
#define RT_SENSOR_THREAD_PRIORITY     3     /* �߳����ȼ� */
#define RT_SENSOR_THREAD_TICK         20    /* ����ͬ���ȼ����̵߳�����ռ�õ�ʱ��Ƭ */

#define RT_SENSOR_CHECK_THREAD_STACK_SIZE   1024  /* �߳�ջ��С */
#define RT_SENSOR_CHECK_THREAD_PRIORITY     2     /* �߳����ȼ� */
#define RT_SENSOR_CHECK_THREAD_TICK         20    /* ����ͬ���ȼ����̵߳�����ռ�õ�ʱ��Ƭ */

extern struct rt_thread sensor_check_thread;

void sensor_thread_init(void);

#endif // __SENSOR_THREAD_H_