/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_CORE_H
#define LPC_CORE_H

#include "../../userspace/sys.h"
#include "../../userspace/process.h"
#include "lpc_config.h"
#include "sys_config.h"

typedef struct _CORE CORE;

extern const REX __LPC_CORE;

__STATIC_INLINE unsigned int lpc_core_request_outside(void* unused, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    return get(object_get(SYS_OBJ_CORE), cmd, param1, param2, param3);
}


#endif // LPC_CORE_H
