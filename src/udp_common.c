#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <udp_common.h>

void Die(char* mess) {
    perror(mess);
    exit(EXIT_FAILURE);
}

int strStartsWith(char* str, char* find) {
    return str == strstr(str, find);
}

/* Conversions between fields and strings */
Seq strToSeq(char* str) {
    return strtoul(str, NULL, 10);
}

char* seqToStr(Seq seq) {
    static char buffer[25];
    snprintf(buffer, 25, "%lu", seq);
    return buffer;
}

Value strToValue(char* str) {
    long int value = atol(str);
    if (value < 0 || value > 100) value = -1;
    return value;
}

char* valueToStr(Value value) {
    static char buffer[5];
    if (value < 0) value = 0;
    else if (value > 100) value = 100;
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
    if (queue->writeHead == queue->readHead) Die("Incoming queue full");

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
    if (mt == OPEN_VALVE || mt == CLOSE_VALVE || mt == OPEN || mt == CLOSE) {
        list->mtList[list->head] = mt;
        list->seqList[list->head] = seq;

        if (list->size < SEQ_HIST_LEN) (list->size)++;

        (list->head)++;
        if (list->head >= SEQ_HIST_LEN) list->head -= SEQ_HIST_LEN;
    }

    return 0;
}
