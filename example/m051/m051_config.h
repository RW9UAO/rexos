/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef M051_CONFIG_H
#define M051_CONFIG_H

//------------------------------ CORE ------------------------------------------------
//Sizeof CORE process stack. Adjust, if monolith UART/USB/Analog/etc is used
#define M051_CORE_STACK_SIZE                   600
#define M051_DRIVERS_IPC_COUNT                 3

//driver is monolith. Enable for size, disable for perfomance
#define MONOLITH_UART                           1
#define MONOLITH_ANALOG                         1
//#define MONOLITH_USB                            1
//------------------------------ POWER -----------------------------------------------
//------------------------------ UART ------------------------------------------------
//Use UART as default stdio
#define UART_STDIO                              1
//PIN_DEFAULT and PIN_UNUSED can be also set.
#define UART_STDIO_PORT                         UART_0
#define UART_STDIO_BAUD                         115200
#define UART_STDIO_DATA_BITS                    8
#define UART_STDIO_PARITY                       'N'
#define UART_STDIO_STOP_BITS                    1

//size of every uart internal tx buf. Increasing this you will get less irq and ipc calls, but faster processing
#define UART_TX_BUF_SIZE                        16
//Sizeof UART process stack. Remember, that process itself requires around 512 bytes
#define M051_UART_STACK_SIZE                   440
//------------------------------ TIMER -----------------------------------------------
#define HPET_TIMER                              TIM_0
#define TIMER_SOFT_RTC                          1
#define SECOND_PULSE_TIMER                      TIM_1
//----------------------------- ANALOG -----------------------------------------------
//------------------------------- WDT ------------------------------------------------
#define HARDWARE_WATCHDOG                       1
//WDT module enable
#define M051_WDT                               1


#endif // M051_CONFIG_H
