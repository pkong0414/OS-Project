//sharedheap.h

#ifndef SHAREDHEAP_H
#define SHAREDHEAP_H

struct sharedheap {
    int id;
//    int count;
//    int shmflg;
    char **sharedString;

}typedef sharedheap;

#endif