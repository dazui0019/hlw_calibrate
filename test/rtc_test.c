#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "sys/time.h"

#define DBG_TAG              "rtc.test"
#define DBG_LVL              LOG_LVL_DBG
#include <ulog.h>

#define RTC_THREAD_STACK_SIZE    1024
#define RTC_THREAD_PRIORITY      20
#define RTC_THREAD_TIMESLICE     10

static rt_thread_t rtc_thread = RT_NULL;
static rt_device_t rtc_dev = RT_NULL;

static void rtc_show_time(void)
{
    time_t now = 0;
    rt_err_t ret = RT_EOK;
    rtc_parameter_struct rtc_time;
    struct tm tm_time = {0};
    
    /* 获取时间戳 */
    get_timestamp(&now);
    localtime_r(&now, &tm_time);

    /* 打印时间 */
    LOG_I("Time: %04d-%02d-%02d %02d:%02d:%02d",
            tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
            tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
}

/* RTC显示线程入口函数 */
static void rtc_thread_entry(void *parameter)
{
    rt_err_t ret = RT_EOK;

    /* 查找RTC设备 */
    rtc_dev = rt_device_find("rtc");
    RT_ASSERT(rtc_dev != RT_NULL);

    /* 打开RTC设备 */
    ret = rt_device_open(rtc_dev, RT_DEVICE_FLAG_RDWR);
    while (1)
    {
        rtc_show_time();
        rt_thread_mdelay(1000);  /* 每秒更新一次 */
    }
}

/* RTC测试任务初始化 */
static int rtc_test_init(void)
{
    /* 创建RTC显示线程 */
    rtc_thread = rt_thread_create("rtc_test",
                                  rtc_thread_entry,
                                  RT_NULL,
                                  RTC_THREAD_STACK_SIZE,
                                  RTC_THREAD_PRIORITY,
                                  RTC_THREAD_TIMESLICE);
    
    if (rtc_thread != RT_NULL)
    {
        rt_thread_startup(rtc_thread);
        LOG_I("rtc test thread started.");
        return RT_EOK;
    }
    
    LOG_E("create rtc test thread failed!");
    return -RT_ERROR;
}
INIT_APP_EXPORT(rtc_test_init);