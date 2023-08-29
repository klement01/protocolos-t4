#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>

#include <udp_client.h>
#include <timer.h>

#define RETRY_MS 50
#define SEQ_MAX 10000

char incomingBuffer[BUFF_SIZE];
char outgoingBuffer[BUFF_SIZE];

CML incomingSeqHistory;
SCMQ* incomingQueue;

CML outgoingSeqHistory;
SCMQ* outgoingQueue;
OutboundMessage retryMessageList[QUEUE_LEN] = {0};

Seq sequenceCounter;

Seq messageGetSeq(char* message, int* err) {
    char* seqStr;

    *err = 1;

    strtok(message, "#");
    
    seqStr = strtok(NULL, "#");
    if (!seqStr) return 0;

    *err = 0;
    return strToSeq(seqStr);
}

Value messageGetValue(char* message, int* err) {
    char* valueStr;
    Value value;

    *err = 1;

    strtok(message, "#");
    
    valueStr = strtok(NULL, "#");
    if (!valueStr) return 0;

    value = strToValue(valueStr);
    if (value < 0 || value > 100) return 0;

    *err = 0;
    return value;
}

void parseSeqMessage(char* message, MessageType mtOut, MessageType mtIn) {
    int err;
    Seq seq = messageGetSeq(message, &err);
    if (err) return;

    //Dies if sequence hasn't been sent
    if (!CMLcheck(&outgoingSeqHistory, mtOut, seq)) Die("Received unexpected sequence");

    //If sequence is received for the first time, remove it from retry
    //queue and add it to incoming queue
    if (!CMLappend(&incomingSeqHistory, mtIn, seq)) {
        Value value;

        int mesFound = 0;
        for (int i = 0; i < QUEUE_LEN; i++) {
            Message* mes2 = &(retryMessageList[i].message);
            if (mes2->messageType == mtOut && mes2->seq == seq) {
                retryMessageList[i].active = 0;
                value = mes2->value;
                mesFound = 1;
                break;
            }
        }
        if (!mesFound) Die("Couldn't find message in retry queue");

        Message mes = {0};
        mes.messageType = mtIn;
        mes.seq = seq;
        mes.value = value;
        SCMQqueue(incomingQueue, &mes);
    }
}

void parseValueMessage(char* message, MessageType mt) {
    int err;
    Value value = messageGetValue(message, &err);
    if (err) return;

    Message mes = {0};
    mes.messageType = mt;
    mes.value = value;
    SCMQqueue(incomingQueue, &mes);
}

void parseIncomingMessage(char* message) {
    Message mes = {0};

    if (strStartsWith(message, "Open#")) {
        parseSeqMessage(message, OPEN_VALVE, OPEN);
    }

    else if (strStartsWith(message, "Close#")) {
        parseSeqMessage(message, CLOSE_VALVE, CLOSE);
    }

    else if (strStartsWith(message, "Level#")) {
        parseValueMessage(message, LEVEL);
    }

    else if (strcmp(message, "Comm#OK!") == 0) {
        mes.messageType = COMM_OK;
        SCMQqueue(incomingQueue, &mes);
    }

    else if (strStartsWith(message, "Max#")) {
        parseValueMessage(message, MAX);
    }

    else if (strcmp(message, "Start#OK!") == 0) {
        mes.messageType = START_OK;
        SCMQqueue(incomingQueue, &mes);
    }
}

void sendMessage(Message* mes, int sock, struct sockaddr* server) {
    //Generates message string
    char* seq = seqToStr(mes->seq);
    char* value = valueToStr(mes->value);
    switch (mes->messageType) {
        case OPEN_VALVE:
            snprintf(outgoingBuffer, BUFF_SIZE, "OpenValve#%s#%s!", seq, value);
            break;
        case CLOSE_VALVE:
            snprintf(outgoingBuffer, BUFF_SIZE, "CloseValve#%s#%s!", seq, value);
            break;
        case GET_LEVEL:
            strncpy(outgoingBuffer, "GetLevel!", BUFF_SIZE);
            break;
        case COMM_TEST:
            strncpy(outgoingBuffer, "CommTest!", BUFF_SIZE);
            break;
        case SET_MAX:
            snprintf(outgoingBuffer, BUFF_SIZE, "SetMax#%s!", value);
            break;
        case START:
            strncpy(outgoingBuffer, "Start!", BUFF_SIZE);
            break;
        default:
            Die("Invalid message type");
            break;
    }

    //Sends message string
    int echolen = strlen(outgoingBuffer);
    printf("Sending message: %s\n", outgoingBuffer);
    if (sendto(sock, outgoingBuffer, echolen, 0,
            server, sizeof(struct sockaddr_in)) != echolen) {
        perror("Mismatch in number of sent bytes");
    }
}

void *udp_client(void *cdptr) {
    int sock;
    struct sockaddr_in server;
    struct sockaddr_in client;
    unsigned int clientlen;
    int received = 0;
    SharedData* clientData = (SharedData*)cdptr;
    fd_set readSet;
    struct timeval timeout = {0};
    Message* mes;

    /* Create the UDP socket */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        Die("Failed to create socket");
    }

    /* Construct the server sockaddr_in structure */
    memset(&server, 0, sizeof(server));                  /* Clear struct */
    server.sin_family = AF_INET;                         /* Internet/IP */
    server.sin_addr.s_addr = inet_addr(clientData->ip);  /* IP address */
    server.sin_port = htons(atoi(clientData->port));     /* server port */

    /* Initialize data structures */
    incomingQueue = clientData->incomingQueue;
    outgoingQueue = clientData->outgoingQueue;
    CMLinit(&incomingSeqHistory);
    CMLinit(&outgoingSeqHistory);
    srand(time(NULL));
    sequenceCounter = rand() % SEQ_MAX;

    /* Client started. */
    puts("UDP client started");
    printf("Initial sequence number: %lu\n", sequenceCounter);

    /* Send the word to the server */
    while (1) {
        /* Check for incoming message */
        FD_SET(sock, &readSet);
        select(sock+1, &readSet, NULL, NULL, &timeout);
        if (FD_ISSET(sock, &readSet)) {
            /* Receive the word back from the server */
            clientlen = sizeof(client);
            received = recvfrom(sock, incomingBuffer, BUFF_SIZE, 0,
                        (struct sockaddr *) &client,
                        &clientlen);
            incomingBuffer[received] = '\0';

            /* Check that client and server are using same socket */
            if (server.sin_addr.s_addr != client.sin_addr.s_addr) {
                perror("Received a packet from an unexpected server");
                fprintf(stderr, "Server: %ud\n", server.sin_addr.s_addr);
                fprintf(stderr, "Client: %ud\n", client.sin_addr.s_addr);
                exit(EXIT_FAILURE);
            }

            /* Parses received message */
            printf("Received message: %s\n", incomingBuffer);
            parseIncomingMessage(incomingBuffer);
        }

        /* Sends new messages */
        while ((mes = SCMQdequeue(outgoingQueue)) != NULL) {
            //Deals with sequenced messages 
            if (mes->messageType == OPEN_VALVE || mes->messageType == CLOSE_VALVE) {
                //Sets sequence number
                mes->seq = sequenceCounter;
                sequenceCounter = (sequenceCounter + 1) % SEQ_MAX;

                //Adds sequence to sent sequence list
                if (CMLappend(&outgoingSeqHistory, mes->messageType, mes->seq)) {
                    Die("Unexpected sequence collision when queueing message");
                }

                //Adds to messages awaiting retries
                int spotFound = 0;
                for (int i = 0; i < QUEUE_LEN; i++) {
                    if (!retryMessageList[i].active) {
                        retryMessageList[i].message = *mes;
                        getCurrentTime(&(retryMessageList[i].lastTry));
                        retryMessageList[i].active = 1;
                        spotFound = 1;
                        break;
                    }
                }
                if (!spotFound) Die("Unable to queue message for retry");
            }
            
            //Send message
            sendMessage(mes, sock, (struct sockaddr*) &server);
        }

        /* Send messages awaiting retry */
        for (int i = 0; i < QUEUE_LEN; i++) {
            if (retryMessageList[i].active
                    && (getPassedTimeMs(&(retryMessageList[i].lastTry)) > RETRY_MS)) {
                sendMessage(&(retryMessageList[i].message), sock, (struct sockaddr*) &server);
                getCurrentTime(&(retryMessageList[i].lastTry));
            }
        }
    }
}
