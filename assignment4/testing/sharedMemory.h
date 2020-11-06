//sharedMemory.h
//

#include <signal.h>
#include <semaphore.h>
#include <pthread.h>

#ifndef SHAREDMEMORY_H
#define SHAREDMEMORY_H


/* Proccess block structure */
typedef struct {
    int state;          //state of the current working process.
                        //2 blocked 1 processing, 0 waiting, -1 not a process
    int queueID;        //flag for what queue the process is in
    int pid;            //real pid
    int localPid;        //local pid
    struct timespec procCpuTime;      //cpu time used statistic
    struct timespec procSysTime;      //time in system statistic
    struct timespec procBlockedTime;  //time blocked statistic
    struct timespec procWaitTime; 	//time waiting statistic
} PCB;

/* Proccess table and sys time shared block */
typedef struct {
    PCB processBlock[18];
    struct timespec sysTime;
    int childCount;


} sharedMem;

#endif