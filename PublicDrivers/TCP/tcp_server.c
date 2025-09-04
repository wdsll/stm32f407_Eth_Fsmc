/* Includes ------------------------------------------------------------------*/
#include<stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#include "lwip/init.h"
#include "lwip/sockets.h"
#include "lwip/netif.h"
#include "lwip/ip_addr.h"
#include "lwip/timeouts.h"
//#include "loopif.h"

#define PORT 8080
#define BUFFER_SIZE 1024
#define ETH_DESC_COUNT 4
#define ETH_MAX_PACKET_SIZE 1536

#define ETH_DESC_SECTION __attribute__((section(".eth_desc")))
#define ETH_BUFF_SECTION __attribute__((section(".eth_buff")))

static void check_not_in_ccm(const void *addr, const char *name) {
    uintptr_t a = (uintptr_t)addr;
    if (a >= CCM_START && a <= CCM_END) {
        fprintf(stderr, "%s located in CCM at 0x%08" PRIXPTR "\n", name, a);
    }
}

static void init_network(struct netif *netif)
{
    ip4_addr_t ipaddr, netmask, gw;
    IP4_ADDR(&ipaddr, 127, 0, 0, 1);
    IP4_ADDR(&netmask, 255, 0, 0, 0);
    IP4_ADDR(&gw, 127, 0, 0, 1);

    lwip_init();
    netif_add(netif, &ipaddr, &netmask, &gw, NULL, loopif_init, ip_input);
    netif_set_default(netif);
    netif_set_up(netif);
}

int tcp_test()
{
    struct netif netif;
    int server_fd, new_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];
    check_not_in_ccm(eth_dma_desc, "eth_dma_desc");
    check_not_in_ccm(eth_rx_buffer, "eth_rx_buffer");
    init_network(&netif);


    server_fd = lwip_socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("lwip_socket failed");
        return EXIT_FAILURE;
    }
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = PP_HTONL(INADDR_ANY);
    address.sin_port = PP_HTONS(PORT);

    if (lwip_bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("lwip_bind failed");
        lwip_close(server_fd);
        return EXIT_FAILURE;
    }

    if (lwip_listen(server_fd, 3) < 0) {
        perror("lwip_listen failed");
        lwip_close(server_fd);
        return EXIT_FAILURE;
    }

    printf("lwIP TCP server listening on port %d\n", PORT);

    while(1)
    {
        new_socket = lwip_accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (new_socket < 0) {
            perror("lwip_accept failed");
            continue;
        }
        ssize_t nread;
        while ((nread = lwip_read(new_socket, buffer, sizeof(buffer))) > 0) {
            lwip_write(new_socket, buffer, nread);
        }
        lwip_close(new_socket);
        sys_check_timeouts();
    }
    lwip_close(server_fd);
    return 0;
}
