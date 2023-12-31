#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include <udp_server.h>

char incomingBuffer[BUFF_SIZE];
char outgoingBuffer[BUFF_SIZE];

CML seqHistory;
SCMQ* incomingQueue;

pthread_mutex_t* levelLock;
Level* level;

void addMessageToQueue(MessageType mt, Seq seq, Value value) {
    //Checks if sequence value has already been received.
    //If it hasn't, adds it.
    if (CMLappend(&seqHistory, mt, seq)) {
        printf("[SERVER] Sequence collision: %s, %s\n",
            mtToStr(mt), seqToStr(seq));
        return;
    }

    //If not, adds message to incoming queue.
    Message mes;
    mes.messageType = mt;
    mes.seq = seq;
    mes.value = value;
    SCMQqueue(incomingQueue, &mes);
}

char* parseOpenCloseValve(char* message, int open) {
    //OpenValve#⟨seq⟩#⟨value⟩!
    //Open#⟨seq⟩!
    //CloseValve#⟨seq⟩#⟨value⟩!
    //Close#⟨seq⟩!
    char* seqStr;
    char* valueStr;
    Seq seq;
    Value value;
    
    strtok(message, "#");

    seqStr = strtok(NULL, "#");
    if (!seqStr) return NULL;

    valueStr = strtok(NULL, "!");
    if (!valueStr) return NULL;

    seq = strToSeq(seqStr);
    value = strToValue(valueStr);

    if (value < 0 || value > 100) return NULL;

    if (open) {
        addMessageToQueue(OPEN_VALVE, seq, value);
        snprintf(outgoingBuffer, BUFF_SIZE, "Open#%s!", seqStr);
    }
    else {
        addMessageToQueue(CLOSE_VALVE, seq, value);
        snprintf(outgoingBuffer, BUFF_SIZE, "Close#%s!", seqStr);
    }
    
    return outgoingBuffer;
}

char* parseOpenValve(char* message) {
    //OpenValve#⟨seq⟩#⟨value⟩!
    //Open#⟨seq⟩!
    return parseOpenCloseValve(message, 1);
}

char* parseCloseValve(char* message) {
    //CloseValve#⟨seq⟩#⟨value⟩!
    //Close#⟨seq⟩!
    return parseOpenCloseValve(message, 0);
}

char* answerGetLevel() {
    //GetLevel!
    //Level#⟨value⟩!
    pthread_mutex_lock(levelLock);
    Level levelRead = *level;
    pthread_mutex_unlock(levelLock);

    Value levelValue = 100*levelRead;
    snprintf(outgoingBuffer, BUFF_SIZE, "Level#%s!", valueToStr(levelValue));

    return outgoingBuffer;
}

char* parseSetMax(char* message) {
    //SetMax#⟨value⟩!
    //Max#⟨value⟩!
    char* valueStr;
    Value value;
    
    strtok(message, "#");

    valueStr = strtok(NULL, "!");
    if (!valueStr) return NULL;

    value = strToValue(valueStr);

    if (value < 0 || value > 100) return NULL;

    addMessageToQueue(SET_MAX, 0, value);
    snprintf(outgoingBuffer, BUFF_SIZE, "Max#%s!", valueToStr(value));

    return outgoingBuffer;
}

char* parseIncomingMessage(char* message) {
    if (strStartsWith(message, "OpenValve#")) {
        return parseOpenValve(message);
    }

    else if (strStartsWith(message, "CloseValve#")) {
        return parseCloseValve(message);
    }

    else if (strcmp(message, "GetLevel!") == 0) {
        return answerGetLevel();
    }

    else if (strcmp(message, "CommTest!") == 0) {
        return "Comm#OK!";
    }

    else if (strStartsWith(message, "SetMax#")) {
        return parseSetMax(message);
    }

    else if (strcmp(message, "Start!") == 0) {
        addMessageToQueue(START, 0, 0);
        return "Start#OK!";
    }

    return NULL;
}

void* udp_server(void* sdptr) {
    int sock;
    struct sockaddr_in server;
    struct sockaddr_in client;
    char* outgoing;
    unsigned int clientlen, serverlen;
    int receivedlen = 0, outgoinglen = 0;
    SharedData* serverData = (SharedData*)sdptr;

    /* Create the UDP socket. */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        Die("[FATAL][SERVER] Failed to create socket");
    }

    /* Construct the server sockaddr_in structure. */
    memset(&server, 0, sizeof(server));               /* Clear struct */
    server.sin_family = AF_INET;                      /* Internet/IP */
    server.sin_addr.s_addr = htonl(INADDR_ANY);       /* Any IP address */
    server.sin_port = htons(atoi(serverData->port));  /* Server port */

    /* Bind the socket. */
    serverlen = sizeof(server);
    if (bind(sock, (struct sockaddr*) &server, serverlen) < 0) {
        Die("[FATAL][SERVER] Failed to bind socket");
    }

    /* Initialize data structures */
    incomingQueue = serverData->incomingQueue;
    CMLinit(&seqHistory);
    levelLock = &(serverData->levelLock);
    level = &(serverData->level);

    /* Server started. */
    puts("[SERVER] Started");

    /* Run until cancelled */
    while (1) {
        /* Receive a message from the client */
        clientlen = sizeof(client);
        receivedlen = recvfrom(sock, incomingBuffer, BUFF_SIZE, 0,
                (struct sockaddr*) &client,
                &clientlen);
        if (receivedlen < 0 || receivedlen >= BUFF_SIZE) {
            perror("[ERROR][SERVER] Failed to receive message");
        }
        else {
            /* Parse message from the client */
            incomingBuffer[receivedlen] = '\0';

            outgoing = parseIncomingMessage(incomingBuffer);
            if (!outgoing) outgoing = "Err!";
            outgoinglen = strlen(outgoing);

            if (sendto(sock, outgoing, outgoinglen, 0,
                    (struct sockaddr*) &client,
                    sizeof(client)) != outgoinglen) {
                perror("[ERROR][SERVER] mismatch in number of sent bytes");
            }
        }
    }
}
