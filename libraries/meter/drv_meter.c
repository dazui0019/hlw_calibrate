/**
 * @file drv_meter.c
 * @brief 电表驱动
 * @note 这个设备是组合设备, 包含CD4051和HLW8032, 用于支持三相电测量
 */
#include "drv_meter.h"

#define LOG_TAG     "drv.meter"
#define LOG_LVL     LOG_LVL_DBG
#include <ulog.h>

#define SWITCH_CHANNEL_INTERVAL  1000    /* 通道切换间隔(ms) */

static struct meter_device meter_dev = {0};

// 在文件开头添加信号量声明
static rt_sem_t meter_start_sem = RT_NULL;

/* 设备控制接口 */
static rt_err_t meter_control(rt_device_t dev, int cmd, void *args)
{
    struct meter_device *meter_dev = (struct meter_device *)dev;
    rt_err_t ret = RT_EOK;
    switch (cmd)
    {
        case METER_CTRL_SWITCH_CHANNEL:
            /* 手动切换到指定通道 */
            if ((intptr_t)args >= METER_CHANNEL_NUM)
                return -RT_ERROR;
            meter_dev->current_channel = (intptr_t)args;
            switch(meter_dev->current_channel)
            {
                case METER_CHANNEL_IDLE:
                    return rt_device_control(meter_dev->cd4051_dev, RT_DEVICE_CTRL_CHANNEL_SELECT, (void*)(intptr_t)3);
                case METER_CHANNEL_A:
                    return rt_device_control(meter_dev->cd4051_dev, RT_DEVICE_CTRL_CHANNEL_SELECT, (void*)(intptr_t)2);
                case METER_CHANNEL_B:
                    return rt_device_control(meter_dev->cd4051_dev, RT_DEVICE_CTRL_CHANNEL_SELECT, (void*)(intptr_t)1);
                case METER_CHANNEL_C:
                    return rt_device_control(meter_dev->cd4051_dev, RT_DEVICE_CTRL_CHANNEL_SELECT, (void*)(intptr_t)0);
                default:
                    log_e("Invalid channel: %d", meter_dev->current_channel);
                    return -RT_ERROR;
            }

        case METER_CTRL_SWITCH_TO_IDLE:
            /* 切换到空闲通道 */
            meter_dev->current_channel = METER_CHANNEL_IDLE;
            return rt_device_control(meter_dev->cd4051_dev, RT_DEVICE_CTRL_CHANNEL_SELECT, 
                                   (void *)((intptr_t)METER_CHANNEL_IDLE));
        case METER_CTRL_PARSE_DATA:
            /* 解析数据并存储到当前通道 */
            rt_mutex_take(meter_dev->measure_lock, RT_WAITING_FOREVER);
            ret = rt_device_control(meter_dev->hlw_dev, HLW8032_CTRL_PARSE_DATA, 
                                    meter_dev->parse_args[meter_dev->current_channel]);
            rt_mutex_release(meter_dev->measure_lock);
            break;
        case METER_CTRL_CALIBRATE:
            /* 校准 */
            rt_mutex_take(meter_dev->measure_lock, RT_WAITING_FOREVER);
            ret = rt_device_control(meter_dev->hlw_dev, HLW8032_CTRL_CALIBRATE, 
                                    meter_dev->parse_args[meter_dev->current_channel]);
            rt_mutex_release(meter_dev->measure_lock);
            break;
        default:
            return -RT_ERROR;
    }
    return ret;
}

/**
 * @brief 读取数据
 * @param[in] dev 设备句柄
 * @param[in] pos 读取位置(通道号)
 * @param[out] buffer 读取缓冲区
 * @param[in] size 读取大小
 */
static rt_ssize_t meter_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    struct meter_device *meter_dev = (struct meter_device *)dev;
    rt_ssize_t ret = 0;
    
    if (pos >= METER_CHANNEL_NUM)
        return 0;
        
    if (size == sizeof(struct hlw8032_data)) {
        rt_mutex_take(meter_dev->measure_lock, RT_WAITING_FOREVER);
        rt_memcpy(buffer, &meter_dev->measure_data[pos], size);
        rt_mutex_release(meter_dev->measure_lock);
        ret = size;
    }
    
    return ret;
}

/**
 * @brief 写入数据(校准参数)
 * @param[in] dev 设备句柄
 * @param[in] pos 写入位置(通道号)
 * @param[in] buffer 写入缓冲区
 * @param[in] size 写入大小
 */
static rt_ssize_t meter_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    struct meter_device *meter_dev = (struct meter_device *)dev;

    // 检查通道号是否有效
    if (pos >= METER_CHANNEL_NUM)
        return -RT_ERROR;

    // 检查数据大小是否匹配
    if (size != sizeof(struct meter_calibration_params))
        return -RT_ERROR;

    // 加锁保护
    rt_mutex_take(meter_dev->measure_lock, RT_WAITING_FOREVER);
    
    // 更新校准参数
    const struct meter_calibration_params *params = (const struct meter_calibration_params *)buffer;
    meter_dev->measure_data[pos].VparamXK = params->VparamXK;
    meter_dev->measure_data[pos].AparamXK = params->AparamXK;
    meter_dev->measure_data[pos].PparamXK = params->PparamXK;
    
    rt_mutex_release(meter_dev->measure_lock);
    
    return size;
}

/* 设备初始化 */
static rt_err_t meter_init(rt_device_t dev)
{
    rt_err_t ret = RT_EOK;
    struct meter_device *meter_dev = (struct meter_device *)dev;
    
    /* 创建互斥锁 */
    meter_dev->measure_lock = rt_mutex_create("meter_lock", RT_IPC_FLAG_PRIO);
    if (meter_dev->measure_lock == RT_NULL) {
        log_e("Create mutex failed!");
        return -RT_ERROR;
    }

    /* 初始化解析参数数组 */
    for (int i = 0; i < METER_CHANNEL_NUM; i++) {
        meter_dev->parse_args[i][0] = meter_dev->temp_buffer;
        meter_dev->parse_args[i][1] = &meter_dev->measure_data[i];
    }

    /* 查找并打开CD4051设备 */
    meter_dev->cd4051_dev = rt_device_find("cd4051");
    if (meter_dev->cd4051_dev == RT_NULL) {
        log_e("Can't find cd4051 device!");
        return -RT_ERROR;
    }
    if (rt_device_open(meter_dev->cd4051_dev, RT_DEVICE_FLAG_RDWR) != RT_EOK) {
        log_e("Open cd4051 device failed!");
        return -RT_ERROR;
    }
    
    /* 切换到空闲通道 */
    meter_dev->current_channel = METER_CHANNEL_IDLE;
    rt_device_control(meter_dev->cd4051_dev, RT_DEVICE_CTRL_CHANNEL_SELECT, 
                      (void*)((intptr_t)meter_dev->current_channel));

    /* 查找并打开HLW8032设备 */
    meter_dev->hlw_dev = rt_device_find("hlw8032");
    if (meter_dev->hlw_dev == RT_NULL) {
        log_e("Can't find hlw8032 device!");
        return -RT_ERROR;
    }
    if (rt_device_open(meter_dev->hlw_dev, RT_DEVICE_FLAG_RDWR) != RT_EOK) {
        log_e("Open hlw8032 device failed!");
        return -RT_ERROR;
    }
    
    return ret;
}

/* 注册组合设备 */
int rt_hw_meter_init(void)
{
    rt_err_t ret = RT_EOK;
    struct meter_device *dev = &meter_dev;
    
    dev->parent.type        = RT_Device_Class_Sensor;
    dev->parent.init        = meter_init;
    dev->parent.control     = meter_control;
    dev->parent.read        = meter_read;
    dev->parent.write       = meter_write;
    dev->parent.user_data   = RT_NULL;
    dev->parent.open        = RT_NULL;

    // 创建信号量
    meter_start_sem = rt_sem_create("meter_start", 0, RT_IPC_FLAG_FIFO);  // 用来通知线程开始工作
    if (meter_start_sem == RT_NULL) {
        log_e("Create meter semaphore failed!");
        return -RT_ERROR;
    }
    
    ret = rt_device_register(&dev->parent, METER_DEVICE_NAME, RT_DEVICE_FLAG_RDWR);
    
    return ret;
}
INIT_BOARD_EXPORT(rt_hw_meter_init);
