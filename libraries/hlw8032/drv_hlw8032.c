#include "drv_hlw8032.h"

#define LOG_TAG     "drv.hlw8032"
#define LOG_LVL     LOG_LVL_ERROR
#include <ulog.h>

#define HLW8032_UART_NAME  "uart7"      // HLW8032使用的串口设备名

/* 采用隔离设计时(互感器)，计算电压，电流，功率所需的参数 */
/**
 * todo: 需要实现校准功能, 现在是手算的。
 *      - 已实现校准功能, 还需要实现校准参数的保存和读取。
 */
static uint32_t VparamXK = 2553320U;     // voltageREG*220V
static uint32_t AparamXK = 32540U;     // currentREG*10A
static uint32_t PparamXK = 140960600U;   // powerREG*220W

/* 消息队列控制块 */
static struct rt_messagequeue rx_mq;
__attribute__((aligned(4))) static rt_uint8_t msg_pool[64];   // 消息队列缓冲区

static struct hlw8032_device hlw8032_dev = {0};


static rt_err_t hlw8032_uart_rx_ind(rt_device_t dev, rt_size_t size)
{
    struct hlw8032_device *hlw = (struct hlw8032_device*)&hlw8032_dev;
    struct serial_rx_msg msg;
    static rt_size_t msg_size = sizeof(struct serial_rx_msg);

    rt_err_t result = RT_EOK;
    msg.dev = dev;
    msg.size = size;

    result = rt_mq_send(hlw->mq, &msg, msg_size);
    if ( result == -RT_EFULL){
        /* 消息队列满 */
        log_d("message queue full.\n");
    }
    return result;
}

/**
 * @brief 解析HLW8032数据
 * @param buffer: 数据缓冲区
 * @param data: 数据结构体
 * @return 0: 成功, -1: 失败
 */
static rt_err_t hlw8032_parse_data(rt_uint8_t *buffer, struct hlw8032_data *data)
{
    if (buffer[1] != 0x5a) {
        log_w("CheckREG Error!");
        return -RT_ERROR;
    }

    switch (buffer[0])
    {
        case 0x55:
        case 0xF2:
            if ((buffer[20] & 0x40) == 0x40) {
                data->v_param = ((buffer[2] << 16) | (buffer[3] << 8) | (buffer[4]));
                data->v_reg = ((buffer[5] << 16) | (buffer[6] << 8) | (buffer[7]));
                data->voltage = ((float)(VparamXK) / (float)(data->v_reg));
                log_d("Voltage: %0.2fV", data->voltage);
            }

            if ((buffer[20] & 0x20) == 0x20) {
                data->i_param = ((buffer[8] << 16) | (buffer[9] << 8) | (buffer[10]));
                data->i_reg = ((buffer[11] << 16) | (buffer[12] << 8) | (buffer[13]));
                data->current = ((float)(AparamXK) / (float)(data->i_reg));
                log_d("Current: %0.2fA", data->current);
            }

            if ((buffer[20] & 0x10) == 0x10) {
                data->p_param = ((buffer[14] << 16) | (buffer[15] << 8) | (buffer[16]));
                data->p_reg = ((buffer[17] << 16) | (buffer[18] << 8) | (buffer[19]));
                data->power = ((float)(PparamXK) / (float)(data->p_reg));
                log_d("Power: %0.2fW", data->power);
            }
            break;

        case 0xAA:
            log_w("HLW8032 not check.");
            return -RT_ERROR;
            break;

        default:
            log_w("other state: 0x%X", buffer[0]);
            return -RT_ERROR;
            break;
    }

    return RT_EOK;
}

/**
 * @brief 校准HLW8032
 * @param buffer: 数据缓冲区
 * @param data: 数据结构体
 * @return 0: 成功, -1: 失败
 */
static rt_err_t hlw8032_calibrate(rt_uint8_t *buffer, struct hlw8032_data *data)
{
    if (buffer[1] != 0x5a) {
        log_w("CheckREG Error!");
        return -RT_ERROR;
    }

    switch (buffer[0])
    {
        case 0x55:
        case 0xF2:
            if ((buffer[20] & 0x40) == 0x40) {
                uint32_t v_reg = ((buffer[5] << 16) | (buffer[6] << 8) | (buffer[7]));
                data -> VparamXK = v_reg * 220;
                log_i("VparamXK: %ld", data -> VparamXK);
            }

            if ((buffer[20] & 0x20) == 0x20) {
                uint32_t i_reg = ((buffer[11] << 16) | (buffer[12] << 8) | (buffer[13]));
                data -> AparamXK = i_reg * 10;
                log_i("AparamXK: %ld", data -> AparamXK);
            }

            if ((buffer[20] & 0x10) == 0x10) {
                uint32_t p_reg = ((buffer[17] << 16) | (buffer[18] << 8) | (buffer[19]));
                data -> PparamXK = p_reg * 2200;
                log_i("PparamXK: %ld", data -> PparamXK);
            }
            break;

        case 0xAA:
            log_w("HLW8032 not check.");
            return -RT_ERROR;
            break;

        default:
            log_w("other state: 0x%X", buffer[0]);
            return -RT_ERROR;
            break;
    }

    return RT_EOK;
}

static rt_err_t hlw8032_control(rt_device_t dev, int cmd, void *args)
{
    struct hlw8032_device *hlw = (struct hlw8032_device *)dev;
    rt_uint8_t *buffer;
    struct hlw8032_data *data;
    
    switch (cmd)
    {
        case HLW8032_CTRL_PARSE_DATA:
            if (args == RT_NULL)
                return -RT_ERROR;
                
            buffer = ((void **)args)[0];
            data = ((void **)args)[1];
            
            return hlw8032_parse_data(buffer, data);
        case HLW8032_CTRL_CALIBRATE:
            if (args == RT_NULL)
                return -RT_ERROR;
                
            buffer = ((void **)args)[0];
            data = ((void **)args)[1];
            return hlw8032_calibrate(buffer, data);

        default:
            return -RT_ERROR;
    }
}

/**
 * @brief 读取HLW8032数据
 * @param dev: 设备句柄
 * @param pos: 位置
 * @param buffer: 缓冲区
 * @param size: 大小
 * @return 读取的字节数
 * @note 读取单个hlw8032芯片
 */
static rt_ssize_t hlw8032_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    struct hlw8032_device *hlw = (struct hlw8032_device *)dev;
    
    if (size == sizeof(struct hlw8032_data)) {
        rt_memcpy(buffer, &hlw->measure_data, size);
        return size;
    }
    return 0;
}

static rt_err_t hlw8032_init(rt_device_t dev)
{
    rt_err_t ret = RT_EOK;
    struct hlw8032_device *hlw = (struct hlw8032_device *)dev;
    
    // 配置串口参数
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
    config.baud_rate = BAUD_RATE_4800;
    config.parity = PARITY_EVEN;

    // 初始化消息队列
    hlw->mq = &rx_mq;
    rt_mq_init(hlw->mq, "hlw_mq",
               msg_pool,
               sizeof(struct serial_rx_msg),
               sizeof(msg_pool),
               RT_IPC_FLAG_FIFO);

    // 打开串口设备
    hlw->serial = rt_device_find(HLW8032_UART_NAME);
    if (hlw->serial == RT_NULL) {
        log_e("Can't find %s device!", HLW8032_UART_NAME);
        return -RT_ERROR;
    }
    
    // 配置串口
    rt_device_control(hlw->serial, RT_DEVICE_CTRL_CONFIG, &config);

    // 打开串口设备
    ret = rt_device_open(hlw->serial, RT_DEVICE_FLAG_DMA_RX);
    if (ret != RT_EOK)
        return ret;
    
    // 设置接收回调
    rt_device_set_rx_indicate(hlw->serial, hlw8032_uart_rx_ind);
    
    return ret;
}

int rt_hw_hlw8032_init(void)
{
    struct hlw8032_device *hlw = &hlw8032_dev;
    
    hlw->parent.type        = RT_Device_Class_Sensor;
    hlw->parent.init        = hlw8032_init;
    hlw->parent.read        = hlw8032_read;
    hlw->parent.write       = RT_NULL;
    hlw->parent.control     = hlw8032_control;
    
    return rt_device_register(&hlw->parent, HLW8032_DEVICE_NAME, RT_DEVICE_FLAG_RDWR);
}
INIT_BOARD_EXPORT(rt_hw_hlw8032_init);
