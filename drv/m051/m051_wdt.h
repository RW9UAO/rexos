#ifndef STM32_WDT_H
#define STM32_WDT_H

#include "../../userspace/sys.h"

void m051_wdt_init();

bool m051_wdt_request(IPC* ipc);


#endif // STM32_WDT_H
