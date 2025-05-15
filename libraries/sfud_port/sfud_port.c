#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <drv_spi.h>

#define DEVICE_NAME "spi10"

static rt_base_t spi10_cs_pin = GET_PIN(B, 12);

int spi10_device_init(void)
{
    static struct rt_spi_device *spi10 = RT_NULL;
    static struct rt_spi_configuration spi_cfg = {
        .mode = RT_SPI_MASTER | RT_SPI_MODE_0 | RT_SPI_MSB,
        .data_width = 8,
        .max_hz = 1 * 1000 * 1000
    };
    rt_err_t ret = RT_EOK;
    
    ret = rt_hw_spi_device_attach("spi1", DEVICE_NAME, spi10_cs_pin);
    if (ret != RT_EOK)
    {
        return 1;
    }
    spi10 = (struct rt_spi_device *)rt_device_find(DEVICE_NAME);
    ret = rt_spi_configure(spi10, &spi_cfg);
    if (ret != RT_EOK)
    {
        return 1;
    }
    return 0;
}
INIT_DEVICE_EXPORT(spi10_device_init);
