/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef POOL_H
#define POOL_H

#include "../userspace/types.h"
#include "kernel_config.h"

typedef struct {
    void* free_slot;
    void* first_slot;
    void* last_slot;
} POOL;

typedef struct {
    unsigned int free_slots;
    unsigned int used_slots;
    unsigned int free;
    unsigned int used;
    unsigned int largest_free;
} POOL_STAT;

void pool_init(POOL* pool, void* data);
void* pool_malloc(POOL* pool, size_t size);
void* pool_realloc(POOL* pool, void* ptr, size_t size);
void pool_free(POOL* pool, void* ptr);

#if (KERNEL_PROFILING)
bool pool_check(POOL* pool, void *sp);
void pool_stat(POOL* pool, POOL_STAT* stat, void* sp);
#endif

#endif // POOL_H
