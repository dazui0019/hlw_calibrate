#include "drv_timer.h"

#if defined(SOC_SERIES_GD32F4xx)
/*!
    \brief      计算 CK_TIMERx 对于 CK_APBx 的倍频系数
    \param[in]  APBx: 指定APB1或者APB2
      \arg      APB1_TIMER
      \arg      APB2_TIMER
    \param[out] none
    \retval     倍频系数
*/
uint8_t timer_clk_pll_get(DRV_TimerSourceTypeDef APBx_TIMER)
{
    uint8_t psc = 0b0000;

    if(APBx_TIMER == APB1_TIMER)        {psc = (RCU_CFG0&RCU_CFG0_APB1PSC)>>10;}               // 获取APBxPSC的值
    else if(APBx_TIMER == APB2_TIMER)   {psc = (RCU_CFG0&RCU_CFG0_APB2PSC)>>13;}
    else                                {return 0;} // ERROR, 非法参数

    if(0x00000000U == (RCU_CFG1&RCU_CFG1_TIMERSEL)){    // 获取TIMERSEL位
        switch (psc)
        {
            case 0b100:
            case 0b000:
            case 0b001:
            case 0b010:
            case 0b011:
                return 1;
            default: /* 111 110 101 */
                return 2;
        }
    }else{
        switch (psc)
        {
            case 0b100:
            case 0b101:
            case 0b000:
            case 0b001:
            case 0b010:
            case 0b011:
                return 1;
            default: /*111、110*/
                return 4;
        }
    }
}

/*!
    \brief      获取 CK_TIMERx
    \param[in]  TIMERx
      \arg      TIEMRx (x = 0...13)
    \param[out] none
    \retval     CK_TIMERx 的频率
*/
uint32_t timer_source_clock_get(uint32_t TIMERx)
{
    uint8_t temp;
    switch (TIMERx)
    {
    case TIMER1:
    case TIMER2:
    case TIMER3:
    case TIMER4:
    case TIMER5:
    case TIMER6:
    case TIMER11:
    case TIMER12:
    case TIMER13:
        temp = timer_clk_pll_get(APB1_TIMER);
        return (temp == 1) ? rcu_clock_freq_get(CK_AHB) : (temp*rcu_clock_freq_get(CK_APB1));
    case TIMER0:
    case TIMER7:
    case TIMER8:
    case TIMER9:
    case TIMER10:
        temp = timer_clk_pll_get(APB2_TIMER);
        return (temp == 1) ? rcu_clock_freq_get(CK_AHB) : (temp*rcu_clock_freq_get(CK_APB2));
    }
    return 0; // ERROR
}
#endif

#if defined(SOC_SERIES_GD32F3xx)
/**
 * @brief       计算定时器时钟源对于对应的APBx时钟的倍频系数
 * @note        定时器的CK_TIMER是不固定的, 某几个寄存器里的值不同就会发生改变
 * @param[in]   APBx(DRV_TimerSourceTypeDef): 指定APB1或者APB2
 * @retval      倍频系数
*/
uint8_t timer_clk_pll_get(DRV_TimerSourceTypeDef APBx_TIMER)
{
    uint8_t psc;
    if(APBx_TIMER == APB1_TIMER)        {psc = (RCU_CFG0&RCU_CFG0_APB1PSC)>>8;}    // 获取APBxPSC的值
    else if(APBx_TIMER == APB2_TIMER)   {psc = (RCU_CFG0&RCU_CFG0_APB2PSC)>>11;}
    else                                {return 0; /* ERROR: 非法参数*/ }
    if(psc < 0b100)
        return 1;
    else
        return 2;
}

/**
 * @brief       获取指定定时器的输入时钟源
 * @note        定时器的CK_TIMER是不固定的, 某几个寄存器里的值不同就会发生改变
 * @param[in]   TIMERx(x = 0...13)
 * @retval  定时器的输入时钟源频率
*/
uint32_t timer_source_clock_get(uint32_t TIMERx)
{
    switch (TIMERx){
    case TIMER1:
    case TIMER2:
    case TIMER3:
    case TIMER4:
    case TIMER5:
    case TIMER6:
    case TIMER11:
    case TIMER12:
    case TIMER13:
        return (rcu_clock_freq_get(CK_APB1)*timer_clk_pll_get(APB1_TIMER));
    case TIMER0:
    case TIMER7:
    case TIMER8:
    case TIMER9:
    case TIMER10:
        return (rcu_clock_freq_get(CK_APB2)*timer_clk_pll_get(APB2_TIMER));
    }
    return 0; // ERROR
}
#endif