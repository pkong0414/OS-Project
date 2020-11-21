// oss.c
// Patrick Kong
// 10/26/20

/*
 *
 * TODO:
 * DONE: Start by creating a Makefile that compiles and builds the two executables: oss and user_proc.
 * Have oss create a process control table with one user process (of real-time class) to verify it is working
 * Schedule the one user process over and over, logging the data
 * Create the round robin queue, add additional user processes, making all user processes alternate in it
 * Keep track of and output statistics like throughput, idle time, etc
 * Implement an additional user class and the multi-level feedback queue.
 * Add the chance for user processes to be blocked on an event, keep track of statistics on this
 *
 */

#include "detachandremove.h"
#include "sharedMemory.h"
#include "queue.h"
#include <stdbool.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <semaphore.h>
#include <pthread.h>
#include <time.h>

//macros
#define PERM (S_IRUSR | S_IWUSR)
//all of these units are relative to nanoseconds
#define MILLISECOND 1000000
#define SECOND 1000000000
#define MICROSECOND 1000

// structure for message queue
static struct mesg_buffer {
    pid_t mesg_pid;
    long mesg_type;
    char mesg_text[100];
} message;

//function prototypes
void printhelp();
void initSharedMem();
void initProcessState();
void initProcessResources();
void scheduler();
int detachShared();
void initMsgQueue();
int findEmptyPB();
void initProcBlocks();
void printPCB();
int findPCBIndex( pid_t pid );
int printProcessState();
int queueProcessing(pid_t pid, int requestType);
void printResourceStatus();
void printStatus();

//output file global
FILE *outFilePtr1;

//constants global
const int BASE_QUANTUM = 10000000; //10 ms for the base which is 10000000 ns
const int NANOS_PER_TEN_MICRO = 10000;
const int MILLISEC_PER_SEC = 1000;
const int MAX_TIME_BETWEEN_NEW_PROCS_NS = 150000;
const int MAX_TIME_BETWEEN_NEW_PROCS_SEC = 1;
const int RT_PERCENT = 80;
bool verbose = false;

//this value will take in seconds after which the process terminate, even if it has not finished.
static int tValue = 5;
static int processCount = 0;
static int maxChildren = 18;
static int localPid = 0;

//Time until next execution
struct timespec nextExec;


//timing statistics
struct timespec totalCpuTime = {0, 0};
struct timespec totalWaitTime = {0, 0};
struct timespec totalBlockedTime = {0, 0};
struct timespec totalTime = {0, 0};
struct timespec idleTime = {0, 0};

//shared memory section
key_t myKey;
key_t key;
int msgid;
static int shm_id;
static sharedMem *procControl;

//message queue globals
key_t userKey;
key_t masterKey;
int toUserID;
int toMasterID;

//queue global
static Queue *resourceRequestQ;


/* ARGUSED */
static void myhandler( int s ) {
    char timeout[] = "./oss handler: timing out processes.\n";
    int timeoutSize = sizeof( timeout );
    int errsave;

    errsave = errno;
    write(STDERR_FILENO, timeout, timeoutSize );
    errno = errsave;
    int i;

    //detaching shared memory
    if( detachShared() == 0 ) {
        printf( "./oss: everything is finished... exiting program\n");
    }
    else {
        printf( "unable to detach shared memory... please check outside of the program\n" );
    }

    //destroying message queues
    msgctl(toUserID, IPC_RMID, NULL);
    msgctl(toMasterID, IPC_RMID, NULL);

    //closing file
    fclose(outFilePtr1);

    printf( "./oss: destroyed message queues\n");
    exit(0);
}

static void myKillSignalHandler( int s ){
    char timeout[] = "./oss: caught ctrl+c, ending processes.\n";
    int timeoutSize = sizeof( timeout );
    int errsave;

    errsave = errno;
    write(STDERR_FILENO, timeout, timeoutSize );
    errno = errsave;
    int i;

    //waiting for max amount of children to terminate
    for (i = 0; i <= maxChildren ; ++i) {
        wait(NULL);
    }

    //detaching shared memory
    if( detachShared() == 0 ) {
        printf( "./oss: everything is finished... exiting program\n");
    }
    else {
        printf( "./oss: unable to detach shared memory... please check outside of the program\n" );
    }

    //destroying message queues
    msgctl(toUserID, IPC_RMID, NULL);
    msgctl(toMasterID, IPC_RMID, NULL);

    //closing file
    fclose(outFilePtr1);

    printf( "./oss: destroyed message queues\n");
    exit(0);
}

static int setupUserInterrupt( void ){
    struct sigaction act;
    act.sa_handler = myKillSignalHandler;
    act.sa_flags = 0;
    return (sigemptyset(&act.sa_mask) || sigaction(SIGINT, &act, NULL));
}

static int setupinterrupt( void ){
    struct sigaction act;
    act.sa_handler = myhandler;
    act.sa_flags = 0;
    return (sigemptyset(&act.sa_mask) || sigaction(SIGALRM, &act, NULL));
}

static int setupitimer(int tValue) {
    struct itimerval value;
    value.it_interval.tv_sec = tValue;
    value.it_interval.tv_usec = 0;
    value.it_value = value.it_interval;
    return (setitimer( ITIMER_REAL, &value, NULL));
}

int main( int argc, char ** argv ) {
    //interrupt signal
    if( setupUserInterrupt() == -1 ){
        perror( "./oss: failed to set up a user kill signal.\n");
        return 1;
    }

    int c;
    char *outPath = "output.log";

    //command line parser
    while ( ( c = getopt(argc, argv, "hv" ) ) != -1 ) {
        switch (c) {
            case 'h':
                printhelp();
                return 0;
            case'v':
                verbose = true;
                break;
            default:
                break;
        }
    }

    //***************************** Resetting log files ***********************************
    outFilePtr1 = fopen("./output.log", "w");
    //*************************************************************************************

    //setting up interrupts
    if (setupinterrupt() == -1) {
        perror("./oss: Failed to set up handler for SIGALRM");
        return 1;
    }

    if (setupitimer(tValue) == -1) {
        perror("./oss: Failed to set up the ITIMER_PROF interval timer");
        return 1;
    }


    //initializing shared memory
    printf("initializing shared memory\n");
    initSharedMem();

    //initializing message queue
    printf("initializing message queue\n");
    initMsgQueue();

    //initializing process queue
    printf("initializing process queue\n");
    resourceRequestQ = createQueue( 18 );

    //initializing process blocks
    printf("initializing process blocks\n");
    initProcBlocks();

    //initializing process states
    printf("initializing process states\n");
    initProcessState();

    //initializing resources
    printf("initializing resources\n");
    initProcessResources();

    //TODO: we'll be making the scheduler create a PCT and one user process (of real time).
    //scheduler will make the process control happen.
    scheduler();

    return 0;
}

void printhelp(){
    printf("This is a program that will simulate resource management with processes\n");
    printf("Banker's Algorithm will be used to prevent deadlock\n");
    printf("INVOCATION:\n");
    printf("./oss\n");
}

//function to initialize shared memory
void initSharedMem(){
    //grabbing key for shared memory
    if( ( myKey = ftok( "sharingIsCaring", 1 ) ) == (key_t)-1 ) {
        fprintf(stderr, "Failed to derive key from filename %s:%s\n", "sharingIsCaring", strerror( errno ) );
        return;
    }

    //shared memory creation
    shm_id = shmget( myKey, 500, PERM | IPC_CREAT | IPC_EXCL);

    if( shm_id == -1 ){
        if(errno != EEXIST){
            perror( "shared memory already exist.\n");

        } else if ( (shm_id = shmget( myKey, sizeof(sharedMem), PERM ) ) == -1) {
            perror( "shared memory exist, just can't control it\n");
        }
    } else {
        printf("created a new memory segment\n");
        procControl = shmat(shm_id, NULL, 0);
        if (procControl == (void *) -1) {
            perror("shmat sharedMem failed");
        } else {
            printf("shmat sharedMem returned %#8.8x\n", procControl);
        }
    }
    return;
}

void scheduler() {
    //This function will handle the main operations of oss
    //In this case, we will apply banker's algorithm to schedule processes in a safe manner.
    pid_t childPid;
    int index;
    int concurrentProc = 0;
    int messageStatus;
    int currentProcPid;
    int diceRoll;
    int status;
    int dequeueResult;
    struct timeval tStart;
    struct timeval tEnd;
    struct timeval nextExecStart;
    struct timeval nextExecEnd;
    long tDiff;

    procControl->sysTime.tv_nsec = 0;
    procControl->inProcess = false;
    //setting up the nanosleep timer
    // we need to set this so that sleep till be in terms of quantum
    // or 1000000 nanosecs

    //seeding random number generator
    srand(time(0));

    gettimeofday(&tStart, NULL);

    nextExec.tv_nsec = MAX_TIME_BETWEEN_NEW_PROCS_NS;
    //debugging output
    printf( "nextExec.tv_nsec: %11d\n", nextExec.tv_nsec);
    procControl->sysTime.tv_nsec = 0;

    while(1) {
        gettimeofday(&tEnd, NULL);
        tDiff = (tEnd.tv_usec - tStart.tv_usec) * MICROSECOND;
        gettimeofday(&nextExecStart, NULL);
        procControl->sysTime.tv_nsec = tDiff;

        //normally we only allow 18 processes (maxChildren) but for now we'll leave it at 1
        //processCount should be limited to 40 but for now we'll have it be 1 in this case

        //I think the system time should be kept in nanoseconds and converted accordingly
        //incrementing the system time
        if( (concurrentProc < 18) && (nextExec.tv_nsec >= MAX_TIME_BETWEEN_NEW_PROCS_NS ) ) {
            if( ( index = findEmptyPB() ) != -1 ) {
                childPid = fork();
                if (childPid < 0) {
                    perror("failed to fork");
                }

                if (childPid == 0) {
                    //*************************** Child Process ***********************************
                    printf("\nMy PID = %d\n", (int) getpid());
                    printf("\n");

                    char *arguments[] = {"./user", NULL};

                    //fork success now exec here
                    printf("execing...\n");
                    execv(arguments[0], arguments);

                    //if exec failed this will print out
                    perror("exec failed\n");
                    myhandler(1);
                }
                //*************************** process handling section ***************************
                //first process is created let's initialize our nextExec
                if (index >= 0) {
                    nextExec.tv_nsec = 0;
                    gettimeofday(&nextExecStart, NULL);
                    printf("./oss childPid: %d\n", childPid);
                    currentProcPid = childPid;
                    printf("./oss: currentProcPid: %d\n", currentProcPid);
                    procControl->processBlock[index].pid = currentProcPid;

                    //determining if process is a realtime
                    diceRoll = (rand() % 100);

                    //assigning RT to process if it rolls 80 or higher.
                    //RT_PERCENT use this later
                    if (diceRoll >= RT_PERCENT) {
                        procControl->processBlock[index].queueID = 0;
                    } else {
                        procControl->processBlock[index].queueID = 1;
                    }

                    //initializing everything
                    procControl->processBlock[index].state = 0;
                    procControl->processBlock[index].pid = childPid;
                    procControl->processBlock[index].localPid = processCount++;
                    procControl->processBlock[index].procCpuTime.tv_nsec = 0;
                    procControl->processBlock[index].procCpuTime.tv_sec = 0;
                    procControl->processBlock[index].procSysTime.tv_nsec = 0;
                    procControl->processBlock[index].procSysTime.tv_sec = 0;
                    procControl->processBlock[index].procBlockedTime.tv_nsec = 0;
                    procControl->processBlock[index].procBlockedTime.tv_sec = 0;
                    procControl->processBlock[index].procWaitTime.tv_nsec = 0;
                    procControl->processBlock[index].procWaitTime.tv_sec = 0;

                    //this will put the newly exec'd proccess in queue and in wait
                    procControl->processBlock[index].state = 0;
                    printf("resourceRequestQ line size: %d\n", resourceRequestQ->currentSize);
                    printProcessState();

                    //outputting to file
                    fprintf(outFilePtr1, "./oss: Process created at: %ld, PID:%d, localPid: %d, Queue:%d \n",
                            procControl->sysTime.tv_nsec,
                            procControl->processBlock[index].pid,
                            procControl->processBlock[index].localPid,
                            procControl->processBlock[index].queueID);

                    //incrementing concurrent processes
                    concurrentProc++;
                } else {
                    //killing process when there's no room in PCB
                    printf("no room in the PCB, killing process...\n");
                    kill(childPid, SIGTERM);
                }
            }


        }

        if ((messageStatus = msgrcv(toMasterID, &message, sizeof(message),
                                    message.mesg_type, 0) > -1)) {
            printf("./oss: PID: %d, message type: %d, message: %s\n", message.mesg_pid, message.mesg_type, message.mesg_text);
            if (strcmp(message.mesg_text, "PROC_TERM") == 0) {
                //receiving messages of termination
                printf("./oss PID: %d has terminated!\n", message.mesg_pid);
                //handle the termination stuff here.
            }
            if (strcmp(message.mesg_text, "REQ_RESOURCE") == 0) {
                //receiving resource request message from a process
                printf("./oss: message received from %d: %s\n", message.mesg_pid, message.mesg_text);
                currentProcPid = message.mesg_pid;
                index = findPCBIndex(currentProcPid);
                //debugging output
                printf("index: %d\n", index);
                printf("currentProcPid: %d\n", currentProcPid);

                queueProcessing( currentProcPid , 1 );

                //sending checking in message to master
                printf("procControl->processBlock[ %d ].queueID: %d\n",
                       index, procControl->processBlock[index].queueID);
                printStatus();
            }
            if (strcmp(message.mesg_text, "REL_RESOURCE") == 0) {
                //receiving resource release message from a process
                printf("./oss: message received from %d: %s\n", message.mesg_pid, message.mesg_text);
                currentProcPid = message.mesg_pid;
                index = findPCBIndex(currentProcPid);
                queueProcessing( currentProcPid , 2 );
                //sending checking in message to master
                printf("procControl->processBlock[ %d ].queueID: %d\n",
                       index, procControl->processBlock[index].queueID);
            }
        }

//        if ((messageStatus = msgrcv(toMasterID, &message, sizeof(message),
//                                        message.mesg_type, MSG_NOERROR | IPC_NOWAIT) > -1)) {
//
        int status;
        pid_t pid;

        /**************************** PROCESS TERMINATION ****************************/
        if ((pid = waitpid((pid_t) -0, &status, WNOHANG)) > 0) {
            //if a PID is returned meaning the child died

            if (WIFEXITED(status)) {

                if (WEXITSTATUS(status) == 14) {
                    //receiving termination message from a process
                    currentProcPid = pid;
                    index = findPCBIndex(currentProcPid);
                    printf("currentProcPid: %d\n", currentProcPid);
                    printf("process is terminating...\n");

                    //if a PID is returned meaning the child died
                    concurrentProc--;
                    printf("processCount: %d\n", processCount);
                    printf("concurrentProc: %d\n", concurrentProc);
                    printf("currentProcIndex: %d\n", index);

                    //returning allocated resources back to SysResources
                    int i;
                    for( i = 0; i < 10; i++ ) {
                        //returning resources to rSet1 first
                        procControl->rSet1.totalResources[i] +=
                                procControl->procResource[index].allocated[i][0];
                        //returning resources to rSet2 now
                        procControl->rSet2.totalResources[i] +=
                                procControl->procResource[index].allocated[i][1];
                    }

                    //taking the localPid tag off of the processBlock
                    if (currentProcPid > -1) {
                        --concurrentProc;
                        procControl->inProcess = false;
                        procControl->processBlock[index].state = 0;
                        procControl->processBlock[index].localPid = -1;
                        procControl->processBlock[index].pid = -1;
                        printf("PID returned. Post terminate status report: \n");
                        printPCB();
                        printStatus();
                    }
                }
            }
        }



        //******************* end of the while loop **************************
        //calculating to increment sysTime on the PCT
        gettimeofday(&tEnd, NULL);
        tDiff = (tEnd.tv_usec - tStart.tv_usec) * MICROSECOND;
        procControl->sysTime.tv_nsec += tDiff;
        //calculating to increment time to nextExec
        tDiff = (tEnd.tv_usec - nextExecStart.tv_usec) * MICROSECOND;
        nextExec.tv_nsec += tDiff;
        //printf( "nextExec.tv_nsec: %ld\n", nextExec.tv_nsec);

        if( concurrentProc < 1) {
            nextExec.tv_nsec = MAX_TIME_BETWEEN_NEW_PROCS_NS;
        }
        if(processCount == 40){
            myhandler(1);
            exit(0);
        }
    }

    //we need to make a user process.

    //we want to make a message queue for the processes running.

    //destroying message queues
    printf("program is shutting down.\n");
    myhandler(1);
    printf( "./oss: destroyed message queues\n");

    return;
}

int detachShared(){

    //detaching shared memory
    if (detachandremove(shm_id, procControl) == -1) {
        perror("failed to destroy shared memory segment\n");
        return -1;
    } else {
        printf("destroyed shared memory segment\n");
    }
    return 0;
}

//function to initialize message queue
void initMsgQueue(){
    // ftok to generate unique key
    //this one will be used for the processing to master queues
    userKey = ftok("sharedMsg1", 123);

    if (userKey == -1) { //checking if we derived a key
        printf("failed to derive a key for userOutKey\n");
        return;
    }

    // msgget creates a message queue
    // and returns identifier
    toUserID = msgget(userKey, 0666 | IPC_CREAT);

    if (toUserID == -1) { //checking see if msgget worked
        printf("failed to attach userOutKey\n");
        return;
    }

    //ftok to generate unique key
    //this one will be use for receiving messages from the master
    masterKey = ftok("sharedMsg2", 124);

    if (masterKey == -1) { //checking if we derived a key
        printf("failed to derive a key for masterInKey\n");
        return;
    }

    //msgget to generate another message queue
    toMasterID = msgget(masterKey, 0666 | IPC_CREAT);

    if (toMasterID == -1) { //checking see if msgget worked
        printf("failed to attach masterInKey\n");
        return;
    }
}

void initProcessResources(){
    int i;
    for( i = 0; i < 10; i++){
        procControl->rSet1.totalResources[i] = 10;
        procControl->rSet2.totalResources[i] = 10;
    }
}

/* Find the next empty proccess block. Returns proccess block position if one is available or -1 if one is not */
int findEmptyPB(){
    int i;
    printf( "procControl->processBlock:\n");
    for (i = 0; i < maxChildren; i++)
    {
        if (procControl->processBlock[i].pid == -1)
            //return an empty index proccess table
            return i;
    }
    //we found nothing in this case.
    return -1;
}

void printPCB(){
    int i;
    printf( "Process control block:\n");
    for( i = 0; i < maxChildren; i++) {
        printf("%d ", procControl->processBlock[i].pid);
    }

    printf( "\n" );
}

/* Initializing Process Blocks*/
void initProcBlocks(){
    int i;
    for (i = 0; i < maxChildren; i++)
        procControl->processBlock[i].pid = -1;
}

void initProcessState(){
    int i;
    for( i = 0; i < maxChildren; i++)
        procControl->processBlock[i].state = -1;
}

int findPCBIndex(pid_t pid){
    int i;
    for( i = 0; i < maxChildren; i++ ){
        if(procControl->processBlock[i].pid == pid)
            return i;
    }
    //if we find nothing
    return -1;
}

int printProcessState(){
    int clear = 0;
    int i;
    printf("state: \n");
    for( i = 0; i < maxChildren; i++ ){
        printf("%d ", procControl->processBlock[i].state);
        if( procControl->processBlock[i].state != 0 )
            clear = 1;
    }

    printf("\n");
    return clear;
}

int queueProcessing( pid_t pid , int requestType ) {
    //this queue will perform banker's algorithm

    //this will consist of request types
    //any requestType not 1 will not be asking to enqueue the pid.
    //enqueuing pid first before we handle request
    printf("inside queueProcessing\n");

    printResourceStatus();
    pid_t currentProcPid;
    int count = 0;
    int dequeueResult;
    int index;                           //this will hold the index of the pid
    int resourceIndex;                      //this will hold the index of the resource requested
    int resourceRequestActual1;             //this will hold the value requested of the first resource set
    int resourceRequestActual2;             //this will hold the value requested of the second resource set
    printStatus();
    int requestResult1;
    int requestResult2;

    if(requestType == 1) {
        printf("enqueuing %d into resourceRequestQ\n", pid);
        enqueue(resourceRequestQ, pid);
    }
    //this will handle the case of terminated process
    //as well as cases of empty queues.
    while( count < resourceRequestQ->currentSize ) {
        printf("./oss: checking queues now\n");
        //we'll need to communicate with sharedMem in order to figure out which processes can proceed.


        //we'll need to dequeue the process first then apply banker's algorithm
        if (((dequeueResult = dequeue(resourceRequestQ)) == -1)) {
            printf("there's nothing in queue 0\n");
            return 0;
        } else {
            //assigning results to currentProcPid
            currentProcPid = dequeueResult;

            //we've found a process to dequeue so we'll apply it here
            index = findPCBIndex(currentProcPid);
            resourceIndex = findResourceRequestIndex(index);
            if (resourceIndex == -1) {
                //error case, where we have a process sitting in queue that isn't requesting
                //for any resources.
                printf("ERROR: a process has been enqueued and isn't asking for any resources\n");
                exit(1);
            }
            //we might not have to subtract request - allocate since bounds are determined
            //by the user process
            resourceRequestActual1 = procControl->procResource[index].request[resourceIndex];

            //trying to see if there's enough resources to grant to a process
            requestResult1 = procControl->rSet1.totalResources[resourceIndex]
                    - resourceRequestActual1;

            if (requestResult1 >= 0) {
                //this means we have enough resources. Let's get this process going.
                printf("./oss: there's enough resources for this process, sending APPROVE message to PID: %d\n",
                       currentProcPid);
                message.mesg_type = currentProcPid+2;
                message.mesg_pid = currentProcPid;
                strcpy(message.mesg_text, "APPROVE");
                //printf("./oss: sending pid:%d message: %s. messageType: %d\n", currentProcPid, message.mesg_text,
                       //message.mesg_type);
                //sending APPROVE message to ./user

                //taking resources away from system resources
                procControl->rSet1.totalResources[resourceIndex]
                -= resourceRequestActual1;

                //we should also allocate the resources to that said process
                procControl->procResource[index].allocated[resourceIndex][0]
                += procControl->procResource[index].request[resourceIndex];

                //resource has been distributed. Let's reset the request
                procControl->procResource[index].request[resourceIndex] = 0;

                if (msgsnd(toUserID, &message, sizeof(message), 0) == -1) {
                    perror("send_message");
                }
            } else if (requestResult1 < 0) {
                //we are dipping into the second set of resources.
                //we will be adding since request result will be in the negative
                requestResult2 = procControl->rSet2.totalResources[resourceIndex]
                                + requestResult1;
                if (requestResult2 >= 0) {
                    //looks like second set of resource type can cover. Let's get this process going.
                    printf("./oss: there's enough resources for this process, sending APPROVE message to PID: %d\n",
                           currentProcPid);
                    message.mesg_type = currentProcPid+2;
                    message.mesg_pid = currentProcPid;
                    strcpy(message.mesg_text, "APPROVE");
                    //printf("./oss: sending pid:%d message: %s. messageType: %d\n", currentProcPid, message.mesg_text,
                           //message.mesg_type);
                    //sending APPROVE message to ./user

                    //taking resources away from system resources
                    //requestResult1 is basically the left over amount we need to cover.
                    resourceRequestActual2 = abs( requestResult1 );

                    //in this case we'll be taking resources from SysResource set1 first
                    procControl->rSet1.totalResources[resourceIndex]
                            -= procControl->rSet1.totalResources[resourceIndex];

                    //now we take the rest of what we need from SysResource set2.
                    procControl->rSet2.totalResources[resourceIndex]
                            -= resourceRequestActual2;

                    //we should also allocate the resources to that said process
                    procControl->procResource[index].allocated[resourceIndex][0] +=
                            ( procControl->procResource[index].request[resourceIndex]
                            + requestResult1 );
                    //adding to allocated SysResources set 2 now
                    procControl->procResource[index].allocated[resourceIndex][1] +=
                            ( resourceRequestActual2 );

                    //resource has been distributed. Let's reset the request
                    procControl->procResource[index].request[resourceIndex] = 0;

                    if (msgsnd(toUserID, &message, sizeof(message), 0) == -1) {
                        perror("send_message");
                    }
                } else {
                    //we cannot cover the resource expenditure
                    //process will have to wait till more resources have opened up.
                    printf("There isn't enough resources. Unsafe state from resource request: DENIED.\n");
                    printf("Enqueuing process: %d\n", currentProcPid);
                    enqueue(resourceRequestQ, currentProcPid);
                    printQueue(resourceRequestQ);
                }
            }
        }
        //incrementing count to line the next process in queue up.
        count++;
    }
}

void printStatus() {
    printProcessState();
    printf( "resourceRequest: \n");
    printQueue( resourceRequestQ );

}

void printResourceStatus() {
    //This should print the resources status currently used by all the processes
    int i;
    int procIndex;
    printf("Total Resources: \n");
    for(i = 0; i < 10; i++) {
        printf("%d ", i);
    }
    printf("\n");
    for(i = 0; i < 10; i++){
        printf( "%d ", procControl->rSet1.totalResources[i]);
    }
    printf("\n");
    for(i = 0; i < 10; i++){
        printf( "%d ", procControl->rSet2.totalResources[i]);
    }
    printf("\n\n");
    for(procIndex = 0; procIndex < 18; procIndex++) {
        if( procControl->processBlock[procIndex].pid > 0) {
            printf("Proc %d allocated: \n", procControl->processBlock[procIndex].pid);
            //first set of allocation
            for (i = 0; i < 10; i++) {
                printf("%d ", procControl->procResource[procIndex].allocated[i][0]);
            }
            printf("\n");
            //second set of allocation
            for (i = 0; i < 10; i++) {
                printf("%d ", procControl->procResource[procIndex].allocated[i][2]);
            }
            printf("\n");
        }
    }

}

int findResourceRequestIndex( int index ) {
    //we'll be using the index to look through that particular processes' resource
    //This is all to find the specific resource it is requesting for.
    int i = 0;

    //we are only looking through the request from Proc_Resource
    for( i; i < 10; i++){
        if( procControl->procResource[index].request[i] > 0){
            return i;
        }
    }
    //the case we don't find any resource request
    return -1;
}
