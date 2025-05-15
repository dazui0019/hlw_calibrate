#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "evse_main.h"
#include "evse_main_thread.h"

#define LOG_TAG "app.evse_main"
#define LOG_LVL LOG_LVL_DBG
#include <ulog.h>

int main(void)
{
    rt_kprintf("\r\n");
    log_d("Hello, World!");

    rt_thread_t tid;
    tid = rt_thread_create( "main",
                            evse_main_thread_entry, RT_NULL,
                            1024,
                            RT_THREAD_PRIORITY_MAX-1, 20);
    if (tid != RT_NULL){
        rt_thread_startup(tid);
    }else{
        log_e("create main thread failed!");
    }
    return RT_EOK;
}
