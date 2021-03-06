#ifndef STM32_WDT_H
#define STM32_WDT_H

#include "../../userspace/sys.h"

void stm32_wdt_pre_init();
void stm32_wdt_init();

bool stm32_wdt_request(IPC* ipc);


#endif // STM32_WDT_H
