/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_POWER_H
#define STM32_POWER_H

#include "stm32_core.h"

#if defined(STM32F10X_CL)
#define PLL_MUL_6_5                             15
#define PLL2_MUL_20                             15
#endif

typedef enum {
    STM32_POWER_GET_CLOCK = HAL_IPC(HAL_POWER),
    STM32_POWER_UPDATE_CLOCK,
    STM32_POWER_GET_RESET_REASON,
    STM32_POWER_DMA_ON,
    STM32_POWER_DMA_OFF,
    STM32_POWER_USB_ON,
    STM32_POWER_USB_OFF
} STM32_POWER_IPCS;

typedef enum {
    STM32_CLOCK_CORE,
    STM32_CLOCK_AHB,
    STM32_CLOCK_APB1,
    STM32_CLOCK_APB2,
    STM32_CLOCK_ADC
} STM32_POWER_CLOCKS;

typedef enum {
    DMA_1 = 0,
    DMA_2
} STM32_DMA;

typedef enum {
    RESET_REASON_UNKNOWN    = 0,
    RESET_REASON_LOW_POWER,
    RESET_REASON_WATCHDOG,
    RESET_REASON_SOFTWARE,
    RESET_REASON_POWERON,
    RESET_REASON_PIN_RST
} RESET_REASON;

typedef struct {
    int write_count;
    RESET_REASON reset_reason;
#if defined(STM32F1)
    int dma_count[2];
#endif
}POWER_DRV;

void stm32_power_init(CORE* core);
bool stm32_power_request(CORE* core, IPC* ipc);

__STATIC_INLINE unsigned int stm32_power_request_inside(CORE* core, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    IPC ipc;
    ipc.cmd = cmd;
    ipc.param1 = param1;
    ipc.param2 = param2;
    ipc.param3 = param3;
    stm32_power_request(core, &ipc);
    return ipc.param2;
}

__STATIC_INLINE unsigned int stm32_power_get_clock_outside(void* unused, STM32_POWER_CLOCKS type)
{
    return stm32_core_request_outside(unused, STM32_POWER_GET_CLOCK, type, 0, 0);
}

__STATIC_INLINE unsigned int stm32_power_get_clock_inside(CORE* core, STM32_POWER_CLOCKS type)
{
    IPC ipc;
    ipc.cmd = STM32_POWER_GET_CLOCK;
    ipc.param1 = (unsigned int)type;
    stm32_power_request(core, &ipc);
    return ipc.param2;
}

#endif // STM32_POWER_H
