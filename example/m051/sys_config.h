/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SYS_CONFIG_H
#define SYS_CONFIG_H

/*
    config.h - userspace config
 */

//add some debug info in SYS
#define SYS_INFO                                            1

//will save few bytes, but not recommended to disable
#define LIB_CHECK_PRESENCE                                  0
//----------------------------- objects ----------------------------------------------
//make sure, you know what are you doing, before change
#define SYS_OBJ_STDOUT                                      0
#define SYS_OBJ_STDIN                                       1
#define SYS_OBJ_CORE                                        2
#define SYS_OBJ_USBD                                        3

#define SYS_OBJ_I2C                                         SYS_OBJ_CORE
#define SYS_OBJ_UART                                        SYS_OBJ_CORE
#define SYS_OBJ_ANALOG                                      SYS_OBJ_CORE
//------------------------------ stdio -----------------------------------------------
#define STDIO_STREAM_SIZE                                   32
//------------------------------ PIN board -------------------------------------------
#define PINBOARD_PROCESS_SIZE                               400
#define PINBOARD_POLL_TIME_MS                               100

#endif // SYS_CONFIG_H
