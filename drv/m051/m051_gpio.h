/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef M051_GPIO_H
#define M051_GPIO_H



#define GPIO_COUNT 5 * 8 

#include "m051_core.h"

typedef struct {
    int* used_pins[GPIO_COUNT];
}GPIO_DRV;


typedef enum{
    E_IO_INPUT,
    E_IO_OUTPUT,
    E_IO_OPENDRAIN,
    E_IO_QUASI
} E_DRVGPIO_IO;


void m051_gpio_init(CORE* core);
bool m051_gpio_request(CORE* core, IPC* ipc);

__STATIC_INLINE unsigned int m051_gpio_request_inside(CORE* core, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    IPC ipc;
    ipc.cmd = cmd;
    ipc.param1 = param1;
    ipc.param2 = param2;
    ipc.param3 = param3;
    m051_gpio_request(core, &ipc);
    return ipc.param2;
}


#endif // M051_GPIO_H
