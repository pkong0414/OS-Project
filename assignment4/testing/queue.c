// queue.c
// NOTE: Portion of the queue was taken from website codesdope's article making a queue using an array.

#include "queue.h"
#include <stdio.h>
#include <stdlib.h>

//macro for max size
#define MAXSIZE 18

//queues global
extern Queue *qset1[4];
extern Queue *qset2[4];
extern Queue blocked;
extern Queue **active;
extern Queue **expired;

//size of the array
static int size;

void createQueues() {
    int i;
    for( i = 0; i < 4; i++){
        qset1[i] = createQueue( 18 );
        qset2[i] = createQueue( 18 );
    }
    active = qset1;
    expired = qset2;
}

Queue* createQueue(int queueSize)
{
    Queue* queue = (Queue*) malloc(sizeof(Queue));
    queue->queueSize = queueSize;
    queue->front = queue->currentSize = 0;
    queue->rear = queueSize - 1;  // This is important, see the enqueue
    queue->line = (int*)malloc(queue->queueSize * sizeof(int));
    return queue;
}

void enqueue(Queue *queue, int value) {
    //idea is to assign to the appropriate based on priority.
    //we don't need to use priority as we can just set the relevant array in the oss part of the code
    if(size<MAXSIZE) {
        if(queue->queueSize<0) {
            queue->line[0] = value;
            queue->front = queue->rear = 0;
            queue->queueSize = 1;
        }
        else if(queue->rear == MAXSIZE-1) {
            queue->line[0] = value;
            queue->rear = 0;
            queue->queueSize++;
        }
        else {
            queue->line[queue->rear+1] = value;
            queue->rear++;
            queue->queueSize++;
        }
    }
    else {
        printf("Queue is full\n");
    }
}

int dequeue(Queue *queue)
{
    if(size<0) {
        printf("Queue is empty\n");
    }
    else {
        queue->currentSize--;
        queue->front++;
    }
}

void swapQueues() {
    Queue **temp = active;
    active = expired;
    expired = temp;
}

void printQueue(Queue *queue) {
    int i;
    for (i = 0; i < queue->queueSize; i++) {
        printf("%d ", queue->line[i]);
    }
    printf("\n");
}

/*********************  testing purpose only *****************************
void main() {
    createQueues();
    int i = 3;
    queue_push(active[i], 1);
    queue_push(active[i], 2);
    queue_push(expired[i], 3);
    queue_push(expired[i], 4);
    printf("Active queue %d before swap => ", i);
    printQueue(active[i]);
    printf("Expired queue %d before swap => ", i);
    printQueue(expired[i]);
    swapQueues();
    printf("Active queue %d after swap => ", i);
    printQueue(active[i]);
    printf("Expired queue %d after swap => ", i);
    printQueue(expired[i]);
}

*************************************************************************/