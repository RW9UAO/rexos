/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_TIMER_H
#define LPC_TIMER_H

#include "lpc_core.h"
#include "../../userspace/lpc_driver.h"

typedef struct {
    unsigned int hpet_start;
    uint8_t main_channel[TIMER_MAX];
} TIMER_DRV;

void lpc_timer_init(CORE* core);
bool lpc_timer_request(CORE* core, IPC* ipc);


#endif // LPC_TIMER_H
