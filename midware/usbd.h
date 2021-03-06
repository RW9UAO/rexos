/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef USBD_H
#define USBD_H

#include "../userspace/process.h"
#include "../userspace/usb.h"
#include "sys_config.h"

typedef struct _USBD USBD;

typedef struct {
    void (*usbd_class_configured)(USBD*, USB_CONFIGURATION_DESCRIPTOR_TYPE*);
    void (*usbd_class_reset)(USBD*, void*);
    void (*usbd_class_suspend)(USBD*, void*);
    void (*usbd_class_resume)(USBD*, void*);
    int (*usbd_class_setup)(USBD*, void*, SETUP*, HANDLE);
    bool (*usbd_class_request)(USBD*, void*, IPC*);
} USBD_CLASS;

extern const REX __USBD;

bool usbd_register_interface(USBD* usbd, unsigned int iface, const USBD_CLASS* usbd_class, void* param);
bool usbd_unregister_interface(USBD* usbd, unsigned int iface, const USBD_CLASS* usbd_class);
bool usbd_register_endpoint(USBD* usbd, unsigned int iface, unsigned int num);
bool usbd_unregister_endpoint(USBD* usbd, unsigned int iface, unsigned int num);
//post IPC to user, if configured
void usbd_post_user(USBD* usbd, unsigned int iface, unsigned int cmd, unsigned int param);
HANDLE usbd_user(USBD* usbd);
HANDLE usbd_usb(USBD* usbd);
#if (USBD_DEBUG)
void usbd_dump(const uint8_t* buf, unsigned int size, const char* header);
#endif

#endif // USBD_H
