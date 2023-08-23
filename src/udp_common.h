#ifndef UDP_COMMON_H
#define UDP_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

#define BUFF_SIZE 255
#define SEQ_HIST_LEN 50
#define QUEUE_LEN 10

/* Shows error and kills program */
void Die(char* mess);

/* Types for messages and fields */
typedef uint64_t Seq;
typedef int8_t Value;

typedef enum {
    OPEN_VALVE,
    CLOSE_VALVE,
    GET_LEVEL,
    COMM_TEST,
    SET_MAX,
    START,
    ERR,
} MessageType;

typedef struct {
    MessageType messageType;
    Seq seq;
    Value value;
} Message;

/* Conversion between fields and strings */
Seq strToSeq(char* str);
char* seqToStr(Seq seq);

Value strToValue(char* str);
char* valueToStr(Value value);

/* Queues and lists */
/* Shared Circular Message Queue */
typedef struct {
    pthread_mutex_t lock;
    Message queue[QUEUE_LEN];
    int readHead;
    int writeHead;
} SCMQ;

void SCMQinit(SCMQ* queue);
void SCMQqueue(SCMQ* queue, Message* mes);
Message* SCMQdequeue(SCMQ* queue);

/* Circular Message List */
typedef struct {
    MessageType mtList[SEQ_HIST_LEN];
    Seq seqList[SEQ_HIST_LEN];
    int size;
    int head;
} CML;

void CMLinit(CML* list);
int CMLcheck(CML* list, MessageType mt, Seq seq);
int CMLappend(CML* list, MessageType mt, Seq seq);

#endif
