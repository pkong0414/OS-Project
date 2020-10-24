//sharedheap.h

#include <signal.h>
#include <semaphore.h>
#include <pthread.h>

#ifndef SHAREDHEAP_H
#define SHAREDHEAP_H

typedef struct sharedheap {
    sig_atomic_t ready;
    char sharedString[700];
    sem_t semMem;
    int *sharedQueue;
    char *choosing;
    int ticket;

}sharedheap;

#endif