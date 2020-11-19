// queue.c
// NOTE: Portion of the queue was taken from website codesdope's article making a queue using an array.

#include "queue.h"
#include <stdio.h>
#include <stdlib.h>

//macro for max size
#define MAXSIZE 18

void createQueues() {
    int i;
    for( i = 0; i < 4; i++){
        qset1[i] = createQueue( 18 );
        qset2[i] = createQueue( 18 );
    }
    blocked = createQueue(18);
    active = qset1;
    expired = qset2;
}

Queue* createQueue(int queueSize)
{
    Queue* queue = (Queue*) malloc(sizeof(Queue));
    queue->queueSize = queueSize;
    queue->front = queue->currentSize = -1;
    queue->rear = queue->currentSize;  // This is important, see the enqueue
    queue->line = (int*) malloc(queueSize * sizeof(int));
    return queue;
}

void enqueue(Queue *queue, int value) {
    //idea is to assign to the appropriate based on priority.
    //we don't need to use priority as we can just set the relevant array in the oss part of the code
    if(queue->currentSize<MAXSIZE)
    {
        if(queue->currentSize < 0)
        {
            queue->line[0] = value;
            queue->front = queue->rear = 0;
            queue->currentSize = 1;
        }
        else if(queue->rear == MAXSIZE-1)
        {
            queue->line[0] = value;
            queue->rear = 0;
            queue->currentSize++;
        }
        else
        {
            queue->line[queue->rear+1] = value;
            queue->rear++;
            queue->currentSize++;
        }
    }
    else
    {
        printf("Queue is full\n");
    }
}

int dequeue(Queue *queue)
{
    if (queue->currentSize <= 0)
        return -1;
    int item = queue->line[queue->front];
    queue->front = (queue->front + 1)
            % queue->queueSize;
    queue->currentSize = queue->currentSize - 1;
    return item;
}

void swapQueues() {
    Queue **temp = active;
    active = expired;
    expired = temp;
}

void printQueue(Queue *queue) {
    int i;
    for (i = queue->front; i <= queue->rear; ++i) {
        printf("%d ", queue->line[i]);
    }
    printf("\n");
}

/*********************  testing purpose only *****************************
void main() {
    createQueues();
    int i = 3;
    //this should give me an error
    printf( "Dequeuing %d from active...\n", dequeue( active[i]) );
    enqueue(active[i], 1);
    enqueue(active[i], 2);
    enqueue(active[i], 1);
    enqueue(active[i], 2);
    enqueue(expired[i], 3);
    enqueue(expired[i], 4);
    enqueue(expired[i], 3);
    enqueue(expired[i], 4);
    printf("Active queue %d before swap => ", i);
    printQueue(active[i]);
    printf("Expired queue %d before swap => ", i);
    printQueue(expired[i]);
    swapQueues();
    printf( "Dequeuing %d from active...\n", dequeue( active[i]) );
    printf("Active queue %d after swap => ", i);
    printQueue(active[i]);
    printf("Expired queue %d after swap => ", i);
    printQueue(expired[i]);
}

*************************************************************************/