#ifndef NET_CONF_H
#define NET_CONF_H

/*
 * Default network configuration values.
 * Override these macros at build time (e.g. by passing -DNET_IP_ADDR0=10) or
 * edit this file to adapt the board IP or MAC address without touching the
 * application sources.
 */

#ifndef NET_IP_ADDR0
#define NET_IP_ADDR0   192
#endif
#ifndef NET_IP_ADDR1
#define NET_IP_ADDR1   168
#endif
#ifndef NET_IP_ADDR2
#define NET_IP_ADDR2   0
#endif
#ifndef NET_IP_ADDR3
#define NET_IP_ADDR3   10
#endif

#ifndef NET_NETMASK0
#define NET_NETMASK0   255
#endif
#ifndef NET_NETMASK1
#define NET_NETMASK1   255
#endif
#ifndef NET_NETMASK2
#define NET_NETMASK2   255
#endif
#ifndef NET_NETMASK3
#define NET_NETMASK3   0
#endif

#ifndef NET_GW_ADDR0
#define NET_GW_ADDR0   192
#endif
#ifndef NET_GW_ADDR1
#define NET_GW_ADDR1   168
#endif
#ifndef NET_GW_ADDR2
#define NET_GW_ADDR2   0
#endif
#ifndef NET_GW_ADDR3
#define NET_GW_ADDR3   1
#endif

#ifndef NET_MAC_ADDR0
#define NET_MAC_ADDR0  0x00
#endif
#ifndef NET_MAC_ADDR1
#define NET_MAC_ADDR1  0x80
#endif
#ifndef NET_MAC_ADDR2
#define NET_MAC_ADDR2  0xE1
#endif
#ifndef NET_MAC_ADDR3
#define NET_MAC_ADDR3  0x00
#endif
#ifndef NET_MAC_ADDR4
#define NET_MAC_ADDR4  0x00
#endif
#ifndef NET_MAC_ADDR5
#define NET_MAC_ADDR5  0x00
#endif

#endif /* NET_CONF_H */
