//sharedMemory.h
//

#include <signal.h>
#include <semaphore.h>
#include <pthread.h>

#ifndef SHAREDMEMORY_H
#define SHAREDMEMORY_H

/* Time structure */
typedef struct {
    //we'll be looking at this unit mainly as quantum ( 1ms )
    int ns;
    int ms;
} Time;

/* Proccess block structure */
typedef struct {
    int realTime;       //flag if process is realtime or not
    int queueID;        //flag for what queue the process is in
    int pid;            //real pid
    int localPid;        //local pid
    Time procCpuTime;      //cpu time used statistic
    Time procSysTime;      //time in system statistic
    Time procBlockedTime;  //time blocked statistic
    Time procWaitTime; 	//time waiting statistic
} PCB;

/* Proccess table and sys time shared block */
typedef struct {
    PCB processBlock[18];
    Time sysTime;
    int childCount;
} sharedMem;

#endif