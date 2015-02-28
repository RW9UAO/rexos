/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef MT_CONFIG_H
#define MT_CONFIG_H

#include "../../../rexos/userspace/m051_driver.h"

//----------------------- pin definition -------------------------------------------
#define SSD1306_DC_PORT		P24
#define SSD1306_RST_PORT	P23
#define SSD1306_CS_PORT		P20
#define SSD1306_SCLK_PORT	P21
#define SSD1306_SDAT_PORT	P22

//backlight pin
//#define MT_BACKLIGHT                    B9
//------------------------------ timeouts ------------------------------------------
//all CLKS time. Refer to datasheet for more details
//Address hold time, min 140ns
//#define TAS                             1
//Data read prepare time, max 320ns
//#define TDDR                            2
//E high pulse time, min 450ns
//#define PW                              3
//Delay between commands, min 8us
//#define TW                              40
//Reset time, max 10us
#define TR                              50
//Reset impulse time, min 200ns
#define TRI                             2
//------------------------------ general ---------------------------------------------
//pixel test api
#define MT_TEST                         1
//#define X_MIRROR                        1

//------------------------------ process -------------------------------------------
//Use as driver or as library. Driver has some switch latency time and require stack memory. However,
//many processes can use low-level LCD API
#define MT_DRIVER                       0
#define MT_STACK_SIZE                   340
//increase in case when many process use LCD at same time
#define MT_IPC_COUNT                    5

#endif // MT_CONFIG_H
