#include "evse_ac_thread.h"
#include "drv_hlw8032.h"
#include "drv_cd4051.h"
#include <rtthread.h>
#include <rtdevice.h>

#define LOG_TAG "app.evse_ac_thread"
#define LOG_LVL LOG_LVL_DBG
#include "ulog.h"

static void hlw8032_process_thread(void *parameter)
{
    rt_device_t hlw_dev = RT_NULL;
    rt_device_t cd4051 = RT_NULL;

    struct hlw8032_device *hlw = RT_NULL;
    struct serial_rx_msg msg;
    rt_uint32_t rx_length;
    static uint8_t rx_buffer[RT_SERIAL_RB_BUFSZ];
    static uint8_t temp_buffer[24];
    static rt_uint32_t accumulated_length = 0;
    rt_ssize_t result;
    void *args[] = {temp_buffer, &hlw->measure_data};

    cd4051 = rt_device_find("cd4051");
    if (cd4051 != RT_NULL){
        rt_device_open(cd4051, RT_DEVICE_FLAG_RDWR);
    }else{
        log_e("cd4051 device not found!");
        return;
    }

    hlw_dev = rt_device_find("hlw8032");
    if(hlw_dev != RT_NULL){
        rt_device_open(hlw_dev, RT_DEVICE_FLAG_RDWR);
    }else{
        log_e("hlw8032 device not found!");
        return;
    }
    hlw = (struct hlw8032_device *)hlw_dev;

    rt_device_control(cd4051, RT_DEVICE_CTRL_CHANNEL_SELECT, (void*)((intptr_t)2));
    while (1)
    {
        rt_memset(&msg, 0, sizeof(struct serial_rx_msg));
        result = rt_mq_recv(hlw->mq, &msg, sizeof(struct serial_rx_msg), RT_WAITING_FOREVER);
        if (result > 0)
        {
            rx_length = rt_device_read(msg.dev, 0, rx_buffer, msg.size);
            
            if (rx_length == 24) {
                if(RT_EOK == rt_device_control(hlw_dev, HLW8032_CTRL_PARSE_DATA, args)){
                    // todo: 切换到下一个通道
                }else{
                    log_d("parse data failed!");
                }
                accumulated_length = 0;
                continue;
            }

            if (accumulated_length == 0) {
                rt_memcpy(temp_buffer, rx_buffer, rx_length);
                accumulated_length = rx_length;
            }else if (accumulated_length + rx_length == 24) {
                rt_memcpy(temp_buffer + accumulated_length, rx_buffer, rx_length);
                if(RT_EOK == rt_device_control(hlw_dev, HLW8032_CTRL_PARSE_DATA, args)){
                    // todo: 切换到下一个通道
                }else{
                    log_d("parse data failed!");
                }
                accumulated_length = 0;
            }else {
                rt_memcpy(temp_buffer, rx_buffer, rx_length);
                accumulated_length = rx_length;
            }
        }
    }
}

int evse_ac_init(void)
{
    // 创建HLW8032处理线程
    rt_thread_t thread = rt_thread_create("hlw_thread",
                                        hlw8032_process_thread,
                                        RT_NULL,
                                        1024,
                                        25,
                                        10);
    if (thread != RT_NULL) {
        rt_thread_startup(thread);
    }

    return RT_EOK;
}
// INIT_APP_EXPORT(evse_ac_init);
