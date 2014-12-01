/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "lpc_core.h"
#include "lpc_core_private.h"
//#include "lpc_timer.h"
#include "lpc_gpio.h"
//#include "lpc_power.h"
#include "../../../userspace/object.h"
#if (MONOLITH_UART)
#include "lpc_uart.h"
#endif

void lpc_core();

const REX __LPC_CORE = {
    //name
    "LPC core driver",
    //size
    LPC_CORE_STACK_SIZE,
    //priority - driver priority
    90,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_HEAP_FLAGS(HEAP_PERSISTENT_NAME),
    //ipc size
    LPC_DRIVERS_IPC_COUNT,
    //function
    lpc_core
};

void lpc_core_loop(CORE* core)
{
    IPC ipc;
    bool need_post;
    int group;
    for (;;)
    {
        error(ERROR_OK);
        need_post = false;
        group = -1;
        ipc.cmd = 0;
        ipc_read_ms(&ipc, 0, 0);
        //TODO: remove after timer setup
        error(ERROR_OK);

        if (ipc.cmd < IPC_USER)
        {
            switch (ipc.cmd)
            {
            case IPC_PING:
                need_post = true;
                break;
            case IPC_CALL_ERROR:
                break;
            case IPC_SET_STDIO:
                open_stdout();
                need_post = true;
                break;
#if (SYS_INFO)
            case IPC_GET_INFO:
                need_post |= lpc_gpio_request(&ipc);
                need_post |= lpc_timer_request(core, &ipc);
                need_post |= lpc_power_request(core, &ipc);
#if (MONOLITH_UART)
                need_post |= lpc_uart_request(core, &ipc);
#endif
                break;
#endif
            case IPC_OPEN:
            case IPC_CLOSE:
            case IPC_FLUSH:
            case IPC_READ:
            case IPC_WRITE:
            case IPC_GET_TX_STREAM:
            case IPC_GET_RX_STREAM:
                group = HAL_GROUP(ipc.param1);
                break;
            case IPC_STREAM_WRITE:
                group = HAL_GROUP(ipc.param3);
                break;
            default:
                ipc_set_error(&ipc, ERROR_NOT_SUPPORTED);
                need_post = true;
            }
        }
        else
            group = HAL_IPC_GROUP(ipc.cmd);
        if (group >= 0)
        {
            switch (group)
            {
            case HAL_POWER:
                need_post = lpc_power_request(core, &ipc);
                break;
            case HAL_GPIO:
                need_post = lpc_gpio_request(&ipc);
                break;
            case HAL_TIMER:
                need_post = lpc_timer_request(core, &ipc);
                break;
#if (MONOLITH_UART)
            case HAL_UART:
                need_post = lpc_uart_request(core, &ipc);
                break;
#endif //MONOLITH_UART
            default:
                ipc_set_error(&ipc, ERROR_NOT_SUPPORTED);
                need_post = true;
                break;
            }
        }
        if (need_post)
            ipc_post_or_error(&ipc);
    }
}

void lpc_core()
{
    CORE core;
    object_set_self(SYS_OBJ_CORE);

    lpc_power_init(&core);
    lpc_gpio_init(&core);
    lpc_timer_init(&core);
#if (MONOLITH_UART)
    lpc_uart_init(&core);
#endif

    lpc_core_loop(&core);
}
