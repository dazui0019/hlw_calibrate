#include <rtthread.h>
#include <rtdevice.h>
#include <sys/time.h>

#define DBG_TAG "RTC_TEST"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

static void alarm_callback(rt_alarm_t alarm, time_t timestamp)
{
    struct tm *p_tm;
    
    p_tm = gmtime(&timestamp);
    LOG_I("Alarm triggered at %04d-%02d-%02d %02d:%02d:%02d",
          p_tm->tm_year + 1900, p_tm->tm_mon + 1, p_tm->tm_mday,
          p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec);
}

static void rtc_alarm_test(void)
{
    rt_alarm_t alarm = RT_NULL;
    struct rt_alarm_setup setup;
    struct tm now = {0};
    time_t now_time = 0;
    
    // 获取当前时间
    get_timestamp(&now_time);
    gmtime_r(&now_time, &now);
    
    LOG_I("Current time: %04d-%02d-%02d %02d:%02d:%02d",
            now.tm_year + 1900, now.tm_mon + 1, now.tm_mday,
            now.tm_hour, now.tm_min, now.tm_sec);
    
    // 设置5秒后的alarm
    setup.flag = RT_ALARM_ONESHOT;  // 一次性闹钟
    setup.wktime = now;
    setup.wktime.tm_sec += 5;       // 5秒后触发
    
    // 创建alarm
    alarm = rt_alarm_create(alarm_callback, &setup);
    if (alarm == RT_NULL) {
        LOG_E("Failed to create alarm!");
        return;
    }
    
    LOG_I("Alarm will trigger at %04d-%02d-%02d %02d:%02d:%02d",
          setup.wktime.tm_year + 1900, setup.wktime.tm_mon + 1, setup.wktime.tm_mday,
          setup.wktime.tm_hour, setup.wktime.tm_min, setup.wktime.tm_sec);
    
    // 启动alarm
    if (rt_alarm_start(alarm) != RT_EOK) {
        LOG_E("Failed to start alarm!");
        rt_alarm_delete(alarm);
        return;
    }
    
    // 等待alarm触发
    rt_thread_mdelay(10000);
    
    // 删除alarm
    rt_alarm_delete(alarm);
    LOG_I("Alarm test finished");
}

// 导出到msh命令
MSH_CMD_EXPORT(rtc_alarm_test, RTC alarm test);
