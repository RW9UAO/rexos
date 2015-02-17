/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef MAC_H
#define MAC_H

#include "tcpip.h"
#include "../../userspace/eth.h"
#include "sys_config.h"
#include <stdint.h>

typedef struct {
    MAC mac;
} TCPIP_MAC;

/*
    MAC packet header format:

        MAC             dst
        MAC             src
        uint16_t        type/len
 */
#define MAC_HEADER_SIZE                             (MAC_SIZE * 2 + 2)
#define MAC_HEADER_LENTYPE_OFFSET                   (MAC_SIZE * 2)

#define MAC_INDIVIDUAL_ADDRESS                      (0 << 0)
#define MAC_MULTICAST_ADDRESS                       (1 << 0)
#define MAC_BROADCAST_ADDRESS                       (3 << 0)

#define MAC_CAST_MASK                               (3 << 0)

#define ETHERTYPE_IP                                0x0800
#define ETHERTYPE_ARP                               0x0806
#define ETHERTYPE_IPV6                              0x86dd

void mac_init(TCPIP* tcpip);
void mac_info(TCPIP* tcpip);

void mac_rx(TCPIP* tcpip, uint8_t* buf, unsigned int size, HANDLE block);

#endif // MAC_H