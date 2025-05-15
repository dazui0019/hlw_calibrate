#include "dev_lock.h"
#include <board.h>

#define DBG_TAG "evse_lock"
#define DBG_LVL LOG_LVL_INFO
#include <ulog.h>

static struct lock_device lock_dev;

static void evse_lock_irq(void *args)
{
    struct lock_device *lock = (struct lock_device *)args;
    rt_ssize_t pin_state = rt_pin_read(lock->fb_pin);
    // lock->state = !(rt_pin_read(lock->fb_pin));
    
    if(pin_state == PIN_HIGH) {
        lock->state = LOCK_UNLOCKED; 
    }else if(pin_state == PIN_LOW) {
        lock->state = LOCK_LOCKED; 
    }

    if(lock->lock_callback != RT_NULL) {
        lock->lock_callback(lock);
    }
}

static rt_err_t evse_lock_init(rt_device_t dev)
{
    struct lock_device *lock = (struct lock_device *)dev;
    
    rt_pin_mode(lock->ctrl_pin, PIN_MODE_OUTPUT);
    rt_pin_write(lock->ctrl_pin, PIN_LOW);

    rt_pin_mode(lock->fb_pin, PIN_MODE_INPUT);
    rt_pin_attach_irq(lock->fb_pin, PIN_IRQ_MODE_RISING_FALLING, evse_lock_irq, (void*)lock);
    rt_pin_irq_enable(lock->fb_pin, PIN_IRQ_ENABLE);
    lock->state = 0;
    lock->lock_callback = RT_NULL;
    
    return RT_EOK;
}

static rt_err_t evse_lock_control(rt_device_t dev, int cmd, void *args)
{
    struct lock_device *lock = (struct lock_device *)dev;
    
    switch (cmd) {
        case RT_DEVICE_CTRL_LOCK_UNLOCK:
            rt_pin_write(lock->ctrl_pin, PIN_LOW);
            log_d("Gun unlocked");
            break;
        case RT_DEVICE_CTRL_LOCK_LOCK:
            rt_pin_write(lock->ctrl_pin, PIN_HIGH);
            log_d("Gun locked");
            break;
        case RT_DEVICE_CTRL_LOCK_SET_CB:
            if(args) {
                lock->lock_callback = (void (*)(void*))args;
                log_d("Lock callback set");
            } else {
                lock->lock_callback = RT_NULL;
                log_d("Lock callback cleared");
            }
        break;
        default:
            return -RT_ERROR;
    }
    
    return RT_EOK;
}

static rt_ssize_t evse_lock_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    struct lock_device *lock = (struct lock_device *)dev;
    rt_ssize_t ret = 0;

    if (size < sizeof(rt_bool_t)) {
        log_w("Write buffer too small: %d", size);
        return 0;
    }

    lock->state = *(rt_bool_t*)buffer;

    if(lock->lock_callback != RT_NULL) {
        lock->lock_callback(lock);
    }

    ret = sizeof(rt_bool_t);

    return ret;
}

static rt_ssize_t evse_lock_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    struct lock_device *lock = (struct lock_device *)dev;
    
    if (size < sizeof(uint8_t)){
        log_w("size too small: %d", size);
        return 0;
    }
        
    *(uint8_t *)buffer = lock->state;

    return sizeof(uint8_t);
}

int rt_hw_evse_lock_init(void)
{
    rt_device_t device = &lock_dev.parent;
    
    lock_dev.ctrl_pin = GET_PIN(C, 9);  // Configure according to actual hardware
    lock_dev.fb_pin = GET_PIN(D, 15);    // Configure according to actual hardware
    
    device->type        = RT_Device_Class_Miscellaneous;
    device->rx_indicate = RT_NULL;
    device->tx_complete = RT_NULL;
    
    device->init    = evse_lock_init;
    device->open    = RT_NULL;
    device->close   = RT_NULL;
    device->read    = evse_lock_read;
    device->write   = evse_lock_write;
    device->control = evse_lock_control;
    
    return rt_device_register(device, "lock", RT_DEVICE_FLAG_RDONLY);
}
INIT_DEVICE_EXPORT(rt_hw_evse_lock_init);