#ifndef UDP_CLIENT_H
#define UDP_CLIENT_H

#include <udp_common.h>
#include <shared.h>
#include <timer.h>

typedef struct {
    Message message;
    struct timespec lastTry;
    int active;
} OutboundMessage;

void *udp_client(void *ip_port);

#endif
