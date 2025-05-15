#include <rtthread.h>
#include "tuya_api.h"

#ifdef FINSH_USING_MSH
static void tuya_fault(int argc, char **argv)
{
    uint16_t fault = 0;
    
    if (argc < 2) {
        rt_kprintf("Usage: tuya_fault <fault_code>\n");
        rt_kprintf("fault_code:\n");
        rt_kprintf("  0x0001 - 过流故障1\n");
        rt_kprintf("  0x0002 - 过流故障2\n");
        rt_kprintf("  0x0004 - 过压故障\n");
        rt_kprintf("  0x0008 - 欠压故障\n");
        rt_kprintf("  0x0010 - 接触器粘连故障\n");
        rt_kprintf("  0x0020 - 接触器故障\n");
        rt_kprintf("  0x0040 - 接地故障\n");
        rt_kprintf("  0x0080 - 电表故障\n");
        rt_kprintf("  0x0100 - 急停故障\n");
        rt_kprintf("  0x0200 - CP故障\n");
        rt_kprintf("  0x0400 - 电表通讯故障\n");
        rt_kprintf("  0x0800 - 读卡器故障\n");
        rt_kprintf("  0x1000 - 短路故障\n");
        rt_kprintf("  0x2000 - 粘连故障\n");
        rt_kprintf("  0x4000 - 自检故障\n");
        rt_kprintf("  0x8000 - 漏电故障\n");
        return;
    }
    
    fault = strtol(argv[1], NULL, 16);
    UPDATE_fault(fault);
    rt_kprintf("Set fault code: 0x%04X\n", fault);
}
MSH_CMD_EXPORT(tuya_fault, tuya fault test);

/* 测试命令列表 */
static void tuya_test_usage(void)
{
    rt_kprintf("tuya_test [option] [param]\n");
    rt_kprintf("         switch on     - 开启充电\n");
    rt_kprintf("         switch off    - 关闭充电\n");
    rt_kprintf("         lock on       - 开锁\n");
    rt_kprintf("         lock off      - 上锁\n");
    rt_kprintf("         energy <kWh>  - 设置单次充电电量\n");
    // ... 其他命令说明 ...
}

/* 测试命令处理函数 */
static int tuya_test(int argc, char *argv[])
{
    if (argc < 2)
    {
        tuya_test_usage();
        return -RT_ERROR;
    }

    if (!rt_strcmp(argv[1], "switch")){
        if (argc != 3){
            rt_kprintf("Usage: tuya_test switch <on/off>\n");
            return -RT_ERROR;
        }

        if (!rt_strcmp(argv[2], "on")){
            UPDATE_SWITCH(SWITCH_ON);
            rt_kprintf("Switch charging on\n");
        }else if (!rt_strcmp(argv[2], "off")){
            UPDATE_SWITCH(SWITCH_OFF);
            rt_kprintf("Switch charging off\n");
        }else{
            rt_kprintf("Invalid parameter, should be 'on' or 'off'\n");
            return -RT_ERROR;
        }
        return RT_EOK;
    }else if (!rt_strcmp(argv[1], "lock")){
        if (argc != 3){
            rt_kprintf("Usage: tuya_test lock <on/off>\n");
            return -RT_ERROR;
        }

        if (!rt_strcmp(argv[2], "on")){
            UPDATE_manual_lock(SWITCH_ON);
            rt_kprintf("Manual lock on (unlock)\n");
        }else if (!rt_strcmp(argv[2], "off")){
            UPDATE_manual_lock(SWITCH_OFF);
            rt_kprintf("Manual lock off (lock)\n");
        }else{
            rt_kprintf("Invalid parameter, should be 'on' or 'off'\n");
            return -RT_ERROR;
        }
        return RT_EOK;
    }else if (!rt_strcmp(argv[1], "energy")){
        if (argc != 3){
            rt_kprintf("Usage: tuya_test energy <kWh>\n");
            rt_kprintf("kWh range: 1-999999\n");
            return -RT_ERROR;
        }

        int kwh = atoi(argv[2]);
        if (kwh < 1 || kwh > 999999){
            rt_kprintf("Invalid kWh value, should be between 1 and 999999\n");
            return -RT_ERROR;
        }

        UPDATE_energy_once(kwh);
        rt_kprintf("Set energy once: %d kWh\n", kwh);
        return RT_EOK;
    }

    return RT_EOK;
}
MSH_CMD_EXPORT(tuya_test, tuya function test);
#endif
