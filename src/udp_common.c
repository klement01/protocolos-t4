#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "udp_common.h"

char buffer[BUFF_SIZE];

void Die(char* mess) {
    perror(mess);
    exit(EXIT_FAILURE);
}

/* Conversions between fields and strings */
Seq strToSeq(char* str) {
    return atol(str);
}

char* seqToStr(Seq seq) {
    snprintf(buffer, 25, "%ld", seq);
    return buffer;
}

Value strToValue(char* str) {
    long int value = atol(str);
    if (value < 0 || value > 100) value = -1;
    return value;
}

char* valueToStr(Value value) {
    snprintf(buffer, 5, "%03d", value);
    return buffer;
}

/* SCMQ functions*/
void SCMQinit(SCMQ* queue) {
    memset(queue, 0, sizeof(SCMQ));
    pthread_mutex_init(&(queue->lock), NULL);
}

void SCMQqueue(SCMQ* queue, Message* mes) {
    pthread_mutex_lock(&(queue->lock));

    Message* oldMes = &(queue->queue[queue->writeHead]);
    oldMes->messageType = mes->messageType;
    oldMes->seq = mes->seq;
    oldMes->value = mes->value;

    (queue->writeHead)++;
    if (queue->writeHead >= QUEUE_LEN) queue->writeHead -= QUEUE_LEN;

    pthread_mutex_unlock(&(queue->lock));
}

Message* SCMQdequeue(SCMQ* queue) {
    Message* mes;

    pthread_mutex_lock(&(queue->lock));

    if (queue->readHead == queue->writeHead) {
        mes = NULL;
    }
    else {
        mes = &(queue->queue[queue->readHead]);
        (queue->readHead)++;
        if (queue->readHead >= QUEUE_LEN) queue->readHead -= QUEUE_LEN;
    }
    
    pthread_mutex_unlock(&(queue->lock));

    return mes;
}

/* CML functions */
void CMLinit(CML* list) {
    memset(list, 0, sizeof(CML));
}

int CMLcheck(CML* list, MessageType mt, Seq seq) {
    for (int i = 0; i < list->size; i++) {
        if (list->mtList[i] == mt && list->seqList[i] == seq) return 1;
    }
    return 0;
}

int CMLappend(CML* list, MessageType mt, Seq seq) {
    if (CMLcheck(list, mt, seq)) return 1;

    //Only adds valid message types.
    if (mt == OPEN_VALVE || mt == CLOSE_VALVE) {
        list->mtList[list->head] = mt;
        list->seqList[list->head] = seq;

        if (list->size < SEQ_HIST_LEN) (list->size)++;

        (list->head)++;
        if (list->head >= SEQ_HIST_LEN) list->head -= SEQ_HIST_LEN;
    }

    return 0;
}
