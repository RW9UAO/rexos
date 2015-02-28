#ifndef M051_TIMER_H
#define M051_TIMER_H

#include "m051_core.h"
#include "../../userspace/gpio.h"

#define TIMER_FLAG_ONE_PULSE_MODE                    (1 << 4)


typedef enum{                   
    E_ONESHOT_MODE      = 0,
    E_PERIODIC_MODE       = 1,
    E_TOGGLE_MODE         = 2, 
} E_TIMER_OPMODE ;


typedef enum {
    //timer
    M051_TIMER_ENABLE = HAL_IPC(HAL_TIMER),
    M051_TIMER_DISABLE,
    M051_TIMER_ENABLE_EXT_CLOCK,
    M051_TIMER_DISABLE_EXT_CLOCK,
    M051_TIMER_SETUP_HZ,
    M051_TIMER_START,
    M051_TIMER_STOP,
    M051_TIMER_GET_CLOCK,
} M051_TIMER_IPCS;

typedef enum {
    TIM_0 = 0,
    TIM_1,
    TIM_2,
    TIM_3/*,
//PWMs as timer
    TIM_4,
    TIM_5,
    TIM_6,
    TIM_7*/
}TIMER_NUM;

typedef struct {
    //timer specific
    int hpet_uspsc;
}TIMER_DRV;

extern const int TIMER_VECTORS[];

void m051_timer_init(CORE* core);
bool m051_timer_request(CORE* core, IPC* ipc);

__STATIC_INLINE unsigned int m051_timer_request_inside(CORE* core, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    IPC ipc;
    ipc.cmd = cmd;
    ipc.param1 = param1;
    ipc.param2 = param2;
    ipc.param3 = param3;
    m051_timer_request(core, &ipc);
    return ipc.param2;
}

#endif