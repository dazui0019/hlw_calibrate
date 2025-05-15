/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author            Notes
 * 2022-01-25     iysheng           first version
 */

#include <board.h>
#include <sys/time.h>

#define DBG_TAG             "drv.rtc"
#define DBG_LVL             LOG_LVL_ERROR
#include "ulog.h"

#ifdef RT_USING_RTC

#define RTC_BCD2DEC(n)          ((((n) >> 4) * 10) + ((n) & 0x0F))
#define RTC_DEC2BCD(n)          ((((n) / 10) << 4) | ((n) % 10))

#define BKP_VALUE    0x32F0

typedef struct {
    struct rt_device rtc_dev;
} gd32_rtc_device;

static gd32_rtc_device g_gd32_rtc_dev;

struct rtc_device_object
{
    rt_rtc_dev_t  rtc_dev;
#ifdef RT_USING_ALARM
    struct rt_rtc_wkalarm   wkalarm;
#endif
};

static struct rtc_device_object rtc_device;

rtc_parameter_struct   rtc_initpara;
#ifdef RT_USING_ALARM
static rt_err_t rtc_alarm_time_set(struct rtc_device_object* p_dev);
static int rt_rtc_alarm_init(void);
static rtc_alarm_struct  rtc_alarm;
#endif

static rt_err_t gd32_rtc_get_secs(time_t *sec)
{
    rtc_parameter_struct rtc_time;
    struct tm tm_new = {0};

    rtc_current_time_get(&rtc_time);

    /* 将RTC时间(BCD码)转换为tm结构(十进制) */
    tm_new.tm_year = RTC_BCD2DEC(rtc_time.year)  + 100;
    tm_new.tm_mon = RTC_BCD2DEC(rtc_time.month) - 1;
    tm_new.tm_mday = RTC_BCD2DEC(rtc_time.date);
    tm_new.tm_hour = RTC_BCD2DEC(rtc_time.hour);
    tm_new.tm_min = RTC_BCD2DEC(rtc_time.minute);
    tm_new.tm_sec = RTC_BCD2DEC(rtc_time.second);
    tm_new.tm_wday = RTC_BCD2DEC(rtc_time.day_of_week);

    /* 转换为时间戳 */
    *sec = timegm(&tm_new);

    return RT_EOK;
}

static rt_err_t set_rtc_time_stamp(time_t time_stamp)
{
    #if defined (SOC_SERIES_GD32F3xx)
    uint32_t rtc_counter;

    rtc_counter = (uint32_t)time_stamp;

    /* wait until LWOFF bit in RTC_CTL to 1 */
    rtc_lwoff_wait();
    /* enter configure mode */
    rtc_configuration_mode_enter();
    /* write data to rtc register */
    rtc_counter_set(rtc_counter);
    /* exit configure mode */
    rtc_configuration_mode_exit();
    /* wait until LWOFF bit in RTC_CTL to 1 */
    rtc_lwoff_wait();
    #elif defined (SOC_SERIES_GD32F4xx)
    struct tm tm_new = {0};

    /* 将时间戳转换为tm结构 */
    gmtime_r(&time_stamp, &tm_new);

    /* 填充RTC初始化结构体(转换为BCD码) */
    rtc_initpara.year = RTC_DEC2BCD(tm_new.tm_year - 100); 
    rtc_initpara.month = RTC_DEC2BCD(tm_new.tm_mon + 1);
    rtc_initpara.date = RTC_DEC2BCD(tm_new.tm_mday);
    rtc_initpara.day_of_week = RTC_DEC2BCD(tm_new.tm_wday);
    rtc_initpara.hour = RTC_DEC2BCD(tm_new.tm_hour);
    rtc_initpara.minute = RTC_DEC2BCD(tm_new.tm_min);
    rtc_initpara.second = RTC_DEC2BCD(tm_new.tm_sec);
    rtc_initpara.display_format = RTC_24HOUR;
    rtc_initpara.factor_asyn = 0x7F;        // 异步预分频系数
    rtc_initpara.factor_syn = 0xFF;         // 同步预分频系数
    rtc_initpara.am_pm = RTC_AM;            // 24小时制

    /* 初始化RTC */
    if(ERROR == rtc_init(&rtc_initpara)) {
        return -RT_ERROR;
    }
    #endif

    return RT_EOK;
}

static rt_err_t gd32_rtc_set_secs(time_t *sec)
{
    rt_err_t result = RT_EOK;

    if (set_rtc_time_stamp(*sec))
    {
        result = -RT_ERROR;
    }
    LOG_D("RTC: set rtc_time %d", *sec);
#ifdef RT_USING_ALARM
    rt_alarm_update(&rtc_device.rtc_dev.parent, 1);
#endif
    return result;
}

static rt_err_t gd32_rtc_init(void)
{
    uint32_t RTCSRC_FLAG = 0;
    /* enable PMU clock */
    rcu_periph_clock_enable(RCU_PMU);
    /* enable the access of the RTC registers */
    pmu_backup_write_enable();
#if defined (BSP_RTC_USING_LSI)
    rcu_osci_on(RCU_IRC32K);
    rcu_osci_stab_wait(RCU_IRC32K);
    rcu_rtc_clock_config(RCU_RTCSRC_IRC32K);

    rtc_initpara.factor_syn = 0x13F;
    rtc_initpara.factor_asyn = 0x63;
#elif defined (BSP_RTC_USING_LSE)
    rcu_osci_on(RCU_LXTAL);
    rcu_osci_stab_wait(RCU_LXTAL);
    rcu_rtc_clock_config(RCU_RTCSRC_LXTAL);

    rtc_initpara.factor_syn = 0xFF;
    rtc_initpara.factor_asyn = 0x7F;
#endif /* RTC_CLOCK_SOURCE_IRC32K */

    rcu_periph_clock_enable(RCU_RTC);
    rtc_register_sync_wait();

    /* get RTC clock entry selection */
    RTCSRC_FLAG = GET_BITS(RCU_BDCTL, 8, 9);

    /* check if RTC has aready been configured */
    if((BKP_VALUE != RTC_BKP0) || (0x00 == RTCSRC_FLAG)){
        /* backup data register value is not correct or not yet programmed
        or RTC clock source is not configured (when the first time the program 
        is executed or data in RCU_BDCTL is lost due to Vbat feeding) */
        set_rtc_time_stamp(0);
        RTC_BKP0 = BKP_VALUE;
    }else{
        /* detect the reset source */
        if (RESET != rcu_flag_get(RCU_FLAG_PORRST)){
            log_d("power on reset occurred....");
        }else if (RESET != rcu_flag_get(RCU_FLAG_EPRST)){
            log_d("external reset occurred....");
        }
        log_d("no need to configure RTC....");
    }
    rcu_all_reset_flag_clear();
    rtc_flag_clear(RTC_FLAG_ALRM0);

    #ifdef RT_USING_ALARM
    rt_rtc_alarm_init();
    #endif

    return RT_EOK;
}

static rt_err_t stm32_rtc_get_alarm(struct rt_rtc_wkalarm *alarm)
{
#ifdef RT_USING_ALARM
    *alarm = rtc_device.wkalarm;
    LOG_D("GET_ALARM %d:%d:%d",rtc_device.wkalarm.tm_hour,
        rtc_device.wkalarm.tm_min,rtc_device.wkalarm.tm_sec);
    return RT_EOK;
#else
    return -RT_ERROR;
#endif
}

static rt_err_t stm32_rtc_set_alarm(struct rt_rtc_wkalarm *alarm)
{
#ifdef RT_USING_ALARM
    LOG_D("RT_DEVICE_CTRL_RTC_SET_ALARM");
    if (alarm != RT_NULL){
        rtc_device.wkalarm.enable = alarm->enable;
        rtc_device.wkalarm.tm_year = alarm->tm_year;
        rtc_device.wkalarm.tm_mon = alarm->tm_mon;
        rtc_device.wkalarm.tm_mday = alarm->tm_mday;
        rtc_device.wkalarm.tm_hour = alarm->tm_hour;
        rtc_device.wkalarm.tm_min = alarm->tm_min;
        rtc_device.wkalarm.tm_sec = alarm->tm_sec;
        rtc_alarm_time_set(&rtc_device);
    }else{
        LOG_E("RT_DEVICE_CTRL_RTC_SET_ALARM error!!");
        return -RT_ERROR;
    }
    return RT_EOK;
#else
    return -RT_ERROR;
#endif
}

static const struct rt_rtc_ops gd32_rtc_ops =
{
    gd32_rtc_init,
    gd32_rtc_get_secs,
    gd32_rtc_set_secs,
    stm32_rtc_get_alarm,
    stm32_rtc_set_alarm,
    RT_NULL,
    RT_NULL,
};

#ifdef RT_USING_ALARM
void rt_rtc_alarm_enable(void)
{
    rtc_alarm_disable(RTC_ALARM0);
    /* RTC alarm configuration */
    rtc_alarm_config(RTC_ALARM0, &rtc_alarm);
    LOG_I("alarm enable: %d:%d:%d", RTC_BCD2DEC(rtc_alarm.alarm_hour),
                                RTC_BCD2DEC(rtc_alarm.alarm_minute),
                                RTC_BCD2DEC(rtc_alarm.alarm_second));
    rtc_interrupt_enable(RTC_INT_ALARM0);
    rtc_alarm_enable(RTC_ALARM0);
}

void rt_rtc_alarm_disable(void)
{
    rtc_alarm_disable(RTC_ALARM0);
}

static int rt_rtc_alarm_init(void)
{
    rtc_flag_clear(RTC_FLAG_ALRM0);
    exti_flag_clear(EXTI_17);
    /* RTC alarm interrupt configuration */
    exti_init(EXTI_17, EXTI_INTERRUPT, EXTI_TRIG_RISING); // Alarm需要使用EXTI_17
    rtc_alarm_disable(RTC_ALARM0);
    rtc_alarm_disable(RTC_ALARM1);
    nvic_irq_enable(RTC_Alarm_IRQn, 2, 0);
    return RT_EOK;
}

static rt_err_t rtc_alarm_time_set(struct rtc_device_object* p_dev)
{
    if (p_dev->wkalarm.enable){
        rtc_alarm.alarm_mask = RTC_ALARM_DATE_MASK;
        rtc_alarm.weekday_or_date = RTC_ALARM_DATE_SELECTED;
        rtc_alarm.alarm_day = RTC_DEC2BCD(p_dev->wkalarm.tm_mday);
        rtc_alarm.am_pm = RTC_AM;   // 24小时制
        rtc_alarm.alarm_hour = RTC_DEC2BCD(p_dev->wkalarm.tm_hour);
        rtc_alarm.alarm_minute = RTC_DEC2BCD(p_dev->wkalarm.tm_min);
        rtc_alarm.alarm_second = RTC_DEC2BCD(p_dev->wkalarm.tm_sec);
        rt_rtc_alarm_enable();
    }else{
        rt_rtc_alarm_disable();
        LOG_I("alarm disable: %d:%d:%d", RTC_BCD2DEC(rtc_alarm.alarm_hour),
                                RTC_BCD2DEC(rtc_alarm.alarm_minute),
                                RTC_BCD2DEC(rtc_alarm.alarm_second));
    }
    return RT_EOK;
}

void GD32_RTC_AlarmAEventCallback()
{
    if(RESET != rtc_flag_get(RTC_FLAG_ALRM0)){
        rtc_flag_clear(RTC_FLAG_ALRM0);
        exti_flag_clear(EXTI_17);
        rt_alarm_update(&rtc_device.rtc_dev.parent, 1);
        LOG_D("rtc alarm isr.");
    }
}

void RTC_Alarm_IRQHandler(void)
{
    rt_interrupt_enter();
    GD32_RTC_AlarmAEventCallback();
    rt_interrupt_leave();
}
#endif

static int rt_hw_rtc_init(void)
{
    rt_err_t ret;

    rtc_device.rtc_dev.ops = &gd32_rtc_ops;

    ret = rt_hw_rtc_register(&rtc_device.rtc_dev, "rtc", RT_DEVICE_FLAG_RDWR, RT_NULL);
    if (ret != RT_EOK){
        LOG_E("failed register internal rtc device, err=%d", ret);
        return ret;
    }
    LOG_D("rtc init success");
    
    return ret;
}
INIT_DEVICE_EXPORT(rt_hw_rtc_init);
#endif
