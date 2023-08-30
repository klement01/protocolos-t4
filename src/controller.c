#include <pthread.h>

#include <controller.h>
#include <shared.h>
#include <timer.h>
#include <simulation.h>

#define PERIOD_MS 10
#define COMM_WAIT_MS 100
#define DATA_CYCLES 100

#define SP 80

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
    struct timespec t;
    long deltaMs = 0;
    long cycleCount = 0;

    //Initializes data
    *level = INITIAL_LEVEL;
    *angleIn = INITIAL_ANGLE_IN;
    *max = INITIAL_MAX;

    //Control variables
    int minAngleIn = INITIAL_ANGLE_IN;
    int maxAngleIn = INITIAL_ANGLE_IN;
    int levelLast = INITIAL_LEVEL;
    int maxLast = INITIAL_MAX;
    int error = 0;
    int angleTarget = 0;
    int deltaValve = 0;
    int maxTarget = 0;

    //Checks for communication
    puts("[CONTROLLER] Checking communications");
    mes.messageType = COMM_TEST;
    ensureMessageSend(incomingQueue, outgoingQueue, &mes, COMM_OK);

    //Starts simulation
    puts("[CONTROLLER] Starting simulation");
    mes.messageType = START;
    ensureMessageSend(incomingQueue, outgoingQueue, &mes, START_OK);
    sharedData->started = 1;

    //Starts control
    puts("[CONTROLLER] Simulation started");

    //Sets initial max outflow value
    mes.messageType = SET_MAX;
    mes.value = INITIAL_MAX;
    do {
        mesPtr = ensureMessageSend(incomingQueue, outgoingQueue, &mes, MAX);
    } while (mesPtr->value != INITIAL_MAX);

    getCurrentTime(&t);
    while(1) {
        error = SP - levelLast;

        //Calculates angle and max outflux
        //TODO: PI control

        //On-off control
        if (error > 5) {
            maxTarget = 0;
        }
        else if (error < -5) {
            maxTarget = 100;
        }
        else {
            maxTarget = 80;
        }

        if (error > 5) {
            angleTarget = 100;
        }
        else if (error > 0) {
            angleTarget = 80;
        }
        else if (error < 0) {
            angleTarget = 0;
        }

        //Calculates necessary valve variation to achieve target
        deltaValve = 0;
        if (angleTarget > maxAngleIn) {
            deltaValve = angleTarget - maxAngleIn;
        }
        else if (angleTarget < minAngleIn) {
            deltaValve = angleTarget - minAngleIn;
        }

        //Queues get level message
        mes.messageType = GET_LEVEL;
        SCMQqueue(outgoingQueue, &mes);

        //Sends necessary valve variation
        if (deltaValve != 0) {
            if (deltaValve > 0) {
                mes.messageType = OPEN_VALVE;
                mes.value = deltaValve;
                maxAngleIn += deltaValve;
            }
            else /* deltaValve < 0 */ {
                mes.messageType = CLOSE_VALVE;
                mes.value = -deltaValve;
                minAngleIn += deltaValve;
            }
            SCMQqueue(outgoingQueue, &mes);
        }

        //Sends max outflow target
        mes.messageType = SET_MAX;
        mes.value = maxTarget;
        SCMQqueue(outgoingQueue, &mes);

        //Occasionally shows controller data.
        cycleCount++;
        if (cycleCount % DATA_CYCLES == 0) {
            puts("[CONTROLLER] Data:");
            printf("[CONTROLLER] ---Error:       %d\n", error);
            printf("[CONTROLLER] ---MaxAngleIn:  %d\n", maxAngleIn);
            printf("[CONTROLLER] ---MinAngleIn:  %d\n", minAngleIn);
            printf("[CONTROLLER] ---AngleTarget: %d\n", angleTarget);
        }

        //Displays last level
        pthread_mutex_lock(levelLock);
        *level = levelLast / 100.0;
        pthread_mutex_unlock(levelLock);

        //Displays best angle estimate and max outflow
        pthread_mutex_lock(angleLock);
        *angleIn = (minAngleIn + maxAngleIn) / 2;
        *max = maxLast;
        pthread_mutex_unlock(angleLock);

        //Waits for next control loop
        while ((deltaMs = getPassedTimeMs(&t)) < PERIOD_MS);
        getCurrentTime(&t);

        //Handles incoming messages
        while ((mesPtr = SCMQdequeue(incomingQueue)) != NULL) {
            switch (mesPtr->messageType) {
                case OPEN:
                    minAngleIn += mesPtr->value;
                    break;
                case CLOSE:
                    maxAngleIn -= mesPtr->value;
                    break;
                case LEVEL:
                    levelLast = mesPtr->value;
                    break;
                case MAX:
                    maxLast = mesPtr->value;
                    break;
                default:
                    break;
            }
        }
    }
}
