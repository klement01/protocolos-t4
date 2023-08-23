#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "udp_server.h"

void *simulation(void *ptr);
void *supervisory(void *ptr);

int main(int argc, char* argv[]) {
    pthread_t threadSimulation, threadSupervisory, threadServer;
    int threadRet;

    //Checks if a port has been provided.
    if (argc != 2) {
        fprintf(stderr, "USAGE: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    //Gather server data.
    SCMQ incomingQueue;
    SCMQinit(&incomingQueue);
    ServerData serverData;
    serverData.port = argv[1];
    serverData.incomingQueue = &incomingQueue;

    //Create all threads.
    if(threadRet = pthread_create(&threadSimulation, NULL, simulation, &serverData))
    {
        fprintf(stderr, "Error: simulation thread return code: %d\n", threadRet);
        exit(EXIT_FAILURE);
    }
    if(threadRet = pthread_create(&threadSupervisory, NULL, supervisory, &serverData))
    {
        fprintf(stderr, "Error: supervisory thread return code: %d\n", threadRet);
        exit(EXIT_FAILURE);
    }
    if(threadRet = pthread_create(&threadServer, NULL, udp_server, (void*) &serverData))
    {
        fprintf(stderr, "Error: server thread return code: %d\n", threadRet);
        exit(EXIT_FAILURE);
    }
    
    //Waits for threads to end.
    pthread_join(threadSimulation, NULL);
    pthread_join(threadSupervisory, NULL);
    pthread_join(threadServer, NULL);

    exit(EXIT_SUCCESS);
}

//TODO
void *simulation(void *ptr) {
    //puts("Simulation started");
}

//TODO
void *supervisory(void *ptr) {
    //puts("Supervisory started");
}
