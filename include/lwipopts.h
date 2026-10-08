#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

// 1. LWIP (Lightweight IP) SYSTEM CONFIGURATION FOR FREERTOS + SDK PICO W
#define NO_SYS                      0
#define LWIP_SOCKET                 0
#define SYS_LIGHTWEIGHT_PROT        1
#define MEM_LIBC_MALLOC             0
#define MEM_ALIGNMENT               4

// 2. THREAD CONFIGURATION
#define TCPIP_THREAD_STACKSIZE      1024
#define DEFAULT_THREAD_STACKSIZE    2048
#define TCPIP_THREAD_PRIO           1
#define DEFAULT_THREAD_PRIO         1
#define TCPIP_MBOX_SIZE             8
#define DEFAULT_RAW_RECVMBOX_SIZE   8

// 2. MEMORY CONFIGURATION
#define MEM_SIZE                    4000
#define MEMP_NUM_TCP_SEG            32
#define MEMP_NUM_ARP_QUEUE          10
#define PBUF_POOL_SIZE              24
#define MEMP_NUM_SYS_TIMEOUT        12
#define MEMP_NUM_TCP_PCB            4

// 3. NETWORK PROTOCOLS CONFIGURATION
#define LWIP_ARP                    1
#define LWIP_ETHERNET               1
#define LWIP_ICMP                   1
#define LWIP_RAW                    1
#define LWIP_DHCP                   1
#define LWIP_IPV4                   1
#define LWIP_UDP                    1
#define LWIP_DNS                    1
#define LWIP_NETIF_STATUS_CALLBACK  1
#define LWIP_NETIF_LINK_CALLBACK    1

// 4. TCP & MQTT CONFIGURATION
#define LWIP_TCP                    1
#define LWIP_MQTT                   1
#define MQTT_OUTPUT_RINGBUF_SIZE    256

#define TCP_WND                     (8 * TCP_MSS)
#define TCP_MSS                     1460
#define TCP_SND_BUF                 (8 * TCP_MSS)
#define TCP_SND_QUEUELEN            ((4 * (TCP_SND_BUF) + (TCP_MSS - 1)) / (TCP_MSS))
#define LWIP_TCP_KEEPALIVE          1
#define LWIP_SO_RCVTIMEO            1

#endif /* _LWIPOPTS_H */
