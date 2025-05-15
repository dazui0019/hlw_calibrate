#include <rtthread.h>
#include <rtdevice.h>
#include "dev_lock.h"

#define DBG_TAG "lock_test"
#define DBG_LVL LOG_LVL_DBG
#include <ulog.h>

#define THREAD_PRIORITY         25
#define THREAD_STACK_SIZE       512
#define THREAD_TIMESLICE        5

static rt_thread_t lock_test_thread = RT_NULL;
static rt_device_t lock_dev = RT_NULL;

static void lock_test_entry(void *parameter)
{
    rt_err_t result;
    uint8_t lock_state;
    
    /* Find and open lock device */
    lock_dev = rt_device_find("lock");
    if (lock_dev == RT_NULL) {
        log_e("Cannot find lock device!");
        return;
    }

    // open lock device
    result = rt_device_open(lock_dev, RT_DEVICE_FLAG_RDONLY);
    if (result!= RT_EOK) {
        log_e("Failed to open lock device: %d", result);
        return;
    }
    
    while (1) {
        /* Test lock operation */
        log_i("Testing lock operation...");
        result = rt_device_control(lock_dev, RT_DEVICE_CTRL_LOCK_LOCK, RT_NULL);
        if (result != RT_EOK) {
            log_e("Failed to lock: %d", result);
        }
        
        /* Read lock state */
        result = rt_device_read(lock_dev, 0, &lock_state, sizeof(uint8_t));
        if (result != sizeof(uint8_t)) {
            log_e("Failed to read lock state");
        } else {
            log_i("Lock state after locking: %d", lock_state);
        }
        
        /* Wait for 3 seconds */
        rt_thread_mdelay(3000);
        
        /* Test unlock operation */
        log_i("Testing unlock operation...");
        result = rt_device_control(lock_dev, RT_DEVICE_CTRL_LOCK_UNLOCK, RT_NULL);
        if (result != RT_EOK) {
            log_e("Failed to unlock: %d", result);
        }
        
        /* Read lock state again */
        result = rt_device_read(lock_dev, 0, &lock_state, sizeof(uint8_t));
        if (result != sizeof(uint8_t)) {
            log_e("Failed to read lock state: %d", result);
        } else {
            log_i("Lock state after unlocking: %d", lock_state);
        }
        
        /* Wait for 3 seconds before next test cycle */
        rt_thread_mdelay(3000);
    }
}

int lock_test_init(void)
{
    /* Create lock test thread */
    lock_test_thread = rt_thread_create("lock_test",
                                      lock_test_entry,
                                      RT_NULL,
                                      THREAD_STACK_SIZE,
                                      THREAD_PRIORITY,
                                      THREAD_TIMESLICE);
    
    if (lock_test_thread != RT_NULL) {
        rt_thread_startup(lock_test_thread);
    } else {
        log_e("Failed to create lock test thread");
        return -RT_ERROR;
    }
    
    return RT_EOK;
}
MSH_CMD_EXPORT(lock_test_init, start lock test);
