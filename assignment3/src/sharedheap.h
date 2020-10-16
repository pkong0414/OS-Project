//sharedheap.h

#include <signal.h>

#ifndef SHAREDHEAP_H
#define SHAREDHEAP_H

typedef struct sharedheap {
    sig_atomic_t ready;
    char *sharedString;
    int *sharedQueue;
    char *choosing;
    int ticket;

}sharedheap;

#endif