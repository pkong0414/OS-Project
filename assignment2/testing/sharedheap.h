//sharedheap.h

#include <signal.h>

#ifndef SHAREDHEAP_H
#define SHAREDHEAP_H

typedef struct sharedheap {
    int id;
    int count;
    int shmflg;
    sig_atomic_t ready;
    char *sharedString;

}sharedheap;

#endif