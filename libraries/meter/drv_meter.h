#pragma once

#include <rtthread.h>
#include <rtdevice.h>
#include "drv_hlw8032.h"
#include "drv_cd4051.h"

#define METER_DEVICE_NAME       "meter"
#define METER_CHANNEL_NUM       3

#define METER_CHANNEL_IDLE      3        /* 空闲通道号 */
#define METER_CHANNEL_A         0        /* 通道A */
#define METER_CHANNEL_B         1        /* 通道B */
#define METER_CHANNEL_C         2        /* 通道C */

/* 控制命令 */
#define METER_CTRL_SWITCH_CHANNEL       0x21    /* 切换通道 */
#define METER_CTRL_PARSE_DATA           0x22    /* 解析数据 */
#define METER_CTRL_SWITCH_TO_IDLE       0x23    /* 切换到空闲通道 */
#define METER_CTRL_CALIBRATE            0x24    /* 校准 */

struct meter_device
{
    struct rt_device        parent;
    struct rt_device        *hlw_dev;           /* HLW8032设备 */
    struct rt_device        *cd4051_dev;        /* CD4051设备 */
    struct hlw8032_data     measure_data[METER_CHANNEL_NUM];  /* 每个通道的测量数据 */
    rt_mutex_t              measure_lock;       /* 测量数据互斥锁 */
    rt_uint8_t              current_channel;    /* 当前通道 */
    rt_timer_t              switch_timer;       /* 通道切换定时器 */
    uint8_t                 temp_buffer[RT_SERIAL_RB_BUFSZ];    /* 临时数据缓冲区 */
    void                    *parse_args[METER_CHANNEL_NUM][2];  /* 解析参数数组 */
};