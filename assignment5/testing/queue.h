//queue.h

#ifndef QUEUE_H
#define QUEUE_H

typedef struct {
    int front;
    int rear;
    int currentSize;
    int queueSize;
    int *line;
} Queue;

//queues global
//for this project we only need 1 queue for processing requests

//functions
Queue *createQueue(int queueSize);
void enqueue(Queue *queue, int value);
int dequeue(Queue *queue);

void printQueue(Queue *queue);

#endif