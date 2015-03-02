#include "DrvSYS.h"
#include "m051_timer.h"
#include "m051_core_private.h"
#include "m051_driver.h"
#include "../../userspace/error.h"
#include "../../userspace/timer.h"
#include "../../userspace/irq.h"
#include <string.h>
#if (SYS_INFO)
#include "../../userspace/stdio.h"
#endif

const uint32_t CH_OFFSET[] = {0x0, 0x20, 0x100000, 0x100020};
const int TIMER_VECTORS[/*TIMERS_COUNT*/ 4 ] = {TMR0_IRQn, TMR1_IRQn, TMR2_IRQn, TMR3_IRQn};
TIMER_T * TIMER_REGS[/*TIMERS_COUNT*/ 4] =  {TIMER0, TIMER1, TIMER2, TIMER3};

void m051_timer_enable(CORE *core, TIMER_NUM num, unsigned int flags){
    unsigned int mode;
    if (num >= TIMERS_COUNT)    {
        error(ERROR_NOT_SUPPORTED);
        return;
    }
    if (flags & TIMER_FLAG_ONE_PULSE_MODE){
	mode = E_ONESHOT_MODE;
    }else{
	mode = E_PERIODIC_MODE;
    }
    switch(num){
    case TIM_0:        				//setup timer1
	SYSCLK->APBCLK.TMR0_EN = 1;
        SYSCLK->CLKSEL1.TMR0_S = 2;		// HCLK (50 mhz)
        outpw((uint32_t)&TIMER0->TCSR, 0);      // disable timer
	break;
    case TIM_1:        				//setup timer1
	SYSCLK->APBCLK.TMR1_EN = 1;
        SYSCLK->CLKSEL1.TMR1_S = 2;
        outpw((uint32_t)&TIMER1->TCSR, 0);      // disable timer
	break;
    case TIM_2:        				//setup timer1
	SYSCLK->APBCLK.TMR2_EN = 1;
        SYSCLK->CLKSEL1.TMR2_S = 2;
        outpw((uint32_t)&TIMER2->TCSR, 0);      // disable timer
	break;
    case TIM_3:        				//setup timer1
	SYSCLK->APBCLK.TMR3_EN = 1;
        SYSCLK->CLKSEL1.TMR3_S = 2;
        outpw((uint32_t)&TIMER3->TCSR, 0);      // disable timer
	break;
	default:
            error(ERROR_NOT_SUPPORTED);
	    return;
	break;
    }
    //only for timers, not for PWM
        TIMER_REGS[num]->TISR.TIF = 1;        		// write 1 to clear for safety
        TIMER_REGS[num]->TCSR.MODE = mode;
        TIMER_REGS[num]->TCSR.IE = 1;

    NVIC_SetPriority((IRQn_Type)((uint32_t)TMR0_IRQn + (uint32_t)num), (1<<__NVIC_PRIO_BITS) - 2);
    NVIC_EnableIRQ((IRQn_Type)((uint32_t)TMR0_IRQn + (uint32_t)num));
}
//------------------------------------------------------------------------------
void m051_timer_disable(CORE *core, TIMER_NUM num){
    if (num >= TIMERS_COUNT)    {
        error(ERROR_NOT_SUPPORTED);
        return;
    }
    switch(num){
        case TIM_0:
	    SYSCLK->APBCLK.TMR0_EN = 0;
    	    outpw((uint32_t)&TIMER0->TCSR, 0);      // disable timer
	break;
        case TIM_1:
	    SYSCLK->APBCLK.TMR1_EN = 0;
    	    outpw((uint32_t)&TIMER1->TCSR, 0);      // disable timer
	break;
        case TIM_2:
	    SYSCLK->APBCLK.TMR2_EN = 0;
    	    outpw((uint32_t)&TIMER2->TCSR, 0);      // disable timer
	break;
        case TIM_3:
	    SYSCLK->APBCLK.TMR3_EN = 0;
    	    outpw((uint32_t)&TIMER3->TCSR, 0);      // disable timer
	break;
	default:
            error(ERROR_NOT_SUPPORTED);
	    return;
	break;
    }
    TIMER_REGS[num]->TCSR.IE = 0;
    NVIC_DisableIRQ((IRQn_Type)((uint32_t)TMR0_IRQn + (uint32_t)num));
}
//--------------------------------------------------------------------------------
void m051_timer_setup_hz(CORE* core, TIMER_NUM num, unsigned int hz){
    unsigned int u32ClockValue, u32PreScale, u32TCMPRValue;

    if (num >= TIMERS_COUNT)    {
        error(ERROR_NOT_SUPPORTED);
        return;
    }

    u32ClockValue = 50000000;//DrvSYS_GetHCLKFreq();	hangup here. WTF???
//    u32ClockValue = __IRC22M;

//    printk("timer_setup_hz: u32ClockValue %d\r\n", u32ClockValue);

    for (u32PreScale = 1; u32PreScale < 256; u32PreScale++){
        u32TCMPRValue = u32ClockValue / (hz * u32PreScale);
        if ((u32TCMPRValue > 1) && (u32TCMPRValue < 0x1000000)){        // The TCMPR value must > 1 
	TIMER_REGS[num]->TCMPR = u32TCMPRValue;
	TIMER_REGS[num]->TCSR.PRESCALE = u32PreScale - 1;
	return;
	}
    }
}
//---------------------------------------------------------------------------------
void m051_timer_start(TIMER_NUM num){
	TIMER_T * tTMR;
    if (num >= TIMERS_COUNT)    {
        error(ERROR_NOT_SUPPORTED);
        return;
    }
    if(num < 4){
    	tTMR = (TIMER_T *)((uint32_t)TIMER0 + CH_OFFSET[num]);
        tTMR->TCSR.CEN = 1;
    }
}
//-------------------------------------------------------
void m051_timer_stop(TIMER_NUM num){
	TIMER_T * tTMR;
    if (num >= TIMERS_COUNT)    {
        error(ERROR_NOT_SUPPORTED);
        return;
    }
    if(num < 4){
    	tTMR = (TIMER_T *)((uint32_t)TIMER0 + CH_OFFSET[num]);
        tTMR->TCSR.CEN = 0;
    }
}
//-------------------------------------------------------------
void hpet_isr(int vector, void* param){
    TIMER_REGS[HPET_TIMER]->TISR.TIF = 1;
    timer_hpet_timeout();
}
//-----------------------------------------------------------
// value in us
void hpet_start(unsigned int value, void* param){
    unsigned int u32ClockValue;//, u32TCMPRValue;
    unsigned char u32PreScale;

    u32ClockValue = DrvSYS_GetHCLKFreq();
    u32PreScale = u32ClockValue / 1000000;
//    u32TCMPRValue = value;
//printk("hpet_start: value %d, prescaler %d, TCMP %d\r\n", value, u32PreScale, u32TCMPRValue);
	TIMER_REGS[HPET_TIMER]->TCSR.CRST = 1;
	TIMER_REGS[HPET_TIMER]->TCSR.TDR_EN = 1;

    	TIMER_REGS[HPET_TIMER]->TCMPR = value;
    	TIMER_REGS[HPET_TIMER]->TCSR.PRESCALE = u32PreScale - 1;
        TIMER_REGS[HPET_TIMER]->TCSR.CEN = 1;
}
//--------------------------------------------------------
void hpet_stop(void* param){
    m051_timer_stop(HPET_TIMER);
}
//------------------------------------------------------
unsigned int hpet_elapsed(void* param){
    unsigned int u32TCMPRValue;
//    unsigned int u32ClockValue;
//    unsigned char u32PreScale;

//    u32ClockValue = DrvSYS_GetHCLKFreq();
//    u32PreScale   = TIMER_REGS[HPET_TIMER]->TCSR.PRESCALE + 1;
    u32TCMPRValue = TIMER_REGS[HPET_TIMER]->TDR;

//printk("hpet: TDR %d\r\n", u32TCMPRValue);

    return u32TCMPRValue;
}
//----------------------------------------------------------------------
//#if (TIMER_SOFT_RTC)
void second_pulse_isr(int vector, void* param){
    if(vector == TIMER_VECTORS[0]){
        TIMER0->TISR.TIF = 1;
    }
    if(vector == TIMER_VECTORS[1]){
        TIMER1->TISR.TIF = 1;
    }
    if(vector == TIMER_VECTORS[2]){
        TIMER2->TISR.TIF = 1;
    }
    if(vector == TIMER_VECTORS[3]){
        TIMER3->TISR.TIF = 1;
    }
    timer_second_pulse();
}
//#endif
//---------------------------------------------------------------------------------
#if (SYS_INFO)
void m051_timer_info(){
    printd("HPET timer: TIM_%d\n\r", HPET_TIMER);
#if (TIMER_SOFT_RTC)
    printd("Second pulse timer: TIM_%d\n\r", SECOND_PULSE_TIMER);
#endif
}
#endif
//----------------------------------------------------------------------------------
void m051_timer_init(CORE *core){
    //setup HPET
    irq_register(TIMER_VECTORS[HPET_TIMER], hpet_isr, (void*)core);
//    core->timer.hpet_uspsc = DrvSYS_GetHCLKFreq() / 1000000;
//    core->timer.hpet_uspsc = __IRC22M / 1000000;
    m051_timer_enable(core, HPET_TIMER, TIMER_FLAG_ONE_PULSE_MODE /*| TIMER_FLAG_ENABLE_IRQ | (13 << TIMER_FLAG_PRIORITY)*/);
    CB_SVC_TIMER cb_svc_timer;
    cb_svc_timer.start = hpet_start;
    cb_svc_timer.stop = hpet_stop;
    cb_svc_timer.elapsed = hpet_elapsed;
    timer_setup(&cb_svc_timer, core);
//#if (TIMER_SOFT_RTC)
    irq_register(TIMER_VECTORS[SECOND_PULSE_TIMER], second_pulse_isr, (void*)core);
    m051_timer_enable(core, SECOND_PULSE_TIMER, /*TIMER_FLAG_ENABLE_IRQ | (13 << TIMER_FLAG_PRIORITY)*/ 0);
    m051_timer_setup_hz(core, SECOND_PULSE_TIMER, 1);
    m051_timer_start(SECOND_PULSE_TIMER);
//#endif
}
//-----------------------------------------------------------------------------
bool m051_timer_request(CORE* core, IPC* ipc){
    bool need_post = false;
    switch (ipc->cmd)    {
#if (SYS_INFO)
    case IPC_GET_INFO:
        m051_timer_info();
        need_post = true;
        break;
#endif
    case M051_TIMER_ENABLE:
        m051_timer_enable(core, (TIMER_NUM)ipc->param1, ipc->param2);
        need_post = true;
        break;
    case M051_TIMER_DISABLE:
        m051_timer_disable(core, (TIMER_NUM)ipc->param1);
        need_post = true;
        break;
    case M051_TIMER_ENABLE_EXT_CLOCK:
//        M051_timer_enable_ext_clock(core, (TIMER_NUM)ipc->param1, (PIN)ipc->param2, ipc->param3);
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    case M051_TIMER_DISABLE_EXT_CLOCK:
//        M051_timer_disable_ext_clock(core, (TIMER_NUM)ipc->param1, (PIN)ipc->param2);
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    case M051_TIMER_SETUP_HZ:
        m051_timer_setup_hz(core, (TIMER_NUM)ipc->param1, ipc->param2);
        need_post = true;
        break;
    case M051_TIMER_START:
        m051_timer_start((TIMER_NUM)ipc->param1);
        need_post = true;
        break;
    case M051_TIMER_STOP:
        m051_timer_stop(ipc->param1);
        need_post = true;
        break;
    case M051_TIMER_GET_CLOCK:
        ipc->param2 = DrvSYS_GetHCLKFreq();
//	ipc->param2 = __IRC22M;
        need_post = true;
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
    return need_post;
}
