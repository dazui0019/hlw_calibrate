#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <drv_spi.h>
#include "SEGGER_RTT.h"
#include "dev_relay.h"
#include "dev_lock.h"
#include "drv_meter.h"
#include <fal.h>

#define LOG_TAG "app.evse_main"
#define LOG_LVL LOG_LVL_DBG
#include <ulog.h>

#include <sfud.h>
#include <dev_spi_flash.h>
#include <dev_spi_flash_sfud.h>

#if defined(BSP_USING_EVENT_RECORDER)
#include "EventRecorder.h"
#endif

static int32_t PparamXK_offset = 1000000;
static int32_t PparamXK_last = 0;

static uint32_t VparamXK_SUM = 0;
static uint32_t AparamXK_SUM = 0;
static uint32_t PparamXK_SUM = 0;

static rt_spi_flash_device_t gd25q32;

static int32_t this_abs(int32_t i)
{
    return (i >= 0 ? i : -i);
}

void evse_main_thread_entry(void *parameter)
{
    rt_device_t meter_dev = RT_NULL;
    rt_device_t hlw_dev = RT_NULL;

    rt_device_t relay_dev = RT_NULL;

    struct meter_device *meter = RT_NULL;
    struct hlw8032_device *hlw = RT_NULL;
    
    struct serial_rx_msg msg = {0};
    rt_uint32_t rx_length = 0;
    static uint8_t rx_buffer[RT_SERIAL_RB_BUFSZ];
    rt_ssize_t result = 0;

    uint32_t calibrate_count = 0;

    /* 初始化继电器 */
    relay_dev = rt_device_find("relay");
    if(relay_dev == RT_NULL){
        log_e("relay device not found!");
        return;
    }
    if(RT_EOK == rt_device_open(relay_dev, RT_DEVICE_FLAG_RDWR)){
        log_d("relay device open success!");
    }else{
        log_e("relay device open failed!");
        return;
    }

    /* 初始化电表 */
    meter_dev = rt_device_find("meter");
    if(meter_dev != RT_NULL){
        rt_device_open(meter_dev, RT_DEVICE_FLAG_RDWR);
    }else{
        log_e("meter device not found!");
        return;
    }
    meter = (struct meter_device *)meter_dev;

    hlw = (struct hlw8032_device *)(meter->hlw_dev);

    /* SFUD initialize */
    gd25q32 = rt_sfud_flash_probe("gd25q32", "spi10");

    fal_init();

    log_d("hlw calibrate start.");

    /* 吸合继电器 */
    rt_device_control(relay_dev, RT_DEVICE_CTRL_RLY_CLOSE, RT_NULL);

    rt_memset(&msg, 0, sizeof(struct serial_rx_msg));
    rt_device_control(meter_dev, METER_CTRL_SWITCH_CHANNEL, (void*)(intptr_t)2);
    for(;;){
        rt_memset(&msg, 0, sizeof(struct serial_rx_msg));
        result = rt_mq_recv(hlw->mq, &msg, sizeof(struct serial_rx_msg), RT_WAITING_FOREVER);
        if (result > 0){
            rx_length = rt_device_read(msg.dev, 0, rx_buffer, msg.size);

            if (rx_length == 24) {
                goto handle;
            }
        }
        continue;   // 数据接收失败 result 不大于0 或 rx_length != 24
    
handle: /* 处理数据 */
        rt_memcpy(meter->temp_buffer, rx_buffer, rx_length);
        if(RT_EOK == rt_device_control(meter_dev, METER_CTRL_CALIBRATE, RT_NULL)){
            if(meter->measure_data[meter->current_channel].PparamXK > 0U){
                if(this_abs((PparamXK_last) - (meter->measure_data[meter->current_channel].PparamXK)) < PparamXK_offset){
                    log_d("VparamXK: %d", meter->measure_data[meter->current_channel].VparamXK);
                    log_d("AparamXK: %d", meter->measure_data[meter->current_channel].AparamXK);
                    log_d("PparamXK: %d", meter->measure_data[meter->current_channel].PparamXK);
                    VparamXK_SUM += meter->measure_data[meter->current_channel].VparamXK;
                    AparamXK_SUM += meter->measure_data[meter->current_channel].AparamXK;
                    PparamXK_SUM += meter->measure_data[meter->current_channel].PparamXK;
                    meter->measure_data[meter->current_channel].PparamXK = 0;
                    calibrate_count++;
                }else{
                    PparamXK_last = meter->measure_data[meter->current_channel].PparamXK;
                    meter->measure_data[meter->current_channel].VparamXK = 0;
                    meter->measure_data[meter->current_channel].AparamXK = 0;
                    meter->measure_data[meter->current_channel].PparamXK = 0;
                    calibrate_count = 0;
                    continue;
                }
            }
        }else{
            log_e("calibrate failed!");
            continue;
        }
    /* 数据处理完毕 */
        // 切换到空闲通道, 等待1000ms
        rt_device_control(meter_dev, METER_CTRL_SWITCH_TO_IDLE, RT_NULL);
        rt_device_read(msg.dev, 0, rx_buffer, RT_SERIAL_RB_BUFSZ);  // 清空接收缓冲区
        rt_mq_control(hlw->mq, RT_IPC_CMD_RESET, RT_NULL);
        if(calibrate_count >= 9){
            log_i("calibrate success!");
            // 切换到下一通道
            // rt_device_control(meter_dev, METER_CTRL_SWITCH_CHANNEL, (void*)(intptr_t)2);
            meter->measure_data[meter->current_channel].VparamXK = VparamXK_SUM / (calibrate_count+1);
            meter->measure_data[meter->current_channel].AparamXK = AparamXK_SUM / (calibrate_count+1);
            meter->measure_data[meter->current_channel].PparamXK = PparamXK_SUM / (calibrate_count+1);
            log_i("calibrate count: %d", (calibrate_count+1));
            log_i("VparamXK: %d", meter->measure_data[meter->current_channel].VparamXK);
            log_i("AparamXK: %d", meter->measure_data[meter->current_channel].AparamXK);
            log_i("PparamXK: %d", meter->measure_data[meter->current_channel].PparamXK);
            for(;;){
                rt_thread_delay(1000);
            }
        }else{
            rt_thread_delay(1000);
        }
        rt_device_control(meter_dev, METER_CTRL_SWITCH_CHANNEL, (void*)(intptr_t)2);
    }
}
