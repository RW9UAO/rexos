/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef M051_H
#define M051_H

#if !defined(LDS) && !defined(__ASSEMBLER__)
#include "M051Series.h"
#endif


#define FLASH_SIZE        0x10000


#define IRQ_VECTORS_COUNT   32

#define SRAM_SIZE        0x1000

#define UARTS_COUNT                                             2
#define TIMERS_COUNT						4

#define M051

#ifndef CORTEX_M
#define CORTEX_M
#endif

#ifndef CORTEX_M0
#define CORTEX_M0
#endif


#if defined(M051)
#ifndef FLASH_BASE
#define FLASH_BASE                0x00000000
#endif
#endif


#endif //STM32_H
