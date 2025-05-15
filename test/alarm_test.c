// 测试同时开启多个闹钟的测试用例

#include <rtthread.h>
#include <rtdevice.h>

static rt_alarm_t second_alarm = RT_NULL;
static struct rt_alarm_setup second_setup = {0};

// 闹钟回调函数
static void second_alarm_callback(rt_alarm_t alarm, time_t timestamp)
{
    static uint32_t count = 0;
    count++;
    rt_kprintf("[%d] Second alarm triggered %d times\n", timestamp, count);
}

void second_alarm_test(void)
{
    time_t now;
    struct tm *tm_now;

    // 获取当前时间
    get_timestamp(&now);
    tm_now = gmtime(&now);
    
    // 配置闹钟
    rt_memcpy(&second_setup.wktime, tm_now, sizeof(struct tm));
    second_setup.flag = RT_ALARM_SECOND;  // 每秒触发
    
    // 创建闹钟
    second_alarm = rt_alarm_create(second_alarm_callback, &second_setup);
    if(second_alarm == RT_NULL) {
        rt_kprintf("Create alarm failed!\n");
        return;
    }
    
    // 启动闹钟
    if(rt_alarm_start(second_alarm) != RT_EOK) {
        rt_kprintf("Start alarm failed!\n");
        rt_alarm_delete(second_alarm);
        return;
    }
    
    rt_kprintf("Second alarm started\n");
}
MSH_CMD_EXPORT(second_alarm_test, second alarm test);

// 停止闹钟的命令
void second_alarm_stop(void)
{
    if(second_alarm) {
        rt_alarm_stop(second_alarm);
        rt_alarm_delete(second_alarm);
        second_alarm = RT_NULL;
        rt_kprintf("Second alarm stopped\n");
    }
}
MSH_CMD_EXPORT(second_alarm_stop, stop second alarm test);

static rt_alarm_t minute_alarm = RT_NULL;
static struct rt_alarm_setup minute_setup = {0};

// 分钟闹钟回调函数
static void minute_alarm_callback(rt_alarm_t alarm, time_t timestamp)
{
    static uint32_t count = 0;
    count++;
    rt_kprintf("[%d] Minute alarm triggered %d times\n", timestamp, count);
}

void minute_alarm_test(void)
{
    time_t now;
    struct tm *tm_now;

    // 获取当前时间
    get_timestamp(&now);
    tm_now = gmtime(&now);
    
    // 配置闹钟
    rt_memcpy(&minute_setup.wktime, tm_now, sizeof(struct tm));
    minute_setup.flag = RT_ALARM_MINUTE;  // 每分钟触发
    
    // 创建闹钟
    minute_alarm = rt_alarm_create(minute_alarm_callback, &minute_setup);
    if(minute_alarm == RT_NULL) {
        rt_kprintf("Create minute alarm failed!\n");
        return;
    }
    
    // 启动闹钟
    if(rt_alarm_start(minute_alarm) != RT_EOK) {
        rt_kprintf("Start minute alarm failed!\n");
        rt_alarm_delete(minute_alarm);
        return;
    }
    
    rt_kprintf("Minute alarm started\n");
}
MSH_CMD_EXPORT(minute_alarm_test, minute alarm test);

void minute_alarm_stop(void)
{
    if(minute_alarm) {
        rt_alarm_stop(minute_alarm);
        rt_alarm_delete(minute_alarm);
        minute_alarm = RT_NULL;
        rt_kprintf("Minute alarm stopped\n");
    }
}
MSH_CMD_EXPORT(minute_alarm_stop, stop minute alarm test);

void alarm_test_all(void)
{
    // 启动秒级闹钟
    second_alarm_test();
    
    // 启动分钟级闹钟
    minute_alarm_test();
    
    rt_kprintf("All alarms started\n");
}
MSH_CMD_EXPORT(alarm_test_all, start all alarms test);

void alarm_stop_all(void)
{
    second_alarm_stop();
    minute_alarm_stop();
    rt_kprintf("All alarms stopped\n");
}
MSH_CMD_EXPORT(alarm_stop_all, stop all alarms test);

static rt_alarm_t onemin_alarm = RT_NULL;
static rt_alarm_t twomin_alarm = RT_NULL;
static struct rt_alarm_setup onemin_setup = {0};
static struct rt_alarm_setup twomin_setup = {0};

// 一分钟闹钟回调函数
static void onemin_alarm_callback(rt_alarm_t alarm, time_t timestamp)
{
    rt_kprintf("[%d] One minute alarm triggered!\n", timestamp);
}

// 两分钟闹钟回调函数
static void twomin_alarm_callback(rt_alarm_t alarm, time_t timestamp)
{
    rt_kprintf("[%d] Two minutes alarm triggered!\n", timestamp);
}

void oneshot_alarm_test(void)
{
    time_t now;
    struct tm *tm_now;

    // 获取当前时间
    get_timestamp(&now);
    tm_now = gmtime(&now);
    
    // 配置一分钟闹钟
    rt_memcpy(&onemin_setup.wktime, tm_now, sizeof(struct tm));
    onemin_setup.flag = RT_ALARM_ONESHOT;
    onemin_setup.wktime.tm_min += 1;
    if(onemin_setup.wktime.tm_min >= 60) {
        onemin_setup.wktime.tm_hour += 1;
        onemin_setup.wktime.tm_min %= 60;
    }
    
    // 配置两分钟闹钟
    rt_memcpy(&twomin_setup.wktime, tm_now, sizeof(struct tm));
    twomin_setup.flag = RT_ALARM_ONESHOT;
    twomin_setup.wktime.tm_min += 2;
    if(twomin_setup.wktime.tm_min >= 60) {
        twomin_setup.wktime.tm_hour += 1;
        twomin_setup.wktime.tm_min %= 60;
    }
    
    // 创建闹钟
    onemin_alarm = rt_alarm_create(onemin_alarm_callback, &onemin_setup);
    twomin_alarm = rt_alarm_create(twomin_alarm_callback, &twomin_setup);
    
    if(onemin_alarm == RT_NULL || twomin_alarm == RT_NULL) {
        rt_kprintf("Create alarm failed!\n");
        return;
    }
    
    // 启动闹钟
    if(rt_alarm_start(onemin_alarm) != RT_EOK || rt_alarm_start(twomin_alarm) != RT_EOK) {
        rt_kprintf("Start alarm failed!\n");
        rt_alarm_delete(onemin_alarm);
        rt_alarm_delete(twomin_alarm);
        return;
    }
    
    rt_kprintf("One minute and two minutes oneshot alarms started\n");
}
MSH_CMD_EXPORT(oneshot_alarm_test, oneshot alarm test);

void oneshot_alarm_stop(void)
{
    if(onemin_alarm) {
        rt_alarm_stop(onemin_alarm);
        rt_alarm_delete(onemin_alarm);
        onemin_alarm = RT_NULL;
    }
    if(twomin_alarm) {
        rt_alarm_stop(twomin_alarm);
        rt_alarm_delete(twomin_alarm);
        twomin_alarm = RT_NULL;
    }
    rt_kprintf("Oneshot alarms stopped\n");
}
MSH_CMD_EXPORT(oneshot_alarm_stop, stop oneshot alarm test);
