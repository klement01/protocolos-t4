#ifndef SERVER_SHARED_H
#define SERVER_SHARED_H

#include <pthread.h>

#include <udp_common.h>
#include <simulation.h>

typedef struct {
    char* port;
    
    SCMQ* incomingQueue;

    int started;

    pthread_mutex_t levelLock;
    Level level;

    pthread_mutex_t angleLock;
    Angle angleIn;
    Angle angleOut;
} ServerData;

#endif
