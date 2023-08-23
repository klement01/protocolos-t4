#ifndef SERVER_SHARED_H
#define SERVER_SHARED_H

#include "udp_common.h"

typedef struct {
    char* port;
    SCMQ* incomingQueue;
} ServerData;

#endif
