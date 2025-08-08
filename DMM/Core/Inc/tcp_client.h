#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include <stddef.h>
#include "lwip/tcp.h"

void tcp_client_init(void);
void tcp_client_send(const char *data, size_t len);

#endif /* TCP_CLIENT_H */
