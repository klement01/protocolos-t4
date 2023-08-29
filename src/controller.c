#include <pthread.h>

#include <controller.h>
#include <shared.h>
#include <timer.h>
#include <simulation.h>

#define PERIOD_MS 10
#define COMM_WAIT_MS 100

#define MAX_OUT 50
#define K_P 0.5
#define K_I 0.1
#define SP 80
#define K_AW (2*K_I)

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
    Angle* angleOut = &(sharedData->angleOut);

    //Helper variables
    Message mes = {0};
    Message* mesPtr;
    int levelChanged;
    struct timespec t;
    long deltaMs;

    //Initializes data
    *level = INITIAL_LEVEL;
    *angleIn = INITIAL_ANGLE_IN;
    *angleOut = 0;

    //Control variables
    Value minAngleIn = INITIAL_ANGLE_IN;
    Value maxAngleIn = INITIAL_ANGLE_IN;
    Value levelLast = INITIAL_LEVEL;
    Value deltaValve;
    Value error;
    double angleTarget = INITIAL_ANGLE_IN;
    double errorIntegral = 0;
    double saturation;

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
    mes.value = MAX_OUT;
    do {
        mesPtr = ensureMessageSend(incomingQueue, outgoingQueue, &mes, MAX);
    } while (mesPtr->value != MAX_OUT);

    getCurrentTime(&t);
    deltaMs = 0;
    while(1) {
        //Calculates angle
        //TODO: PI control
        /*
        error = SP - levelLast;
        errorIntegral += error * deltaMs;
        angleTarget = K_P * error + K_I * errorIntegral / 1000.0;
        if (angleTarget > 100) {
            saturation = angleTarget - 100;
            angleTarget = 100;
        }
        else if (angleTarget < 0) {
            saturation = angleTarget;
            angleTarget = 0;
        }
        else {
            saturation = 0;
        }
        errorIntegral -= K_AW * saturation * deltaMs / 1000.0;
        */

        //On-off control
        error = SP - levelLast;
        if (error > 0) angleTarget = 100;
        if (error < 0) angleTarget = 0;

        //Calculates necessary valve delta
        if (angleTarget > maxAngleIn) deltaValve = angleTarget - maxAngleIn;
        else if (angleTarget < minAngleIn) deltaValve = angleTarget - minAngleIn;
        else deltaValve = 0;

        //Queues open/close valve message (if needed)
        if (deltaValve != 0) {
            if (deltaValve > 0) {
                mes.messageType = OPEN_VALVE;
                mes.value = deltaValve;
                maxAngleIn += deltaValve;
            }
            else {
                mes.messageType = CLOSE_VALVE;
                mes.value = -deltaValve;
                minAngleIn -= deltaValve;
            }
            SCMQqueue(outgoingQueue, &mes);
        }

        //Queues get level message
        mes.messageType = GET_LEVEL;
        SCMQqueue(outgoingQueue, &mes);

        //Displays best angle estimate
        pthread_mutex_lock(angleLock);
        *angleIn = (minAngleIn + maxAngleIn) / 2;
        pthread_mutex_unlock(angleLock);

        //Waits for next control loop
        while ((deltaMs = getPassedTimeMs(&t)) < PERIOD_MS);
        getCurrentTime(&t);

        //Handles incoming messages
        levelChanged = 0;
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
                    levelChanged = 1;
                    break;
                default:
                    break;
            }
        }

        //Displays last level
        if (levelChanged) {
            pthread_mutex_lock(levelLock);
            *level = levelLast / 100.0;
            pthread_mutex_unlock(levelLock);
        }
    }
}
