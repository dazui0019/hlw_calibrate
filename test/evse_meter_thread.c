#include "evse_meter_thread.h"
#include "drv_meter.h"
#include <rtthread.h>
#include <rtdevice.h>

#define LOG_TAG "app.evse_ac_thread"
#define LOG_LVL LOG_LVL_INFO
#include "ulog.h"

static rt_sem_t meter_start_sem = RT_NULL;      // 用于通知meter线程启动

static rt_bool_t is_overvoltage = RT_FALSE;     // 过压状态
static rt_bool_t is_undervoltage = RT_FALSE;    // 欠压状态
static rt_bool_t is_overcurrent = RT_FALSE;     // 全局过流状态

static rt_bool_t is_overcurrent_p[3] = {RT_FALSE, RT_FALSE, RT_FALSE};   // A,B,C相过流状态
static rt_bool_t is_overvoltage_p[3] = {RT_FALSE, RT_FALSE, RT_FALSE};    // A,B,C相过压状态
static rt_bool_t is_undervoltage_p[3] = {RT_FALSE, RT_FALSE, RT_FALSE};   // A,B,C相欠压状态

__IO static uint8_t max_current = 0;

void set_max_current(uint8_t current)
{
    max_current = current;
}

static void meter_thread(void *parameter)
{
    rt_device_t meter_dev = RT_NULL;
    rt_device_t hlw_dev = RT_NULL;

    struct meter_device *meter = RT_NULL;
    struct hlw8032_device *hlw = RT_NULL;
    
    struct serial_rx_msg msg = {0};
    rt_uint32_t rx_length = 0;
    static uint8_t rx_buffer[RT_SERIAL_RB_BUFSZ];
    rt_ssize_t result = 0;
    // 等待EVSE初始化完成
    meter_start_sem = (rt_sem_t)rt_object_find("meter_start", RT_Object_Class_Semaphore);
    if (meter_start_sem == RT_NULL) {
        log_e("meter start semaphore not found!");
        return;
    }
    rt_sem_take(meter_start_sem, RT_WAITING_FOREVER);
    log_i("Start meter thread.");

    meter_dev = rt_device_find("meter");
    if(meter_dev != RT_NULL){
        rt_device_open(meter_dev, RT_DEVICE_FLAG_RDWR);
    }else{
        log_e("meter device not found!");
        return;
    }
    meter = (struct meter_device *)meter_dev;

    hlw = (struct hlw8032_device *)(meter->hlw_dev);

    #ifdef METER_CALIBRATE
    for(;;){
        rt_memset(&msg, 0, sizeof(struct serial_rx_msg));
        result = rt_mq_recv(hlw->mq, &msg, sizeof(struct serial_rx_msg), RT_WAITING_FOREVER);
        rt_device_control(meter_dev, METER_CTRL_SWITCH_CHANNEL, (void*)(intptr_t)2);
        if (result > 0){
            rx_length = rt_device_read(msg.dev, 0, rx_buffer, msg.size);

            if (rx_length == 24) {
                rt_memcpy(meter->temp_buffer, rx_buffer, rx_length);
                if(RT_EOK == rt_device_control(meter_dev, METER_CTRL_CALIBRATE, RT_NULL)){
                    log_d("calibrate success!");
                }else{
                    log_d("calibrate failed!");
                    continue;
                }
            }
        }
    }
    #endif

    for(;;){
        rt_memset(&msg, 0, sizeof(struct serial_rx_msg));
        result = rt_mq_recv(hlw->mq, &msg, sizeof(struct serial_rx_msg), RT_WAITING_FOREVER);
        if (result > 0){
            rx_length = rt_device_read(msg.dev, 0, rx_buffer, msg.size);

            if (rx_length == 24) {
                rt_memcpy(meter->temp_buffer, rx_buffer, rx_length);
                if(RT_EOK == rt_device_control(meter_dev, METER_CTRL_PARSE_DATA, RT_NULL)){
                    log_d("current channel: %d", meter->current_channel);
                    log_d("voltage: %0.2f", meter->measure_data[meter->current_channel].voltage);
                }else{
                    log_d("parse data failed!");
                    continue;   // 如果解析数据失败, 那么直接进入下一次循环, 等待下一次数据到来
                }
                goto complete; // 处理完毕
            }
        }
        continue;   // 数据接收失败

/* 数据处理完毕 */
complete:
        // 上报数据
        switch (meter->current_channel)
        {
        case 0:
            UPDATE_phase_c(meter->measure_data[meter->current_channel].voltage, meter->measure_data[meter->current_channel].current);
            break;
        case 1:
            UPDATE_phase_b(meter->measure_data[meter->current_channel].voltage, meter->measure_data[meter->current_channel].current);
            break;
        case 2:
            UPDATE_phase_a(meter->measure_data[meter->current_channel].voltage, meter->measure_data[meter->current_channel].current);
            break;
        default:
            break;
        }
        // 判断过压
        if (meter->measure_data[meter->current_channel].voltage > EVSE_OVERVOLTAGE_THRESHOLD) {
            if (is_overvoltage_p[meter->current_channel] == RT_FALSE) {
                is_overvoltage_p[meter->current_channel] = RT_TRUE;
                is_overvoltage = RT_TRUE;  // 设置全局过压标志
                log_w("Phase %c overvoltage: %.1fV", 'A' + meter->current_channel, meter->measure_data[meter->current_channel].voltage);
                evse_fault_set_flag(FAULT_OVER_VOL);
            }
        } else {
            if (is_overvoltage_p[meter->current_channel] == RT_TRUE) {
                is_overvoltage_p[meter->current_channel] = RT_FALSE;
                // 检查其他相是否都正常
                rt_bool_t all_normal = RT_TRUE;
                for(int i = 0; i < 3; i++) {
                    if(is_overvoltage_p[i] == RT_TRUE) {
                        all_normal = RT_FALSE;
                        break;
                    }
                }
                if(all_normal) {
                    is_overvoltage = RT_FALSE;
                    evse_fault_clear_flag(FAULT_OVER_VOL);
                }
                log_i("Phase %c voltage normal(overvoltage recovered): %.1fV", 'A' + meter->current_channel, meter->measure_data[meter->current_channel].voltage);
            }
        }

        // 判断欠压
        if (meter->measure_data[meter->current_channel].voltage < EVSE_UNDERVOLTAGE_THRESHOLD) {
            if (is_undervoltage_p[meter->current_channel] == RT_FALSE) {
                is_undervoltage_p[meter->current_channel] = RT_TRUE;
                is_undervoltage = RT_TRUE;  // 设置全局欠压标志
                log_w("Phase %c undervoltage: %.1fV", 'A' + meter->current_channel, meter->measure_data[meter->current_channel].voltage);
                evse_fault_set_flag(FAULT_UNDER_VOL);
            }
        } else {
            if (is_undervoltage_p[meter->current_channel] == RT_TRUE) {
                is_undervoltage_p[meter->current_channel] = RT_FALSE;
                // 检查其他相是否都正常
                rt_bool_t all_normal = RT_TRUE;
                for(int i = 0; i < 3; i++) {
                    if(is_undervoltage_p[i] == RT_TRUE) {
                        all_normal = RT_FALSE;
                        break;
                    }
                }
                if(all_normal) {
                    is_undervoltage = RT_FALSE;
                    evse_fault_clear_flag(FAULT_UNDER_VOL);
                }
                log_i("Phase %c voltage normal(undervoltage recovered): %.1fV", 'A' + meter->current_channel, meter->measure_data[meter->current_channel].voltage);
            }
        }
        
        // 判断过流
        if (100*(meter->measure_data[meter->current_channel].current) > max_current*EVSE_OVERCURRENT_PERCENT){
            if (is_overcurrent_p[meter->current_channel] == RT_FALSE){
                is_overcurrent_p[meter->current_channel] = RT_TRUE;
                is_overcurrent = RT_TRUE;
                evse_fault_set_flag(FAULT_OVER_CUR);
                log_e("Phase %c overcurrent: %.2f A", 'A' + meter->current_channel, meter->measure_data[meter->current_channel].current);
            }
        }
#ifdef EVSE_OVERCURRENT_AUTO_RECOVERY   // 清除过流状态
        else if (100*(meter->measure_data[meter->current_channel].current) < max_current*EVSE_OVERCURRENT_RECOVERY_PERCENT){
            if (is_overcurrent_p[meter->current_channel] == RT_TRUE){
                is_overcurrent_p[meter->current_channel] = RT_FALSE;
                // 检查其他相是否都正常
                rt_bool_t all_normal = RT_TRUE;
                for(int i = 0; i < 3; i++) {
                    if(is_overcurrent_p[i] == RT_TRUE) {
                        all_normal = RT_FALSE;
                        break;
                    }
                }
                if(all_normal) {
                    is_overcurrent = RT_FALSE;
                }
                log_i("Phase %c current normal(overcurrent recovered): %.2f A", 'A' + meter->current_channel, meter->measure_data[meter->current_channel].current);
            }
        }
#endif
        
        // 切换到空闲通道, 等待1000ms
        rt_device_control(meter_dev, METER_CTRL_SWITCH_TO_IDLE, RT_NULL);
        rt_device_read(msg.dev, 0, rx_buffer, RT_SERIAL_RB_BUFSZ);  // 清空接收缓冲区
        rt_mq_control(hlw->mq, RT_IPC_CMD_RESET, RT_NULL);
        rt_thread_delay(1000);
        // 切换到下一通道
        rt_device_control(meter_dev, METER_CTRL_SWITCH_CHANNEL, (void*)(intptr_t)2);
    }
}

int evse_meter_init(void)
{
    // 创建HLW8032处理线程
    rt_thread_t thread = rt_thread_create("meter_thread",
                                        meter_thread,
                                        RT_NULL,
                                        1024,
                                        RT_THREAD_PRIORITY_MAX-1,
                                        10);
    if (thread != RT_NULL) {
        rt_thread_startup(thread);
    }

    return RT_EOK;
}
INIT_APP_EXPORT(evse_meter_init);
