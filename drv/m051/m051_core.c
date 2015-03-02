#include "DrvSYS.h"
#include "m051_core.h"
#include "m051_core_private.h"
#include "m051_gpio.h"
#include "m051_timer.h"
#if (MONOLITH_UART)
#include "m051_uart.h"
#endif
#if (M051_WDT)
#include "m051_wdt.h"
#endif

#include "../../userspace/object.h"

void m051_core();

const REX __M051_CORE = {
    //name
    "M051 core driver",
    //size
    M051_CORE_STACK_SIZE,
    //priority - driver priority
    90,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_HEAP_FLAGS(HEAP_PERSISTENT_NAME),
    //ipc size
    M051_DRIVERS_IPC_COUNT,
    //function
    m051_core
};

void m051_core_loop(CORE* core){
    IPC ipc;
    bool need_post;
    int group;
    for (;;)    {

        error(ERROR_OK);
        need_post = false;
        group = -1;
        ipc.cmd = 0;
        ipc_read_ms(&ipc, 0, ANY_HANDLE);
//	printk("m051_core_loop cmd: %#X\n\r", ipc.cmd);

        if (ipc.cmd < IPC_USER)        {
            switch (ipc.cmd)            {
            case IPC_PING:
                need_post = true;
                break;
            case IPC_SET_STDIO:
                open_stdout();
                need_post = true;
                break;
            case IPC_OPEN:
            case IPC_CLOSE:
            case IPC_FLUSH:
            case IPC_READ:
            case IPC_WRITE:
            case IPC_GET_TX_STREAM:
            case IPC_GET_RX_STREAM:
            case IPC_STREAM_WRITE:
                group = HAL_GROUP(ipc.param1);
                break;
            default:
                error(ERROR_NOT_SUPPORTED);
                need_post = true;
            }
        }
        else
            group = HAL_IPC_GROUP(ipc.cmd);
        if (group >= 0)        {
            switch (group)            {
            case HAL_GPIO:
                need_post = m051_gpio_request(core, &ipc);
                break;
            case HAL_TIMER:
                need_post = m051_timer_request(core, &ipc);
                break;
#if (MONOLITH_UART)
            case HAL_UART:
                need_post = m051_uart_request(core, &ipc);
                break;
#endif //MONOLITH_UART
#if (M051_WDT)
            case HAL_WDT:
                need_post = m051_wdt_request(&ipc);
                break;
#endif //STM32_WDT
            default:
                error(ERROR_NOT_SUPPORTED);
                need_post = true;
                break;
            }
        }
        if (need_post)
            ipc_post_or_error(&ipc);
    }
}

void m051_core(){
    CORE core;
    object_set_self(SYS_OBJ_CORE);

    // start PLL
    UNLOCKREG();
    SYSCLK->PLLCON.PLL_SRC = 1;	// internal 22.1184 MHz
    DrvSYS_Open(50000000); // core clock
    LOCKREG();

    m051_timer_init(&core);
    m051_gpio_init(&core);
//#if (MONOLITH_UART)
    m051_uart_init(&core);
//#endif

#if (M051_WDT)
    m051_wdt_init();
#endif

    m051_core_loop(&core);
}
