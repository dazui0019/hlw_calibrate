#include <rtthread.h>
#include <rtdevice.h>
#include "drv_rtt.h"

static rt_ssize_t rtt_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    return SEGGER_RTT_Write(0, buffer, size);
}

static rt_ssize_t rtt_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    return SEGGER_RTT_Read(0, buffer, size);
}

static rt_err_t rtt_control(rt_device_t dev, int cmd, void *args)
{
    return RT_EOK;
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops rtt_ops =
{
    RT_NULL,
    RT_NULL,
    RT_NULL,
    rtt_read,
    rtt_write,
    rtt_control
};
#endif

static struct rt_device rtt_device = {0};
int rt_hw_rtt_init(void)
{
#ifdef RT_USING_DEVICE_OPS
    rtt_device.ops = &rtt_ops;
#else
    rtt_device.read = rtt_read;
    rtt_device.write = rtt_write;
    rtt_device.control = rtt_control;
#endif

    rt_device_register(&rtt_device, "rtt", RT_DEVICE_FLAG_RDWR|RT_DEVICE_FLAG_STREAM);

    return RT_EOK;
}
