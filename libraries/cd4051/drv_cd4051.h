#pragma once

// CD4051设备结构体(重新封装父设备，增加自己的成员)
struct cd4051_device
{
    struct rt_device parent;    // RT-Thread 设备基类
    rt_base_t pin_a;            // A必须连接
    rt_base_t pin_b;            // 可选,不用时设为RT_NULL
    rt_base_t pin_c;            // 可选,不用时设为RT_NULL
    rt_uint8_t current_channel;
};

#define CD4051_DEVICE_NAME    "cd4051"

// 建议使用以下格式定义自己的控制命令
#define RT_DEVICE_CTRL_CD4051_BASE      (RT_DEVICE_CTRL_BASE(Miscellaneous))
#define RT_DEVICE_CTRL_CHANNEL_SELECT   (RT_DEVICE_CTRL_CD4051_BASE + 1)
