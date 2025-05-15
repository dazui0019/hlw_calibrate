#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "evse_ac.h"
#include "drv_cd4051.h"
#include "drv_usart.h"
#include "evse_main_thread.h"

#define LOG_TAG "app.evse_ac"
#define LOG_LVL LOG_LVL_WARNING
#include <ulog.h>

// Urms>253V(1.15*220V)时触发过压, 当Urms<242V(1.1*220V)时解除过压
// Urms<187V(0.85*220V)时触发欠压, 当Urms>198V(0.9*220V)时解除欠压

// 添加状态变量在文件开头
static rt_bool_t is_overvoltage = RT_FALSE;    // 过压状态
static rt_bool_t is_undervoltage = RT_FALSE;   // 欠压状态

/* 采用隔离设计时(互感器)，计算电压，电流，功率所需的参数 */
static uint32_t VparamXK = 394900U;     // voltageREG*220V
static uint32_t AparamXK = 101700U;     // currentREG*10A
static uint32_t PparamXK = 63800000U;   // powerREG*220W

#define UART_NAME    "uart7"

static rt_device_t serial;
static rt_device_t cd4051;
static struct rt_semaphore rx_sem;

/* 串口接收消息结构*/
struct rx_msg{
    rt_device_t dev;
    rt_size_t size;
};

/* 消息队列控制块 */
static struct rt_messagequeue rx_mq;

/* 接收数据回调函数 */
static rt_err_t uart_input(rt_device_t dev, rt_size_t size)
{
    struct rx_msg msg;
    static rt_size_t msg_size = sizeof(struct rx_msg);

    rt_err_t result = RT_EOK;
    msg.dev = dev;
    msg.size = size;

    result = rt_mq_send(&rx_mq, &msg, msg_size);
    if ( result == -RT_EFULL){
        /* 消息队列满 */
        log_d("message queue full.\n");
    }
    return result;
}

void process_data(uint8_t *data)
{
    /* 寄存器 */
    static uint8_t StateREG = 0;            // 状态寄存器，1字节
    static uint8_t UpdateREG = 0;           // 数据更新寄存器，1字节
    static uint32_t VoltageParamREG = 0;    // 电压参数寄存器，3字节
    static uint32_t voltageREG = 0;         // 电压寄存器，3字节
    static uint32_t CurrentParamREG = 0;    // 电流参数寄存器，3字节
    static uint32_t CurrentREG = 0;         // 电流寄存器，3字节
    static uint32_t PowerParamREG = 0;      // 功率参数寄存器，3字节
    static uint32_t PowerREG = 0;           // 功率寄存器，3字节

    static float voltage = 0.0f;
    static float current = 0.0f;
    static float power = 0.0f;

    // todo: 处理数据
    /* 处理数据 */
    /* 这里考虑下是否要做CRC校验，个人感觉没必要 */

    /* 判断检测寄存器的值是否为0x5a，该值为固定值，如果不一致说明接收到的数据存在问题 */
    if (data[1] != 0x5a){
        log_w("CheckREG Error!");
    }else{
        /* 获取StateREG的值，状态寄存器的每一位，表明当前计量芯片的工作状态 */
        StateREG = data[0];
        /* 根据StateREG的值，执行相应的动作 */
        switch (StateREG)
        {
            /* 正常工作时 */
            case 0x55:
            case 0xF2:  // 功率接近0
                /* 计算电压，获取DataUpdateREG的值，bit6为1时，说明电压检测OK */
                if ((data[20] & 0x40) == 0x40){
                    /* 根据电压参数寄存器，电压寄存器 */
                    VoltageParamREG = ((data[2] << 16) | (data[3] << 8) | (data[4]));
                    voltageREG = ((data[5] << 16) | (data[6] << 8) | (data[7]));
                    voltage = ((float)(VparamXK) / (float)(voltageREG));
                    log_d("Vparam:%d, Vreg:%d, V:%0.2f", VoltageParamREG, voltageREG, voltage);
                    /* 过压检测 */
                    if (!is_overvoltage && voltage >= EVSE_OVERVOLTAGE_THRESHOLD) {
                        /* 触发过压保护 */
                        is_overvoltage = RT_TRUE;
                        log_e("Over voltage protection triggered: %0.2fV", voltage);
                        // TODO: 添加过压保护处理逻辑
                        evse_fault_set_flag(FAULT_OVER_VOL);
                    }else if(is_overvoltage && voltage <= EVSE_OVERVOLTAGE_RECOVERY) {
                        /* 过压恢复 */
                        is_overvoltage = RT_FALSE;
                        log_w("Over voltage recovered: %0.2fV", voltage);
                        // TODO: 添加过压恢复处理逻辑
                        evse_fault_clear_flag(FAULT_OVER_VOL);
                    }
                    /* 欠压检测 */
                    if (!is_undervoltage && voltage <= EVSE_UNDERVOLTAGE_THRESHOLD) {
                        /* 触发欠压保护 */
                        is_undervoltage = RT_TRUE;
                        log_e("Under voltage protection triggered: %0.2fV", voltage);
                        // TODO: 添加欠压保护处理逻辑
                        evse_fault_set_flag(FAULT_UNDER_VOL);
                    }else if(is_undervoltage && voltage >= EVSE_UNDERVOLTAGE_RECOVERY) {
                        /* 欠压恢复 */
                        is_undervoltage = RT_FALSE;
                        log_w("Under voltage recovered: %0.2fV", voltage);
                        // TODO: 添加欠压恢复处理逻辑
                        evse_fault_clear_flag(FAULT_UNDER_VOL);
                    }
                }else{
                    log_i("Voltage Flag Error.");
                }

                /* 计算电流，获取DataUpdateREG的值，bit5为1时，说明电流检测OK */
                if ((data[20] & 0x20) == 0x20){
                    /* 根据电流参数寄存器，电流寄存器 */
                    CurrentParamREG = ((data[8] << 16) | (data[9] << 8) | (data[10]));
                    CurrentREG = ((data[11] << 16) | (data[12] << 8) | (data[13]));
                    current = ((float)(AparamXK) / (float)(CurrentREG));
                    log_d("Aparam:%d, Creg:%d, A:%0.2f", CurrentParamREG, CurrentREG, current);
                }else{
                    log_i("Current Flag Error.");
                }

                /* 计算功率，获取DataUpdateREG的值，bit4为1时，说明电流检测OK */
                if ((data[20] & 0x10) == 0x10){
                    /* 根据电流参数寄存器，电流寄存器 */
                    PowerParamREG = ((data[14] << 16) | (data[15] << 8) | (data[16]));
                    PowerREG = ((data[17] << 16) | (data[18] << 8) | (data[19]));
                    power = ((float)(PparamXK) / (float)(PowerREG));
                    log_d("Pparam:%d, Preg:%d, P:%0.2f", PowerParamREG, PowerREG, power);
                }else{
                    log_i("Power Flag Error.");
                }
                break;
            /* 芯片未校准 */
            case 0xAA:
                log_w("HLW8032 not check.");
                break;
            /* 其他情况 */
            default:
                log_i("other state: 0x%X", StateREG);
                break;
        }
    }
}

static void serial_thread_entry(void *parameter)
{
    struct rx_msg msg;
    rt_err_t result;
    rt_uint32_t rx_length;
    static uint8_t rx_buffer[RT_SERIAL_RB_BUFSZ];
    static uint8_t temp_buffer[24];  // 临时缓冲区存储不完整的数据
    static rt_uint32_t accumulated_length = 0;  // 已累积的数据长度

    while (1){
        rt_memset(&msg, 0, sizeof(msg));
        /* 从消息队列中读取消息*/
        result = rt_mq_recv(&rx_mq, &msg, sizeof(msg), RT_WAITING_FOREVER);
        if (result > 0) {
            /* 从串口读取数据*/
            rx_length = rt_device_read(msg.dev, 0, rx_buffer, msg.size);
            /* 处理接收到的数据 */
            if (rx_length == 24) {
                /* 直接处理完整的24字节数据 */
                process_data(rx_buffer);
                accumulated_length = 0;
                continue;
            }
            /* 处理不完整数据 */
            if (accumulated_length == 0) {
                /* 开始新的数据累积 */
                rt_memcpy(temp_buffer, rx_buffer, rx_length);
                accumulated_length = rx_length;
            } else if (accumulated_length + rx_length == 24) {
                /* 拼接后刚好24字节，可以处理数据 */
                rt_memcpy(temp_buffer + accumulated_length, rx_buffer, rx_length);
                process_data(temp_buffer);
                accumulated_length = 0;
            } else {
                /* 丢弃之前的数据，保存新数据 */
                rt_memcpy(temp_buffer, rx_buffer, rx_length);
                accumulated_length = rx_length;
            }
        }
    }
}

int uart_dma_sample()
{
    rt_err_t ret = RT_EOK;
    static char msg_pool[64];
    char str[] = "hello RT-Thread!\r\n";
    /* 配置串口参数 */
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;  /* 使用默认配置 */
    config.baud_rate = BAUD_RATE_4800;
    config.parity = PARITY_EVEN;

    /* 查找串口设备 */
    serial = rt_device_find(UART_NAME);
    if (!serial){
        log_d("find %s failed!", UART_NAME);
        return RT_ERROR;
    }

    /* 控制串口设备 */
    rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &config);

    cd4051 = rt_device_find("cd4051");
    if (cd4051){
        rt_device_open(cd4051, RT_DEVICE_FLAG_RDWR);
    }

    rt_uint8_t channel = 2;  // 选择通道3
    rt_device_control(cd4051, RT_DEVICE_CTRL_CHANNEL_SELECT, &channel);

    /* 初始化消息队列 */
    rt_mq_init(&rx_mq, "rx_mq",
               msg_pool,                 /* 存放消息的缓冲区 */
               sizeof(struct rx_msg),    /* 一条消息的最大长度 */
               sizeof(msg_pool),         /* 存放消息的缓冲区大小 */
               RT_IPC_FLAG_FIFO);        /* 如果有多个线程等待，按照先来先得到的方法分配消息 */

    /* 以 DMA 接收及轮询发送方式打开串口设备 */
    rt_device_open(serial, RT_DEVICE_FLAG_DMA_RX);
    /* 设置接收回调函数 */
    rt_device_set_rx_indicate(serial, uart_input);

    /* 创建 serial 线程 */
    rt_thread_t thread = rt_thread_create("serial", serial_thread_entry, RT_NULL, 1024, 25, 10);
    /* 创建成功则启动线程 */
    if (thread != RT_NULL){
        rt_thread_startup(thread);
    }else{
        ret = RT_ERROR;
    }

    return ret;
}
// INIT_APP_EXPORT(uart_dma_sample);
