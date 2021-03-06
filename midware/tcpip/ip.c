/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "ip.h"
#include "tcpip_private.h"
#include "../../userspace/stdio.h"
#include "icmp.h"
#include <string.h>
#include "udp.h"

#define IP_DF                                   (1 << 6)
#define IP_MF                                   (1 << 5)

#define IP_HEADER_VERSION(buf)                  ((buf)[0] >> 4)
#define IP_HEADER_IHL(buf)                      ((buf)[0] & 0xf)
#define IP_HEADER_TOTAL_LENGTH(buf)             (((buf)[2] << 8) | (buf)[3])
#define IP_HEADER_ID(buf)                       (((buf)[4] << 8) | (buf)[5])
#define IP_HEADER_FLAGS(buf)                    ((buf)[6] & 0xe0)
#define IP_HEADER_FRAME_OFFSET(buf)             ((((buf)[6] & 0x1f) << 8) | (buf[7]))
#define IP_HEADER_TTL(buf)                      ((buf)[8])
#define IP_HEADER_PROTOCOL(buf)                 ((buf)[9])
#define IP_HEADER_CHECKSUM(buf)                 (((buf)[10] << 8) | (buf)[11])
#define IP_HEADER_SRC_IP(buf)                   ((IP*)((buf) + 12))
#define IP_HEADER_DST_IP(buf)                   ((IP*)((buf) + 16))


#if (SYS_INFO) || (TCPIP_DEBUG)
void ip_print(const IP* ip)
{
    int i;
    for (i = 0; i < IP_SIZE; ++i)
    {
        printf("%d", ip->u8[i]);
        if (i < IP_SIZE - 1)
            printf(".");
    }
}
#endif //(SYS_INFO) || (TCPIP_DEBUG)

const IP* tcpip_ip(TCPIP* tcpip)
{
    return &tcpip->ip.ip;
}

void ip_init(TCPIP* tcpip)
{
    tcpip->ip.ip.u32.ip = IP_MAKE(0, 0, 0, 0);
    tcpip->ip.id = 0;
}

#if (SYS_INFO)
static inline void ip_info(TCPIP* tcpip)
{
    printf("IP info\n\r");
    printf("IP: ");
    ip_print(&tcpip->ip.ip);
    printf("\n\r");
}
#endif

static inline void ip_set_request(TCPIP* tcpip, uint32_t ip)
{
    tcpip->ip.ip.u32.ip = ip;
}

static inline void ip_get_request(TCPIP* tcpip, HANDLE process)
{
    ipc_post_inline(process, IP_GET, 0, tcpip->ip.ip.u32.ip, 0);
}

bool ip_request(TCPIP* tcpip, IPC* ipc)
{
    bool need_post = false;
    switch (ipc->cmd)
    {
#if (SYS_INFO)
    case IPC_GET_INFO:
        ip_info(tcpip);
        need_post = true;
        break;
#endif
    case IP_SET:
        ip_set_request(tcpip, ipc->param1);
        need_post = true;
        break;
    case IP_GET:
        ip_get_request(tcpip, ipc->process);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
    return need_post;
}

uint8_t* ip_allocate_io(TCPIP* tcpip, IP_IO* ip_io, unsigned int size, uint8_t proto)
{
    //TODO: fragmented frames
    if (mac_allocate_io(tcpip, &ip_io->io) == NULL)
        return NULL;
    //reserve space for IP header
    ip_io->hdr_size = IP_HEADER_SIZE;
    ip_io->io.buf += IP_HEADER_SIZE;
    ip_io->proto = proto;
    return ip_io->io.buf;
}

void ip_release_io(TCPIP* tcpip, IP_IO *ip_io)
{
    //TODO: fragmented frames
    tcpip_release_io(tcpip, &ip_io->io);
}

void ip_tx(TCPIP* tcpip, IP_IO* ip_io, const IP* dst)
{
    //TODO: fragmented frames
    uint16_t cs;
    ip_io->io.buf -= ip_io->hdr_size;
    ip_io->io.size += ip_io->hdr_size;

    //VERSION, IHL
    ip_io->io.buf[0] = 0x45;
    //DSCP, ECN
    ip_io->io.buf[1] = 0;
    //total len
    ip_io->io.buf[2] = (ip_io->io.size >> 8) & 0xff;
    ip_io->io.buf[3] = ip_io->io.size & 0xff;
    //id
    ip_io->io.buf[4] = (tcpip->ip.id >> 8) & 0xff;
    ip_io->io.buf[5] = tcpip->ip.id & 0xff;
    ++tcpip->ip.id;
    //flags, offset
    ip_io->io.buf[6] = ip_io->io.buf[7] = 0;
    //ttl
    ip_io->io.buf[8] = 0xff;
    //proto
    ip_io->io.buf[9] = ip_io->proto;
    //src
    *(uint32_t*)(ip_io->io.buf + 12) = tcpip_ip(tcpip)->u32.ip;
    //dst
    *(uint32_t*)(ip_io->io.buf + 16) = dst->u32.ip;
    //update checksum
    ip_io->io.buf[10] = ip_io->io.buf[11] = 0;
    cs = ip_checksum(ip_io->io.buf, ip_io->hdr_size);
    ip_io->io.buf[10] = (cs >> 8) & 0xff;
    ip_io->io.buf[11] = cs & 0xff;

    route_tx(tcpip, &(ip_io->io), dst);
}

static void ip_process(TCPIP* tcpip, IP_IO* ip_io, IP* src)
{
#if (IP_DEBUG_FLOW)
    printf("IP: from ");
    ip_print(src);
    printf(", proto: %d, len: %d\n\r", ip_io->proto, ip_io->io.size);
#endif
    switch (ip_io->proto)
    {
#if (ICMP)
    case PROTO_ICMP:
        icmp_rx(tcpip, ip_io, src);
        break;
#endif //ICMP
#if (UDP)
    case PROTO_UDP:
        udp_rx(tcpip, ip_io, src);
        break;
    default:
#endif //UDP
#if (IP_DEBUG)
        printf("IP: unhandled proto %d from", ip_io->proto);
        ip_print(src);
        printf("\n\r");
#endif
#if (ICMP_FLOW_CONTROL)
        icmp_destination_unreachable(tcpip, ICMP_PROTOCOL_UNREACHABLE, ip_io, src);
#else
        ip_release_io(tcpip, ip_io);
#endif
    }
}

#if (IP_CHECKSUM)
uint16_t ip_checksum(uint8_t* buf, unsigned int size)
{
    unsigned int i;
    uint32_t sum = 0;
    for (i = 0; i < (size >> 1); ++i)
        sum += (buf[i << 1] << 8) | (buf[(i << 1) + 1]);
    sum = ((sum & 0xffff) + (sum >> 16)) & 0xffff;
    return ~((uint16_t)sum);
}
#endif

void ip_rx(TCPIP* tcpip, TCPIP_IO* io)
{
    IP_IO ip_io;
    IP src;
    if (io->size < IP_HEADER_SIZE)
    {
        tcpip_release_io(tcpip, io);
        return;
    }
    ip_io.io.size = IP_HEADER_TOTAL_LENGTH(io->buf);
    ip_io.hdr_size = IP_HEADER_IHL(io->buf) << 2;
#if (IP_CHECKSUM)
    //drop if checksum is invalid
    if (ip_checksum(io->buf, ip_io.hdr_size))
    {
        tcpip_release_io(tcpip, io);
        return;
    }
#endif
    src.u32.ip = (IP_HEADER_SRC_IP(io->buf))->u32.ip;
    ip_io.proto = IP_HEADER_PROTOCOL(io->buf);
    ip_io.io.buf = io->buf + ip_io.hdr_size;
    ip_io.io.size -= ip_io.hdr_size;
    ip_io.io.block = io->block;
    if ((IP_HEADER_VERSION(io->buf) != 4) || (ip_io.hdr_size < IP_HEADER_SIZE))
    {
#if (ICMP_FLOW_CONTROL)
        icmp_parameter_problem(tcpip, 0, &ip_io, &src);
#else
        tcpip_release_io(tcpip, io);
#endif
        return;
    }
    //unicast-only filter
    if (tcpip_ip(tcpip)->u32.ip != IP_HEADER_DST_IP(io->buf)->u32.ip)
    {
        tcpip_release_io(tcpip, io);
        return;
    }

    //len more than MTU, inform host by ICMP and only than drop packet
    if (ip_io.io.size > io->size)
    {
#if (ICMP_FLOW_CONTROL)
        icmp_parameter_problem(tcpip, 2, &ip_io, &src);
#else
        tcpip_release_io(tcpip, io);
#endif
        return;
    }
    //ttl exceeded
    if (IP_HEADER_TTL(io->buf) == 0)
    {
#if (ICMP_FLOW_CONTROL)
        icmp_time_exceeded(tcpip, ICMP_TTL_EXCEED_IN_TRANSIT, &ip_io, &src);
#else
        tcpip_release_io(tcpip, io);
#endif
        return;
    }

#if (IP_FRAGMENTATION)
    //TODO: packet assembly from fragments
#error IP FRAGMENTATION is Not Implemented!
#else
    //drop all fragmented frames, inform by ICMP
    if ((IP_HEADER_FLAGS(io->buf) & IP_MF) || (IP_HEADER_FRAME_OFFSET(io->buf)))
    {
#if (ICMP_FLOW_CONTROL)
        icmp_parameter_problem(tcpip, 6, &ip_io, &src);
#else
        tcpip_release_io(tcpip, io);
#endif
        return;
    }
#endif

    ip_process(tcpip, &ip_io, &src);
}
