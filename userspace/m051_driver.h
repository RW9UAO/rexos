#ifndef M051_DRIVER_H
#define M051_DRIVER_H

#include "core/core.h"
#include "sys.h"

//------------------------------------------------- GPIO ---------------------------------------------------------------------

typedef enum {
    M051_GPIO_ENABLE_PIN = HAL_IPC(HAL_GPIO),
    M051_GPIO_DISABLE_PIN//,
//    M051_GPIO_ENABLE_EXTI,
//    M051_GPIO_DISABLE_EXTI,
//    M051_GPIO_DISABLE_JTAG
} M051_GPIO_IPCS;

typedef enum {
    P00 = 0, 	P01, P02, P03, P04, P05, P06, P07,
    P10,	P11, P12, P13, P14, P15, P16, P17,
    P20,	P21, P22, P23, P24, P25, P26, P27,
    P30,	P31, P32, P33, P34, P35, P36, P37,
    P40,	P41, P42, P43, P44, P45, P46, P47
} PIN;

#endif