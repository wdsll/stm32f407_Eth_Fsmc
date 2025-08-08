#include "tcp_client.h"
#include "lwip/err.h"
#include "lwip/init.h"
#include "lwip/ip_addr.h"
#include <string.h>

static struct tcp_pcb *client_pcb;

static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
    const char hello[] = "Hello from STM32F407";
    tcp_write(tpcb, hello, strlen(hello), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
    return ERR_OK;
}

void tcp_client_init(void)
{
    ip_addr_t server_ip;
    IP_ADDR4(&server_ip, 192, 168, 1, 100); /* adjust server IP */
    client_pcb = tcp_new();
    if (client_pcb != NULL)
    {
        tcp_connect(client_pcb, &server_ip, 5000, tcp_client_connected);
    }
}

void tcp_client_send(const char *data, size_t len)
{
    if (client_pcb != NULL && data != NULL && len > 0)
    {
        tcp_write(client_pcb, data, len, TCP_WRITE_FLAG_COPY);
        tcp_output(client_pcb);
    }
}
