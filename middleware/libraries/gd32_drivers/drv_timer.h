#pragma once

#include <rtthread.h>
#include <board.h>

#if defined(SOC_SERIES_GD32F4xx)
#include "gd32f4xx.h"
#include "gd32f4xx_timer.h"
#elif defined(SOC_SERIES_GD32F3xx)
#include "gd32f3xx.h"
#include "gd32f3xx_timer.h"
#endif

/**
  * @brief  Timer definition
  */
typedef enum {
    DRV_TIMER1 = 0,
    DRV_TIMER2 = 1,
    DRV_TIMER3,
    DRV_TIMER4,
    DRV_TIMER5,
    DRV_TIMER6,
    DRV_TIMER7
}DRV_TIMERxTypedef;

/** 
  * @brief  Timer clock source structures definition  
  */  
typedef enum 
{
    APB1_TIMER,
    APB2_TIMER
}DRV_TimerSourceTypeDef;

uint32_t timer_source_clock_get(uint32_t TIMERx);
