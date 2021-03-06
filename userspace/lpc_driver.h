/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_DRIVER_H
#define LPC_DRIVER_H

#include "sys.h"
#include "uart.h"

//------------------------------------------------- GPIO ---------------------------------------------------------------------
typedef enum {
    LPC_GPIO_ENABLE_PIN = HAL_IPC(HAL_GPIO),
    LPC_GPIO_DISABLE_PIN,
} LPC_GPIO_IPCS;

typedef enum {
    PIO0_0 = 0, PIO0_1,  PIO0_2,  PIO0_3,  PIO0_4,  PIO0_5,  PIO0_6,  PIO0_7,
    PIO0_8,     PIO0_9,  PIO0_10, PIO0_11, PIO0_12, PIO0_13, PIO0_14, PIO0_15,
    PIO0_16,    PIO0_17, PIO0_18, PIO0_19, PIO0_20, PIO0_21, PIO0_22, PIO0_23,
    PIO0_24,    PIO0_25, PIO0_26, PIO0_27, PIO0_28, PIO0_29, PIO0_30, PIO0_31,
    PIO1_0,     PIO1_1,  PIO1_2,  PIO1_3,  PIO1_4,  PIO1_5,  PIO1_6,  PIO1_7,
    PIO1_8,     PIO1_9,  PIO1_10, PIO1_11, PIO1_12, PIO1_13, PIO1_14, PIO1_15,
    PIO1_16,    PIO1_17, PIO1_18, PIO1_19, PIO1_20, PIO1_21, PIO1_22, PIO1_23,
    PIO1_24,    PIO1_25, PIO1_26, PIO1_27, PIO1_28, PIO1_29, PIO1_30, PIO1_31,
    //only for service calls
    PIN_DEFAULT,
    PIN_UNUSED
} PIN;

#define LPC_GPIO_MODE_MASK                      0x7ff
#define LPC_GPIO_MODE_OUT                       (1 << 11)

//Ignore AF, value is set in mode
#define AF_IGNORE                               0
//PIN as GPIO
#define AF_DEFAULT                              1
//PIN as UART IO pins. UART module must be enabled
#define AF_UART                                 2
//PIN as I2C IO pins. I2C module must be enabled
#define AF_I2C                                  3
//PIN as FAST I2C IO pins. I2C module must be enabled
#define AF_FAST_I2C                             4
//PIN as timer input/output
#define AF_TIMER                                5

#define GPIO_PORT(pin)                  ((pin) >> 5)
#define GPIO_PIN(pin)                   ((pin) & 0x1f)
#define GPIO_MAKE_PIN(port, pin)        (((port) << 5)  + (pin))

//------------------------------------------------ Timer ---------------------------------------------------------------------

typedef enum {
    TC16B0 = 0,
    TC16B1,
    TC32B0,
    TC32B1,
    TIMER_MAX
} TIMER;

typedef enum {
    LPC_TIMER_START = HAL_IPC(HAL_TIMER),
    LPC_TIMER_STOP
} LPC_TIMER_IPCS;

#define TIMER_CHANNEL0                               0
#define TIMER_CHANNEL1                               1
#define TIMER_CHANNEL2                               2
#define TIMER_CHANNEL3                               3
#define TIMER_CHANNEL_INVALID                        0xff


#define TIMER_FLAG_ENABLE_IRQ                        (1 << 0)
#define TIMER_FLAG_PRIORITY_POS                      16
#define TIMER_FLAG_PRIORITY_MASK                     (0xff << 16)

#define TIMER_MODE_CHANNEL_POS                       16
#define TIMER_MODE_CHANNEL_MASK                      (3 << TIMER_MODE_CHANNEL_POS)

#define TIMER_MODE_CHANNEL0                          (TIMER_CHANNEL0 << TIMER_CHANNEL_POS)
#define TIMER_MODE_CHANNEL1                          (TIMER_CHANNEL1 << TIMER_CHANNEL_POS)
#define TIMER_MODE_CHANNEL2                          (TIMER_CHANNEL2 << TIMER_CHANNEL_POS)
#define TIMER_MODE_CHANNEL3                          (TIMER_CHANNEL3 << TIMER_CHANNEL_POS)

//stop counter after one pulse. Only for channel 0
#define TIMER_MODE_ONE_PULSE                         (1  << 20)

#define TIMER_MODE_TYPE_MASK                         (7 << 21)
#define TIMER_MODE_TYPE_SLAVE                        (0  << 21)
//value in us units
#define TIMER_MODE_TYPE_MASTER_US                    (1  << 21)
//value in hz units
#define TIMER_MODE_TYPE_MASTER_HZ                    (2  << 21)
//time in raw clk units
#define TIMER_MODE_TYPE_MASTER_CLK                   (3  << 21)
#define TIMER_MODE_TYPE_PWM                          (4  << 21)
#define TIMER_MODE_TYPE_PWM_UPDATE                   (5  << 21)

#define TIMER_MODE_PIN_MASK                          (0xff)

//------------------------------------------------ Power ---------------------------------------------------------------------
typedef enum {
    LPC_POWER_GET_SYSTEM_CLOCK = HAL_IPC(HAL_POWER),
    LPC_POWER_UPDATE_CLOCK,
    LPC_POWER_GET_RESET_REASON,
    LPC_POWER_USB_ON,
    LPC_POWER_USB_OFF
} LPC_POWER_IPCS;

typedef enum {
    RESET_REASON_POWERON = 0,
    RESET_REASON_EXTERNAL,
    RESET_REASON_WATCHDOG,
    RESET_REASON_BROWNOUT,
    RESET_REASON_SOFTWARE,
    RESET_REASON_UNKNOWN
} RESET_REASON;

typedef struct {
    RESET_REASON reset_reason;
}POWER_DRV;

//------------------------------------------------- UART ---------------------------------------------------------------------

typedef enum {
    IPC_UART_SET_BAUDRATE = HAL_IPC(HAL_UART),
    IPC_UART_GET_BAUDRATE,
    IPC_UART_GET_LAST_ERROR,
    IPC_UART_CLEAR_ERROR,
    //used internally
    IPC_UART_ISR_TX,
    IPC_UART_ISR_RX
} LPC_UART_IPCS;

typedef enum {
    UART_0 = 0,
    UART_1,
    UART_2,
    UART_3,
    UART_4,
    UART_MAX
}UART_PORT;

typedef struct {
    uint8_t tx, rx;
    uint16_t stream_size;
    BAUD baud;
} UART_ENABLE;

//-------------------------------------------------- I2C ---------------------------------------------------------------------

typedef enum {
    I2C_0,
    I2C_1
} I2C_PORT;

#define I2C_MASTER                  (1 << 16)
#define I2C_SLAVE                   (0 << 16)

#define I2C_NORMAL_SPEED            (0 << 17)
#define I2C_FAST_SPEED              (1 << 17)

//size of address. If 0, no Rs condition will be used. MSB goes first
#define I2C_ADDR_SIZE_POS           0
#define I2C_ADDR_SIZE_MASK          (0xf << 0)

//size of len. Used for some smartcards IO
#define I2C_LEN_SIZE_POS            8
#define I2C_LEN_SIZE_MASK           (0xf << 8)

typedef enum {
    I2C_IO_IDLE = 0,
    I2C_IO_TX,
    I2C_IO_RX
} I2C_IO;


#endif // LPC_DRIVER_H
