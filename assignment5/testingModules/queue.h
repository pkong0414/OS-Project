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

#endif