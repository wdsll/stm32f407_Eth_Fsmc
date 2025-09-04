/*
 * tcp_client.c
 *
 *  Created on: Aug 8, 2025
 *      Author: huyan
 */

/* Includes ------------------------------------------------------------------*/
#include "tcp_client.h"
#include "socket.h"
#include "w5500_conf.h"
#include <string.h>

#define TCP_CLIENT_SOCKET 0
#define TCP_BUFFER_SIZE   64

static uint8_t tcp_rx_buf[TCP_BUFFER_SIZE];

void tcp_client_init(void)
{
    socket(TCP_CLIENT_SOCKET, Sn_MR_TCP, local_port, 0);
}

void tcp_client_run(void)
{
    if (connect(TCP_CLIENT_SOCKET, remote_ip, remote_port) == SOCK_OK)
    {
        const char hello[] = "Hello from W5500";
        send(TCP_CLIENT_SOCKET, (uint8_t*)hello, strlen(hello));
        recv(TCP_CLIENT_SOCKET, tcp_rx_buf, TCP_BUFFER_SIZE);
        disconnect(TCP_CLIENT_SOCKET);
        close(TCP_CLIENT_SOCKET);
    }
}
