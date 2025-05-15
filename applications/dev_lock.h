#ifndef __DEV_LOCK_H__
#define __DEV_LOCK_H__

#include <rtthread.h>
#include <rtdevice.h>

/* Lock device control commands */
#define RT_DEVICE_CTRL_LOCK_LOCK      0x31    /* Lock the gun */
#define RT_DEVICE_CTRL_LOCK_UNLOCK    0x32    /* Unlock the gun */
#define RT_DEVICE_CTRL_LOCK_SET_CB    0x33    /* Set lock callback function */

/* lock state */
#define LOCK_UNLOCKED  0
#define LOCK_LOCKED    1

/* Lock device structure */
struct lock_device {
    struct rt_device parent;                /* RT-Thread device parent */
    rt_base_t ctrl_pin;                     /* Lock control pin */
    rt_base_t fb_pin;                       /* Lock feedback pin */
    rt_bool_t state;                        /* Current lock state: 0-unlocked, 1-locked */
    void (*lock_callback)(void*);           /* Lock callback function */
};

/* Lock device interface functions */
int rt_hw_evse_lock_init(void);

#endif /* __DEV_LOCK_H__ */