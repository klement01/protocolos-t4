#ifndef SUPERVISORY_H
#define SUPERVISORY_H

#include <pthread.h>

#include <udp_common.h>
#include <simulation.h>

typedef struct {
    int* started;

    pthread_mutex_t* levelLock;
    Level* level;

    pthread_mutex_t* angleLock;
    Angle* angleIn;
    Angle* angleOut;
} SupervisoryData;

void *supervisory(void *sdptr);

#endif
