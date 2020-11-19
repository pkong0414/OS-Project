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

void createQueues();
Queue *createQueue(int queueSize);
void swapQueues();
void enqueue(Queue *queue, int value);
int dequeue(Queue *queue);
void printQueue(Queue *queue);

//queues global
//for this project we only need 1 queue for processing requests
extern Queue *resourceRequest;
extern Queue *qset1[4];
extern Queue *qset2[4];
extern Queue *blocked;
extern Queue **active;
extern Queue **expired;

#endif