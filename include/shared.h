#ifndef SHARED_H
#define SHARED_H

#include <pthread.h>

#include <udp_common.h>
#include <simulation.h>

typedef struct {
    char* ip;
    char* port;
    
    SCMQ* incomingQueue;
    SCMQ* outgoingQueue;

    int started;

    pthread_mutex_t levelLock;
    Level level;

    pthread_mutex_t angleLock;
    Angle angleIn;
    Angle angleOut;
} SharedData;

#endif
