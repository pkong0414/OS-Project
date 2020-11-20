// queue.c
// NOTE: Portion of the queue was taken from website codesdope's article making a queue using an array.

#include "queue.h"
#include <stdio.h>
#include <stdlib.h>

//macro for max size
#define MAXSIZE 18

//queue global
Queue *resourceRequestQ;
Queue *qset1[4];
Queue *qset2[4];
Queue *blocked;
Queue **active;
Queue **expired;

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
    queue->front = -1;
    queue->currentSize = 0;
    queue->rear = -1;  // This is important, see the enqueue
    queue->line = (int*) malloc(queueSize * sizeof(int));
    return queue;
}

void enqueue(Queue *queue, int value) {
    //idea is to assign to the appropriate based on priority.
    //we don't need to use priority as we can just set the relevant array in the oss part of the code
    if(queue->currentSize == queue->queueSize) {
        printf("Queue is full\n");
        return;
    }

    if(queue->currentSize == 0) {
        //this case we have an empty queue.
        queue->line[0] = value;
        queue->front = queue->rear = 0;
        queue->currentSize++;
    } else if(queue->rear == MAXSIZE-1) {
        //this is the case where the queue is almost full
        //by assigning the rear to the beginning and giving value
        // to the first of the line makes it circular
        queue->line[0] = value;
        queue->rear = 0;
        queue->currentSize++;
    } else {
        //this is the normal case for the queue
        queue->line[queue->rear+1] = value;
        queue->rear++;
        queue->currentSize++;
    }
}

int dequeue(Queue *queue)
{
    if (queue->currentSize <= 0)
        return -1;
    int item = queue->line[queue->front];
    queue->front = (queue->front + 1) % queue->queueSize;
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
    int count = 0;
    if( queue->front > queue->rear ){
        //this will consider the circular case
        for(i = queue->front; i < MAXSIZE; i++) {
            printf("%d ", queue->line[i]);
            count++;
        }
        //now we will print the end of the loop
        for(i = queue->rear; count < queue->currentSize; i++) {
            printf("%d ", queue->line[i]);
            count++;
        }
    } else {
        for (i = queue->front; i <= queue->rear; ++i) {
            printf("%d ", queue->line[i]);
        }
    }
    printf("\n");
}

/*********************  testing purpose only *****************************/
void main() {
    createQueues();
    int i = 3;
    //this should give me an error
    printf( "Dequeuing %d from active...\n", dequeue( active[i]) );
    enqueue(resourceRequestQ, 1);
    enqueue(resourceRequestQ, 2);
    enqueue(resourceRequestQ, 1);
    enqueue(resourceRequestQ, 2);
    enqueue(resourceRequestQ, 3);
    enqueue(resourceRequestQ, 4);
    enqueue(resourceRequestQ, 3);
    enqueue(resourceRequestQ, 4);
    printf("Active queue %d before swap => ", i);
    printQueue(resourceRequestQ);
    printf("Expired queue %d before swap => ", i);
    printQueue(resourceRequestQ);
    swapQueues();
    printf( "Dequeuing %d from active...\n", dequeue( resourceRequestQ) );
    printf("Active queue %d after swap => ", i);
    printQueue(resourceRequestQ);
    printf("Expired queue %d after swap => ", i);
    printQueue(resourceRequestQ);
}

/*************************************************************************/