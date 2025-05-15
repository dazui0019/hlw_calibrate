#ifndef APPLICATIONS_UART_TEST_THREAD_H_
#define APPLICATIONS_UART_TEST_THREAD_H_

#include <rtthread.h>
#include <rtdevice.h>

// 测试缓冲区大小
#define UART_TEST_BUFFER_SIZE    128

// 测试函数声明
int uart_dma_test(void);

#endif /* APPLICATIONS_UART_TEST_THREAD_H_ */