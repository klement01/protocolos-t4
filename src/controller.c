#include <pthread.h>

#include <controller.h>
#include <shared.h>
#include <timer.h>
#include <simulation.h>

#define PERIOD_MS 10
#define COMM_WAIT_MS 100

#define K_P 0.1
#define K_I 0.1

Message* ensureMessageSend(SCMQ* incomingQueue, SCMQ* outgoingQueue,
        Message* mes, MessageType mt) {
    int waitingResponse = 1;
    Message* retMes;
    Message* incMes;

    while (waitingResponse) {
        SCMQqueue(outgoingQueue, mes);
        sleepMs(COMM_WAIT_MS);
        while ((incMes = SCMQdequeue(incomingQueue)) != NULL) {
            if (incMes->messageType == mt) {
                waitingResponse = 0;
                retMes = incMes;
            }
        }
    }
    return retMes;
}

void *controller(void *sdptr) {
    //Unpacks shared data
    SharedData* sharedData = (SharedData*) sdptr;

    SCMQ* incomingQueue = sharedData->incomingQueue;
    SCMQ* outgoingQueue = sharedData->outgoingQueue;

    pthread_mutex_t* levelLock = &(sharedData->levelLock);
    Level* level = &(sharedData->level);

    pthread_mutex_t* angleLock = &(sharedData->angleLock);
    Angle* angleIn = &(sharedData->angleIn);
    Angle* max = &(sharedData->angleOut);

    //Helper variables
    Message mes = {0};
    Message* mesPtr;

    //Initializes data
    *level = INITIAL_LEVEL;
    *angleIn = INITIAL_ANGLE_IN;
    *max = INITIAL_MAX;

    //Control variables
    Angle minAngleIn = INITIAL_ANGLE_IN;
    Angle maxAngleIn = INITIAL_ANGLE_IN;
    Value levelLast = INITIAL_LEVEL;
    double angleTarget = INITIAL_ANGLE_IN;

    //Checks for communication
    puts("Controller initialized, checking for communications");
    mes.messageType = COMM_TEST;
    ensureMessageSend(incomingQueue, outgoingQueue, &mes, COMM_OK);

    //Starts simulation
    puts("Communication ok, starting simulation");
    mes.messageType = START;
    ensureMessageSend(incomingQueue, outgoingQueue, &mes, START_OK);
    sharedData->started = 1;

    //Starts control
    puts("Simulation started, starting control");

    //Sets max value
    puts("Setting max value");
    mes.messageType = SET_MAX;
    mes.value = 100;
    do {
        mesPtr = ensureMessageSend(incomingQueue, outgoingQueue, &mes, MAX);
    } while (mesPtr->value != 100);
    pthread_mutex_lock(angleLock);
    *max = 100;
    pthread_mutex_unlock(angleLock);

    while(1) {
        
    }
}
