#include "dev_relay.h"
#include <board.h>

#define DBG_TAG "evse_relay"
#define DBG_LVL LOG_LVL_DBG
#include <ulog.h>

static struct evse_relay_device relay_dev;

static rt_err_t evse_relay_init(rt_device_t dev)
{
    struct evse_relay_device *relay = (struct evse_relay_device *)dev;
    
    rt_pin_mode(relay->pin, PIN_MODE_OUTPUT);
    rt_pin_write(relay->pin, PIN_LOW);
    relay->state = 0;
    
    return RT_EOK;
}

static rt_err_t evse_relay_control(rt_device_t dev, int cmd, void *args)
{
    struct evse_relay_device *relay = (struct evse_relay_device *)dev;
    
    switch (cmd) {
        case RT_DEVICE_CTRL_RLY_OPEN:
            if(relay->state == 1){
                rt_pin_write(relay->pin, PIN_LOW);
                relay->state = 0;
            }
            break;
        case RT_DEVICE_CTRL_RLY_CLOSE:
            if(relay->state == 0){
                rt_pin_write(relay->pin, PIN_HIGH);
                relay->state = 1;
            }
            break;
        default:
            return -RT_ERROR;
    }
    
    return RT_EOK;
}

static rt_ssize_t evse_relay_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    struct evse_relay_device *relay = (struct evse_relay_device *)dev;
    // uint8_t *state = (uint8_t *)buffer;
    
    if (size < sizeof(uint8_t)){
        log_w("size too small: %d", size);
        return 0;
    }
        
    *(uint8_t *)buffer = relay->state;

    return sizeof(uint8_t);
}

int rt_hw_evse_relay_init(void)
{
    rt_device_t device = &relay_dev.parent;
    
    relay_dev.pin = GET_PIN(A, 8);  // 根据实际硬件修改引脚
    
    device->type        = RT_Device_Class_Miscellaneous;
    device->rx_indicate = RT_NULL;
    device->tx_complete = RT_NULL;
    
    device->init       = evse_relay_init;
    device->open       = RT_NULL;
    device->close      = RT_NULL;
    device->read       = evse_relay_read;
    device->write      = RT_NULL;
    device->control    = evse_relay_control;
    
    return rt_device_register(device, "relay", RT_DEVICE_FLAG_RDONLY);
}
INIT_DEVICE_EXPORT(rt_hw_evse_relay_init);
