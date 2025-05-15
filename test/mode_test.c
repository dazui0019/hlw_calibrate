#include <rtthread.h>
#include <rtdevice.h>
#include <stdlib.h>
#include "dev_evse.h"

// 测试延时充电模式
static void test_delay_mode(int argc, char *argv[])
{
    rt_device_t evse_dev;
    struct charge_mode_config config = {0};
    
    if(argc != 2) {
        rt_kprintf("Usage: test_delay_mode [hours]\n");
        return;
    }
    
    evse_dev = rt_device_find("evse");
    if(evse_dev == RT_NULL) {
        rt_kprintf("Cannot find evse device!\n");
        return;
    }
    
    config.mode = CHARGE_MODE_DELAY;
    config.timer.delay_hours = atoi(argv[1]);
    
    if(rt_device_write(evse_dev, EVSE_RW_CHARGE_MODE, &config, sizeof(config)) != sizeof(config)) {
        rt_kprintf("Set delay mode failed!\n");
        return;
    }
    rt_kprintf("Set to delay mode, delay %d hours\n", config.timer.delay_hours);
}
MSH_CMD_EXPORT(test_delay_mode, test delay charging mode [hours]);

// 测试预约充电模式
static void test_schedule_mode(int argc, char *argv[])
{
    rt_device_t evse_dev;
    struct charge_mode_config config = {0};
    
    if(argc != 3) {
        rt_kprintf("Usage: test_schedule_mode [start_hour] [stop_hour]\n");
        rt_kprintf("Example: test_schedule_mode 22 6 (22:00-06:00)\n");
        return;
    }
    
    evse_dev = rt_device_find("evse");
    if(evse_dev == RT_NULL) {
        rt_kprintf("Cannot find evse device!\n");
        return;
    }
    
    config.mode = CHARGE_MODE_SCHEDULE;
    config.timer.schedule_time[0] = atoi(argv[1]);  // 开始时间
    config.timer.schedule_time[1] = atoi(argv[2]);  // 结束时间
    
    if(config.timer.schedule_time[0] >= 24 || config.timer.schedule_time[1] >= 24) {
        rt_kprintf("Invalid time! Hour must be 0-23\n");
        return;
    }
    
    if(rt_device_write(evse_dev, EVSE_RW_CHARGE_MODE, &config, sizeof(config)) != sizeof(config)) {
        rt_kprintf("Set schedule mode failed!\n");
        return;
    }
    rt_kprintf("Set to schedule mode, time %02d:00-%02d:00\n", 
        config.timer.schedule_time[0], 
        config.timer.schedule_time[1]);
}
MSH_CMD_EXPORT(test_schedule_mode, test schedule charging mode [start_hour] [stop_hour]);

// 测试普通充电模式
static void test_normal_mode(void)
{
    rt_device_t evse_dev;
    struct charge_mode_config config = {0};
    
    evse_dev = rt_device_find("evse");
    if(evse_dev == RT_NULL) {
        rt_kprintf("Cannot find evse device!\n");
        return;
    }
    
    config.mode = CHARGE_MODE_NORMAL;
    
    if(rt_device_write(evse_dev, EVSE_RW_CHARGE_MODE, &config, sizeof(config)) != sizeof(config)) {
        rt_kprintf("Set normal mode failed!\n");
        return;
    }
    rt_kprintf("Set to normal mode\n");
}
MSH_CMD_EXPORT(test_normal_mode, test normal charging mode);
