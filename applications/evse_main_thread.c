#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <drv_spi.h>
#include "SEGGER_RTT.h"
#include "dev_relay.h"
#include "dev_lock.h"
#include "drv_meter.h"
#include "evse_db.h"
#include "evse_meter_thread.h"

#define LOG_TAG "app.evse_main"
#define LOG_LVL LOG_LVL_DBG
#include <ulog.h>

#if defined(BSP_USING_EVENT_RECORDER)
#include "EventRecorder.h"
#endif

static int32_t PparamXK_offset = 1000000;
static int32_t PparamXK_last = 0;

static uint32_t VparamXK_SUM = 0;
static uint32_t AparamXK_SUM = 0;
static uint32_t PparamXK_SUM = 0;

/* hlw8032 */
static uint32_t VparamXK[3] = {0, 0, 0};
static uint32_t IparamXK[3] = {0, 0, 0};
static uint32_t PparamXK[3] = {0, 0, 0};

static uint32_t hlw_calibrated = 0;

static int32_t this_abs(int32_t i)
{
    return (i >= 0 ? i : -i);
}

void evse_main_thread_entry(void *parameter)
{
    rt_thread_t calibrate_tid, measure_tid;
    calibrate_tid = rt_thread_create( "calibrate",
                            evse_calibrate_thread_entry, RT_NULL,
                            1024,
                            RT_THREAD_PRIORITY_MAX-1, 20);
    measure_tid = rt_thread_create( "measure",
                            evse_measure_thread_entry, RT_NULL,
                            1024,
                            RT_THREAD_PRIORITY_MAX-1, 20);

    evse_db_get_hlw_param(VparamXK, IparamXK, PparamXK);
    log_d("VparamXK: %d, %d, %d", VparamXK[0], VparamXK[1], VparamXK[2]);
    log_d("IparamXK: %d, %d, %d", IparamXK[0], IparamXK[1], IparamXK[2]);
    log_d("PparamXK: %d, %d, %d", PparamXK[0], PparamXK[1], PparamXK[2]);

    evse_db_get_calibrated(&hlw_calibrated);

    if(hlw_calibrated == 1){
        log_d("hlw calibrated, start measure thread.");
        rt_thread_startup(measure_tid);
    }else{
        log_d("hlw not calibrated, start calibrate thread.");
        rt_thread_startup(calibrate_tid);
    }

    for(;;){
        rt_thread_mdelay(1000);
    }
}
