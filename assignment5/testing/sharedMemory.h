//sharedMemory.h
//

#include <signal.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdbool.h>

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

/* Resources block structure */
typedef struct {
    int totalResources[10];               //this is initialized randomly from 1-10
    int allocatedResources[10];           //this is the resources allocated at the moment
} SysResources;

typedef struct {
    //allocated will keep track of resources from
    //the 2 SysResource sets
    int maxClaim[10];  //max claims will be initialized
    int allocated[10][2]; //resources held
    int request[10];      //request claims will be dictated by allocated resources
} Proc_Resource;

/* Proccess table and sys time shared block */
typedef struct {

    //we will link both Proc_Resource and PCB by index.
    PCB processBlock[18];
    Proc_Resource procResource[18];

    SysResources rSet1;
    SysResources rSet2;
    bool inProcess;
    struct timespec sysTime;
    int childCount;
} sharedMem;

#endif