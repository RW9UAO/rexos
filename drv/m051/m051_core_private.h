/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef M051_CORE_PRIVATE_H
#define M051_CORE_PRIVATE_H

#include "m051_config.h"
#include "m051_core.h"
#include "m051_gpio.h"
#include "m051_uart.h"
#include "m051_timer.h"


typedef struct _CORE {
    GPIO_DRV gpio;
    TIMER_DRV timer;
//    POWER_DRV power;
#if (MONOLITH_UART)
    UART_DRV uart;
#endif
#if (MONOLITH_ANALOG)
//    ANALOG_DRV analog;
#endif
}CORE;


#if (SYS_INFO)
#if (UART_STDIO) && (MONOLITH_UART)
#define printd          printu
#else
#define printd          printf
#endif
#endif //SYS_INFO


#endif // M051_CORE_H
