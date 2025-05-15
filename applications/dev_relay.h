#pragma once

#include <rtthread.h>
#include <rtdevice.h>

#define RT_DEVICE_CTRL_RELAY_BASE       (RT_DEVICE_CTRL_BASE(Miscellaneous))
#define RT_DEVICE_CTRL_RLY_OPEN         (RT_DEVICE_CTRL_RELAY_BASE + 1)
#define RT_DEVICE_CTRL_RLY_CLOSE        (RT_DEVICE_CTRL_RELAY_BASE + 2)

struct evse_relay_device {
    struct rt_device parent;
    rt_base_t pin;
    uint8_t state;
};

int rt_hw_evse_relay_init(void);
