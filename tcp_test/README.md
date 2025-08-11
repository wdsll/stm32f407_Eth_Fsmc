# stm32f407_Eth_Fsmc

## Network configuration

Network addressing is no longer hard-coded in the sources. Default values for
IP, netmask, gateway and MAC address reside in `LWIP/App/net_conf.h`.

To adapt the board configuration without editing the C files you can either
modify `net_conf.h` or override any of the `NET_*` macros at build time, for
example:

```
-DNET_IP_ADDR0=10 -DNET_IP_ADDR1=0 -DNET_IP_ADDR2=0 -DNET_IP_ADDR3=123
-DNET_MAC_ADDR0=0x02 -DNET_MAC_ADDR1=0x12 -DNET_MAC_ADDR2=0x34 \
-DNET_MAC_ADDR3=0x56 -DNET_MAC_ADDR4=0x78 -DNET_MAC_ADDR5=0x9A
```

These options allow the network parameters to be adjusted without modifying
application source files.
