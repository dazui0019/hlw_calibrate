#include "uart_test_thread.h"

#define UART0_NAME    "uart0"
#define UART1_NAME    "uart1"

static uint8_t uart0_rx_buffer[UART_TEST_BUFFER_SIZE];
static uint8_t uart1_rx_buffer[UART_TEST_BUFFER_SIZE];

// UART0接收完成回调
static rt_err_t uart0_rx_ind(rt_device_t dev, rt_size_t size)
{
    rt_kprintf("UART0 received %d bytes: ", size);
    
    rt_device_read(dev, 0, uart0_rx_buffer, UART_TEST_BUFFER_SIZE);

    for(rt_size_t i = 0; i < size; i++)
    {
        rt_kprintf("%02X ", uart0_rx_buffer[i]);
    }
    rt_kprintf("\r\n");
    return RT_EOK;
}

// UART1接收完成回调
static rt_err_t uart1_rx_ind(rt_device_t dev, rt_size_t size)
{
    rt_kprintf("UART1 received %d bytes: ", size);
    
    // 继续接收下一包数据
    rt_device_read(dev, 0, uart1_rx_buffer, UART_TEST_BUFFER_SIZE);

    for(rt_size_t i = 0; i < size; i++)
    {
        rt_kprintf("%02X ", uart1_rx_buffer[i]);
    }
    rt_kprintf("\r\n");
    return RT_EOK;
}

int uart_dma_test(void)
{
    rt_device_t uart0_dev, uart1_dev;
    rt_kprintf("\r\n");
    // 打开串口1设备
    uart0_dev = rt_device_find(UART0_NAME);
    if (uart0_dev == RT_NULL)
    {
        rt_kprintf("Cannot find %s device!\r\n", UART1_NAME);
        return RT_ERROR;
    }
    
    // 打开串口2设备
    uart1_dev = rt_device_find(UART1_NAME);
    if (uart1_dev == RT_NULL)
    {
        rt_kprintf("Cannot find %s device!\r\n", UART1_NAME);
        return RT_ERROR;
    }
    
    // 以DMA方式打开设备
    rt_device_open(uart0_dev, RT_DEVICE_FLAG_DMA_RX);
    rt_device_open(uart1_dev, RT_DEVICE_FLAG_DMA_RX);
    
    // 设置接收回调
    rt_device_set_rx_indicate(uart0_dev, uart0_rx_ind);
    rt_device_set_rx_indicate(uart1_dev, uart1_rx_ind);
    
    // 启动接收
    // rt_device_read(uart1_dev, 0, uart1_rx_buffer, UART_TEST_BUFFER_SIZE);
    // rt_device_read(uart2_dev, 0, uart2_rx_buffer, UART_TEST_BUFFER_SIZE);
    
    rt_kprintf("UART DMA receive test started...\r\n");
    rt_kprintf("Please send data to UART0 and UART1\r\n");

    return RT_EOK;
}
INIT_APP_EXPORT(uart_dma_test);
