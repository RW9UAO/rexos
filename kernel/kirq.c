/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "kirq.h"
#include "kernel.h"
#include "kmalloc.h"
#include "../userspace/error.h"

void kirq_stub(int vector, void* param)
{
#if (KERNEL_INFO)
    printk("Warning: irq vector %d stub called\n\r", vector);
#endif
    kprocess_error_current(ERROR_STUB_CALLED);
}

typedef KIRQ* KIRQ_P;
static const KIRQ __KIRQ_STUB ={kirq_stub, NULL, NULL};

void kirq_init()
{
    int i;
    for (i = 0; i < IRQ_VECTORS_COUNT; ++i)
    {
        __KERNEL->irqs[i] = (KIRQ_P)&__KIRQ_STUB;
    }
    __KERNEL->context = -1;
#ifdef SOFT_NVIC
    rb_init(&__KERNEL->irq_pend_rb, IRQ_VECTORS_COUNT);
    for (i = 0; i < (IRQ_VECTORS_COUNT + 7) / 8; ++i)
        __KERNEL->irq_pend_mask[i] = 0;
#endif
}

void kirq_enter(int vector)
{
#ifdef SOFT_NVIC
    int pending;
    //already pending. exit.
    if (__KERNEL->irq_pend_mask[vector / 8] & (1 << (vector & 7)))
        return;
    disable_interrupts();
    //pend and exit
    if ((pending = irq_pend_list_size++) == 0)
    {
        __KERNEL->irq_pend_mask[vector / 8] |= 1 << (vector & 7);
        __KERNEL->irq_pend_list[rb_put(&__KERNEL->irq_pend_rb)] = vector;
    }
    enable_interrupts();
    if (pending++)
        return;
#endif
    register int saved_context = __KERNEL->context;
#ifdef SOFT_NVIC
    while (pending--)
    {
#endif
        __KERNEL->context = vector;
        __KERNEL->irqs[vector]->handler(vector, __KERNEL->irqs[vector]->param);
#ifdef SOFT_NVIC
        if (pending)
        {
            disable_interrupts();
            --irq_pend_list_size;
            __KERNEL->irq_pend_mask[vector / 8] &= ~(1 << (vector & 7));
            vector = __KERNEL->irq_pend_list[rb_get(&__KERNEL->irq_pend_rb)];
            enable_interrupts();
        }
    }
#endif
    __KERNEL->context = saved_context;
}

void kirq_register(int vector, IRQ handler, void* param)
{
    if (vector >= IRQ_VECTORS_COUNT)
    {
        kprocess_error_current(ERROR_OUT_OF_RANGE);
        return;
    }
    if (__KERNEL->irqs[vector] != (KIRQ_P)&__KIRQ_STUB)
    {
        kprocess_error_current(ERROR_ALREADY_CONFIGURED);
        return;
    }
    __KERNEL->irqs[vector] = kmalloc(sizeof(KIRQ));
    if (__KERNEL->irqs[vector] == NULL)
    {
        __KERNEL->irqs[vector] = (KIRQ_P)&__KIRQ_STUB;
        kprocess_error_current(ERROR_OUT_OF_SYSTEM_MEMORY);
        return;
    }
    __KERNEL->irqs[vector]->handler = handler;
    __KERNEL->irqs[vector]->param = param;
    __KERNEL->irqs[vector]->process = kprocess_get_current();
}

void kirq_unregister(int vector)
{
    if (__KERNEL->irqs[vector]->process == kprocess_get_current())
    {
        kfree(__KERNEL->irqs[vector]);
        __KERNEL->irqs[vector] = (KIRQ_P)&__KIRQ_STUB;
    }
    else
        kprocess_error_current(ERROR_ACCESS_DENIED);
}
