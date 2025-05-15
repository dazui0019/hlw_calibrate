#pragma once

#include <rtthread.h>
#include <rtdevice.h>

#define HLW8032_DEVICE_NAME    "hlw8032"

// 自定义控制命令
#define RT_DEVICE_CTRL_HLW8032_BASE     (RT_DEVICE_CTRL_BASE(Sensor))
#define HLW8032_CTRL_PARSE_DATA         (RT_DEVICE_CTRL_HLW8032_BASE + 1)
#define HLW8032_CTRL_CALIBRATE          (RT_DEVICE_CTRL_HLW8032_BASE + 2)

struct serial_rx_msg {
    rt_device_t dev;
    rt_size_t size;
};

struct hlw8032_data
{
    float    voltage;   // 电压值
    float    current;   // 电流值
    float    power;     // 功率值
    uint32_t v_param;   // 电压参数
    uint32_t v_reg;     // 电压寄存器
    uint32_t i_param;   // 电流参数
    uint32_t i_reg;     // 电流寄存器
    uint32_t p_param;   // 功率参数
    uint32_t p_reg;     // 功率寄存器
    uint32_t VparamXK;  // 电压校准参数
    uint32_t AparamXK;  // 电流校准参数
    uint32_t PparamXK;  // 功率校准参数
};

struct hlw8032_device
{
    struct rt_device    parent;         // RT-Thread设备基类
    rt_device_t         serial;         // 串口设备句柄
    rt_mq_t             mq;             // 消息队列
    rt_uint8_t          rx_buffer[24];  // 接收缓冲区
    struct hlw8032_data measure_data;   // 测量数据
};

// 导出解析数据函数供应用层使用
int rt_hw_hlw8032_init(void);
