#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "drv_cd4051.h"

#define LOG_TAG "drv.cd4051"
#define LOG_LVL LOG_LVL_DBG
#include "ulog.h"

static struct cd4051_device cd4051_dev = {
    .pin_a = GET_PIN(B, 9),
    .pin_b = GET_PIN(B, 8),
    .pin_c = RT_NULL,
};

// 设置通道
static rt_err_t cd4051_set_channel(struct cd4051_device *dev, uint8_t channel)
{
    rt_uint8_t max_channel;
    
    // 根据实际连接的引脚数计算最大通道数
    if (dev->pin_c != RT_NULL)
        max_channel = 8;
    else if (dev->pin_b != RT_NULL)
        max_channel = 4;
    else
        max_channel = 2;
        
    if (channel >= max_channel)
        return -RT_ERROR;
    
    rt_pin_write(dev->pin_a, channel & 0x01);
    
    if (dev->pin_b != RT_NULL)
        rt_pin_write(dev->pin_b, (channel >> 1) & 0x01);
        
    if (dev->pin_c != RT_NULL)
        rt_pin_write(dev->pin_c, (channel >> 2) & 0x01);
    
    dev->current_channel = channel;
    return RT_EOK;
}

// 设备控制接口
static rt_err_t cd4051_control(rt_device_t dev, int cmd, void *args)
{
    struct cd4051_device *cd4051 = (struct cd4051_device *)dev;
    
    switch (cmd)
    {
        case RT_DEVICE_CTRL_CHANNEL_SELECT:
            return cd4051_set_channel(cd4051, (intptr_t)args);
        
        default:
            return -RT_ERROR;
    }
}

// 设备初始化
static rt_err_t cd4051_init(rt_device_t dev)
{
    struct cd4051_device *cd4051 = (struct cd4051_device *)dev;
    
    RT_ASSERT(cd4051 != RT_NULL);
    RT_ASSERT(cd4051->pin_a != RT_NULL);

    // 处理pin_c != null 但是pin_b==null的情况
    if (cd4051->pin_c != RT_NULL && cd4051->pin_b == RT_NULL){
        log_w("pin_c != null but pin_b == null");
        cd4051->pin_c = RT_NULL;
        log_w("pin_c set to null");
    }

    rt_pin_mode(cd4051->pin_a, PIN_MODE_OUTPUT);
    
    if (cd4051->pin_b != RT_NULL)
        rt_pin_mode(cd4051->pin_b, PIN_MODE_OUTPUT);
        
    if (cd4051->pin_c != RT_NULL)
        rt_pin_mode(cd4051->pin_c, PIN_MODE_OUTPUT);
    
    cd4051_set_channel(cd4051, 0);
    
    return RT_EOK;
}

// 注册CD4051设备
int rt_hw_cd4051_init(void)
{
    int ret = RT_EOK;
    struct cd4051_device *cd4051 = &cd4051_dev;
    
    cd4051->current_channel = 0;
    
    cd4051->parent.type        = RT_Device_Class_Miscellaneous;
    cd4051->parent.init        = cd4051_init;
    cd4051->parent.control     = cd4051_control;
    cd4051->parent.user_data   = RT_NULL;
    
    ret = rt_device_register(&cd4051->parent, CD4051_DEVICE_NAME, RT_DEVICE_FLAG_RDWR);
    
    return ret;
}
INIT_BOARD_EXPORT(rt_hw_cd4051_init);
