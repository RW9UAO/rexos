#ifndef M051_CORE_H
#define M051_CORE_H

#include "../../userspace/sys.h"
#include "../../userspace/process.h"
#include "m051_config.h"
#include "sys_config.h"

typedef struct _CORE CORE;

extern const REX __M051_CORE;

__STATIC_INLINE unsigned int m051_core_request_outside(void* unused, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    return get(object_get(SYS_OBJ_CORE), cmd, param1, param2, param3);
}


#endif // M051_CORE_H
