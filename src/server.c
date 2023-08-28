#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include <udp_server.h>
#include <simulation.h>
#include <supervisory.h>

void *simulation(void *ptr);
void *supervisory(void *ptr);

int main(int argc, char* argv[]) {
    SharedData serverData = {0};
    SupervisoryData supervisoryData = {0};
    pthread_t threadSimulation, threadSupervisory, threadServer;
    int threadRet;

    //Checks if a port has been provided.
    if (argc != 2) {
        fprintf(stderr, "USAGE: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    //Gather server data.
    serverData.port = argv[1];

    SCMQ incomingQueue;
    SCMQinit(&incomingQueue);
    serverData.incomingQueue = &incomingQueue;

    serverData.started = 0;
    pthread_mutex_init(&(serverData.levelLock), NULL);
    pthread_mutex_init(&(serverData.angleLock), NULL);

    //Gather supervisory data.
    supervisoryData.started = &(serverData.started);
    supervisoryData.levelLock = &(serverData.levelLock);
    supervisoryData.level = &(serverData.level);
    supervisoryData.angleLock = &(serverData.angleLock);
    supervisoryData.angleIn = &(serverData.angleIn);
    supervisoryData.angleOut = &(serverData.angleOut);

    //Create all threads.
    if(threadRet = pthread_create(&threadSimulation, NULL, simulation, &serverData))
    {
        fprintf(stderr, "Error: simulation thread return code: %d\n", threadRet);
        exit(EXIT_FAILURE);
    }
    if(threadRet = pthread_create(&threadSupervisory, NULL, supervisory, &supervisoryData))
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
