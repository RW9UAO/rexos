/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "../../userspace/core/core.h"
#include "karm7.h"

void prefetch_abort_entry_arm7(unsigned int address)
{
#if (KERNEL_INFO)
    printk("PREFETCH ABORT AT: %#X\n\r", address);
#endif
    panic();
}

void data_abort_entry_arm7(unsigned int address)
{
#if (KERNEL_INFO)
    printk("DATA ABORT AT: %#X\n\r", address);
#endif
    panic();
}

