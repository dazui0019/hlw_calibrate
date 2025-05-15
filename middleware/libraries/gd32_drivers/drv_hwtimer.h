#ifndef __DRV_HWTIMER_H__
#define __DRV_HWTIMER_H__

#ifdef __cplusplus
extern "C" {
#endif

#if defined SOC_SERIES_GD32F10x
#include "gd32f10x_timer.h"
#elif defined SOC_SERIES_GD32F20x
#include "gd32f20x_timer.h"
#elif defined SOC_SERIES_GD32F30x
#include "gd32f30x_timer.h"
#elif defined SOC_SERIES_GD32F4xx
#include "gd32f4xx_timer.h"
#elif defined SOC_SERIES_GD32H7xx
#include "gd32h7xx_timer.h"
#elif defined SOC_SERIES_GD32E50x
#include "gd32e50x_timer.h"
#endif

#ifdef __cplusplus
}
#endif

#endif /* __DRV_HWTIMER_H__ */