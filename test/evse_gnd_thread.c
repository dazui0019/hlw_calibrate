#include "evse_gnd_thread.h"
#include "evse_gnd.h"
#include <rtthread.h>

#define LOG_TAG              "app.gnd"
#define LOG_LVL             LOG_LVL_DBG
#include <ulog.h>

#define GND_THREAD_STACK_SIZE     1024
#define GND_THREAD_PRIORITY       20

static rt_thread_t gnd_thread = RT_NULL;

static void gnd_thread_entry(void *parameter)
{
    uint16_t adc_value;
    
    /* 配置 GND ADC 通道 */
    adc_gndd_ch_config();
    
    while (1)
    {
        /* 软件触发常规通道转换 */
        adc_software_trigger_enable(ADC0, ADC_ROUTINE_CHANNEL);
        
        /* 等待转换完成 */
        while (!adc_flag_get(ADC0, ADC_FLAG_EOC));
        
        /* 读取转换结果 */
        adc_value = adc_routine_data_read(ADC0);
        
        /* 清除转换完成标志 */
        adc_flag_clear(ADC0, ADC_FLAG_EOC);
        
        /* 使用 ulog 打印结果 */
        LOG_D("GND ADC value: %d", adc_value);
        
        /* 延时1秒 */
        rt_thread_mdelay(1000);
    }
}

int gnd_thread_init(void)
{
    /* 动态创建线程 */
    gnd_thread = rt_thread_create("gnd_test",
                                  gnd_thread_entry,
                                  RT_NULL,
                                  GND_THREAD_STACK_SIZE,
                                  GND_THREAD_PRIORITY,
                                  20);
    
    if (gnd_thread != RT_NULL)
    {
        rt_thread_startup(gnd_thread);
        return RT_EOK;
    }
    
    return -RT_ERROR;
}
// INIT_APP_EXPORT(gnd_thread_init);
