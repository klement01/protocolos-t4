#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "udp_client.h"

void *controller(void *ptr);
void *supervisory(void *ptr);

int main(int argc, char* argv[]) {
    pthread_t threadController, threadSupervisory, threadClient;
    int threadRet;

    // Checks if an IP and a port have been provided.
    if (argc != 3) {
        fprintf(stderr, "USAGE: %s <ip> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    //Gather client data.
    ClientData clientData;
    clientData.ip = argv[1];
    clientData.port = argv[2];

    // Creates all the necessary threads.
    if(threadRet = pthread_create(&threadController, NULL, controller, &clientData))
    {
        fprintf(stderr, "Error: simulation thread return code: %d\n", threadRet);
        exit(EXIT_FAILURE);
    }
    if(threadRet = pthread_create(&threadSupervisory, NULL, supervisory, &clientData))
    {
        fprintf(stderr, "Error: supervisory thread return code: %d\n", threadRet);
        exit(EXIT_FAILURE);
    }
    if(threadRet = pthread_create(&threadClient, NULL, udp_client, (void*) &clientData))
    {
        fprintf(stderr, "Error: client thread return code: %d\n", threadRet);
        exit(EXIT_FAILURE);
    }

    /* Waits for threads to end. */  
    pthread_join(threadController, NULL);
    pthread_join(threadSupervisory, NULL);
    pthread_join(threadClient, NULL);

    exit(EXIT_SUCCESS);
}

//TODO
void *controller(void *ptr) {
    //puts("Controller started");
}

//TODO
void *supervisory(void *ptr) {
    //puts("Supervisory started");
}
