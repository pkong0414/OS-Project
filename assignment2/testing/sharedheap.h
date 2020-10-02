//sharedheap.h

#ifndef SHAREDHEAP_H
#define SHAREDHEAP_H

typedef struct sharedheap {
    int id;
    int count;
    double sum;
    char *sharedaddress;

} sharedheap;

#endif