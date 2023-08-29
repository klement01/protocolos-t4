#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include <udp_client.h>
#include <controller.h>
#include <supervisory.h>

int main(int argc, char* argv[]) {
    SharedData clientData = {0};
    SupervisoryData supervisoryData = {0};
    pthread_t threadController, threadSupervisory, threadClient;
    int threadRet;

    //Checks if an IP and a port have been provided.
    if (argc != 3) {
        fprintf(stderr, "USAGE: %s <ip> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    //Gather client data.
    clientData.ip = argv[1];
    clientData.port = argv[2];

    SCMQ incomingQueue;
    SCMQinit(&incomingQueue);
    clientData.incomingQueue = &incomingQueue;

    SCMQ outgoingQueue;
    SCMQinit(&outgoingQueue);
    clientData.outgoingQueue = &outgoingQueue;

    clientData.started = 0;
    pthread_mutex_init(&(clientData.levelLock), NULL);
    pthread_mutex_init(&(clientData.angleLock), NULL);

    //Gather supervisory data.
    supervisoryData.started = &(clientData.started);
    supervisoryData.levelLock = &(clientData.levelLock);
    supervisoryData.level = &(clientData.level);
    supervisoryData.angleLock = &(clientData.angleLock);
    supervisoryData.angleIn = &(clientData.angleIn);
    supervisoryData.angleOut = &(clientData.angleOut);

    // Creates all the necessary threads.
    if(threadRet = pthread_create(&threadController, NULL, controller, &clientData))
    {
        fprintf(stderr, "[FATAL][MAIN] Simulation thread return code: %d\n", threadRet);
        exit(EXIT_FAILURE);
    }
    if(threadRet = pthread_create(&threadSupervisory, NULL, supervisory, &supervisoryData))
    {
        fprintf(stderr, "[FATAL][MAIN] Supervisory thread return code: %d\n", threadRet);
        exit(EXIT_FAILURE);
    }
    if(threadRet = pthread_create(&threadClient, NULL, udp_client, (void*) &clientData))
    {
        fprintf(stderr, "[FATAL][MAIN] Client thread return code: %d\n", threadRet);
        exit(EXIT_FAILURE);
    }

    /* Waits for threads to end. */  
    pthread_join(threadController, NULL);
    pthread_join(threadSupervisory, NULL);
    pthread_join(threadClient, NULL);

    exit(EXIT_SUCCESS);
}
