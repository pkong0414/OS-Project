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
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/msg.h>
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
struct mesg_buffer {
    long mesg_type;
    pid_t mesg_pid;
    char mesg_text[100];
} message;

//function prototypes
void printhelp();
void initSharedMem();
void scheduler();
int detachShared();
void initMsgQueue();
int findEmptyPB();
void initProcBlocks();
void printPCB();
int findPCBIndex( pid_t pid );
int printProcessState();
int queueProcessing( pid_t *currentProcPid );
void tellProcessResume( pid_t pid );
void printStatus( pid_t currentProcPid);

//output file global
FILE *outFilePtr1;

//constants global
const int BASE_QUANTUM = 10000000; //10 ms for the base which is 10000000 ns
const int NANOS_PER_TEN_MICRO = 10000;
const int MILLISEC_PER_SEC = 1000;
const int MAX_TIME_BETWEEN_NEW_PROCS_NS = 150000;
const int MAX_TIME_BETWEEN_NEW_PROCS_SEC = 1;
const int RT_PERCENT = 80;

//this value will take in seconds after which the process terminate, even if it has not finished.
static int tValue = 10;
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
Queue *qset1[4];
Queue *qset2[4];
Queue *blocked;
Queue **active;
Queue **expired;


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
    while ( ( c = getopt(argc, argv, "h" ) ) != -1 ) {
        switch (c) {
            case 'h':
                printhelp();
                return 0;
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
    createQueues();

    //initializing process blocks
    printf("initializing process blocks\n");
    initProcBlocks();

    //initializing process states
    printf("initializing process states\n");
    void initProcessState();

    //Queue max time limit. 3rd queue is infinity with 1.25 ms
    //this isn't an actual time limit since RT processes maintain priority 0
    int queueTimeLimit0 = ( BASE_QUANTUM ) * MILLISECOND;
    int queueTimeLimit1 = ( BASE_QUANTUM / 2) * 2 * MILLISECOND;
    int queueTimeLimit2 = ( BASE_QUANTUM / 4) * 3 * MILLISECOND;

    //this isn't an actual time limit since you cannot move down another queue
    int queueTimeLimit3 = ( BASE_QUANTUM / 8) * MILLISECOND;

    //TODO: we'll be making the scheduler create a PCT and one user process (of real time).
    //scheduler will make the process control happen.
    scheduler();

    return 0;
}

void printhelp(){
    printf("This is a program that will simulate processes in a queue\n");
    printf("The scheduling will be in round robin.\n");
    printf("Base quantum will be 10ms and the quantum will be halved at each queue level (4 level Queue)\n");
    printf("The program will terminate after 3 seconds have passed or 100 processes have been made.\n");
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

    long allowedQuantum0 = 10 * MILLISECOND;
    long allowedQuantum1 = allowedQuantum0 / 2;
    long allowedQuantum2 = allowedQuantum1 / 2;
    long allowedQuantum3 =  allowedQuantum2 /2;

    //initializing max time limits for the queues
    long maxQueueLimit1 = ( 10* MILLISECOND );        //10 ms
    long maxQueueLimit2 = (( maxQueueLimit1 / 4 ) * 3 );  //2.5 * 3 = 7.5 msg

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
        //processCount should be limited to 100 but for now we'll have it be 1 in this case

        //I think the system time should be kept in nanoseconds and converted accordingly
        //incrementing the system time
        if( ( processCount < 100 && concurrentProc < 18) && (nextExec.tv_nsec >= MAX_TIME_BETWEEN_NEW_PROCS_NS ) ) {
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
                    if (procControl->processBlock[index].queueID == 0) {
                        procControl->processBlock[index].state = 0;
                        enqueue(active[0], procControl->processBlock[index].pid);
                        printf("active[0] line size: %d\n", active[0]->currentSize);
                        printf("active[0] line:\n");
                        printQueue(active[0]);
                        printProcessState();
                    } else {
                        procControl->processBlock[index].state = 0;
                        enqueue(active[1], procControl->processBlock[index].pid);
                        printf("active[1] line size: %d\n", active[1]->currentSize);
                        printf("active[1] line:\n");
                        printQueue(active[1]);
                        printProcessState();
                    }

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

        //for now let's leave it, it seems the process has to communicate to oss on waiting.
        int priorityCases = procControl->processBlock[index].queueID;
        //exit process fix this later
        //receiving message that a process wants to terminate itself
//        printf("./oss: currentProcPid: %d\n", currentProcPid);
//        printProcessState();
//        printf( "active 0: \n");
//        printQueue( active[0] );
//        printf( "active 1: \n");
//        printQueue( active[1] );
//        printf( "active 2: \n");
//        printQueue( active[2] );
//        printf( "active 3: \n");
//        printQueue( active[3] );
//        printf( "expired 1: \n");
//        printQueue( expired[1] );
//        printf( "expired 2: \n");
//        printQueue( expired[2] );
//        printf( "expired 3: \n");
//        printQueue( expired[3] );
//        printPCB();
//        printf("inProcess: %d\n", procControl->inProcess);
            if ((messageStatus = msgrcv(toMasterID, &message, sizeof(message),
                                        currentProcPid, 0) > -1)) {
                printf("./oss: PID: %d, message: %s\n", message.mesg_type, message.mesg_text);
                if (strcmp(message.mesg_text, "PROC_TERM") == 0) {
                    //receiving messages of termination
                    printf("./oss PID: %d has terminated!\n", message.mesg_type);
                    //handle the termination stuff here.
                }
                if (strcmp(message.mesg_text, "PROC_WAIT") == 0) {
                    //receiving termination message from a process
                    printf("./oss WAIT: message received from %d: %s\n", message.mesg_type, message.mesg_text);
                    printf("process is waiting\n");
                    printf("checking queue now to put into processing");
                    currentProcPid = message.mesg_type;
                    index = findPCBIndex(currentProcPid);
                    priorityCases = procControl->processBlock[index].queueID;
                    //we are not processing at this time.
                    printf("inProcess: %d\n", procControl->inProcess);
                    //sending checking in message to master
                    printf("procControl->processBlock[ %d ].queueID: %d\n",
                           index, procControl->processBlock[index].queueID);
                    switch (priorityCases) {
                        printf("./oss: looking at priorities right now\n");
                        case 0:
                            if (procControl->processBlock[index].procCpuTime.tv_nsec >= (10 * MILLISECOND)) {
                                /*************************** Queueing section ****************************/
                                //in the case of RT processes we end up immediately pulling the next one.
                                //queueing RT process to queue 0;
                                printf("enqueuing: %d\n", currentProcPid);
                                procControl->inProcess = false;
                                procControl->processBlock[index].state = 0;
                                enqueue(active[0], currentProcPid);
                                printf("printing active queue0: ");
                                printQueue(active[0]);
                                printPCB();
                                printProcessState();
                                queueProcessing(&currentProcPid);
                                break;
                            } else {
                                queueProcessing(&currentProcPid);
                            }
                        case 1:
                            if (procControl->processBlock[index].procCpuTime.tv_nsec >= maxQueueLimit1) {
                                //penalizing the process for taking too long
                                //process will move to expired queue 2
                                printf("enqueuing: %d to expired queue2\n", currentProcPid);
                                procControl->processBlock[index].queueID = 2;
                                procControl->processBlock[index].state = 0;
                                enqueue(expired[2], currentProcPid);
                                procControl->inProcess = false;
                                printProcessState();
                                queueProcessing(&currentProcPid);
                                break;
                            } else if (procControl->processBlock[index].procCpuTime.tv_nsec >= (5 * MILLISECOND)) {
                                printf("enqueuing: %d to expired queue1\n", currentProcPid);
                                procControl->processBlock[index].state = 0;
                                enqueue(expired[1], currentProcPid);
                                procControl->inProcess = false;
                                printProcessState();
                                queueProcessing(&currentProcPid);
                                break;
                            }
                        case 2:
                            /*************************** Queueing section ****************************/
                            if (procControl->processBlock[index].procCpuTime.tv_nsec >= maxQueueLimit2) {
                                //penalizing the process for taking too long
                                //process will move to expired queue 3

                                printf("enqueuing: to expired queue3%d\n", currentProcPid);
                                procControl->processBlock[index].queueID = 3;
                                procControl->processBlock[index].state = 0;
                                enqueue(expired[3], currentProcPid);
                                procControl->inProcess = false;
                                printProcessState();
                                queueProcessing(&currentProcPid);
                                break;
                            } else if (procControl->processBlock[index].procCpuTime.tv_nsec >= (2.5 * MILLISECOND)) {
                                printf("enqueuing: to expired queue2%d\n", currentProcPid);
                                enqueue(expired[2], currentProcPid);
                                procControl->processBlock[index].state = 0;
                                procControl->inProcess = false;
                                printProcessState();
                                queueProcessing(&currentProcPid);
                                break;
                            }
                        case 3:
                            //we'll never put RT in expired or move it down
                            if (procControl->processBlock[index].procCpuTime.tv_nsec >= (1.25 * MILLISECOND)) {
                                //queueing priority 3 process to queue 3
                                printf("enqueuing: to expired queue3%d\n", currentProcPid);
                                enqueue(expired[3], procControl->processBlock[index].pid);
                                procControl->processBlock[index].state = 0;
                                procControl->inProcess = false;
                                printProcessState();
                                queueProcessing(&currentProcPid);
                                break;
                            }

                    }
                    printf("currentProcessPID: %d\n", currentProcPid);
                    printProcessState();
                    printf("active 0: \n");
                    printQueue(active[0]);
                    printf("active 1: \n");
                    printQueue(active[1]);
                    printf("active 2: \n");
                    printQueue(active[2]);
                    printf("active 3: \n");
                    printQueue(active[3]);
                    printf("expired 1: \n");
                    printQueue(expired[1]);
                    printf("expired 2: \n");
                    printQueue(expired[2]);
                    printf("expired 3: \n");
                    printQueue(expired[3]);
                    printPCB();
                    printf("inProcess: %d\n", procControl->inProcess);
                }
            }

//        if ((messageStatus = msgrcv(toMasterID, &message, sizeof(message),
//                                        message.mesg_type, MSG_NOERROR | IPC_NOWAIT) > -1)) {
//
        int status;
        pid_t pid;

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

                    //we'll need to store long numbers
                    long time1;
                    long time2;
                    long time3;
                    long totalTime1;
                    long totalTime2;
                    struct timespec waitReportTime;

                    //taking the localPid tag off of the processBlock
                    if (currentProcPid > -1) {
                        --concurrentProc;
                        procControl->inProcess = false;
                        procControl->processBlock[index].state = 0;
                        procControl->processBlock[index].localPid = -1;
                        procControl->processBlock[index].pid = -1;
                        printf("PID returned. Post terminate status report: \n");
                        printPCB();
                        printStatus(currentProcPid);
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
        if(processCount == 100){
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

int queueProcessing( pid_t *currentProcPid ) {
    int count = 0;
    int dequeueResult;
    printf("inside queueProcessing");
    printStatus(*currentProcPid);

    //this will handle the case of terminated process
    //as well as cases of empty queues.
    while( procControl->inProcess == false ) {
        if (procControl->inProcess == false) {
            printf("./oss: checking queues now\n");
            //nothing is processing. Time to look for things in queue and set them to process.
            //we won't check for queue 0 since the queueing process would empty RT processes
            if (((dequeueResult = dequeue(active[0])) == -1) && procControl->inProcess == false) {
                printf("there's nothing in queue 0\n");
            } else {
                //assigning results to currentProcPid
                *currentProcPid = dequeueResult;
                //getting the current PCB index of the process
                //telling the process to continue processing
                tellProcessResume(*currentProcPid);
                //we are now processing
                procControl->inProcess = true;
                printQueue(active[0]);
                printProcessState();
                return 0;
            }
            if (((dequeueResult = dequeue(active[1])) == -1) && procControl->inProcess == false) {
                printf("There is no processes in queue 1\n");
            } else {
                //we'll have the first number in queue 1 assigned to currentProcPid
                *currentProcPid = dequeueResult;
                tellProcessResume(*currentProcPid);
                procControl->inProcess = true;
                printQueue(active[1]);
                printProcessState();
                return 0;
            }
            if (((dequeueResult = dequeue(active[2])) == -1) && procControl->inProcess == false) {
                printf("There is no processes in queue 2\n");
            } else {
                //we'll have the first number in queue 2 assigned to currentProcPid
                *currentProcPid = dequeueResult;
                tellProcessResume(*currentProcPid);
                procControl->inProcess = true;
                printQueue(active[2]);
                printProcessState();
                return 0;
            }
            if (((dequeueResult = dequeue(active[3])) == -1) && procControl->inProcess == false) {
                printf("There is no processes in queue 3\n");
            } else {
                //we'll have the first number in queue 3 assigned to currentProcPid
                *currentProcPid = dequeueResult;
                tellProcessResume(*currentProcPid);
                procControl->inProcess = true;
                printQueue(active[3]);
                printProcessState();
                return 0;
            }
            //swapping queue before starting the queue search again
            if (procControl->inProcess == false) {
                if( count >=2 ){
                    return -1;
                }
                printf("swapping queues now\n");
                swapQueues();
                count++;
            }
        }
    }
}

void tellProcessResume( pid_t pid ){
    printf("./oss: process currently waiting... sending RESUME message to PID: %d\n",
           pid);
    strcpy(message.mesg_text, "PROC_RESUME");
    message.mesg_type = pid;
    printf("./oss: sending pid:%d tellProcessResume message: %s\n", pid, message.mesg_text);
    //sending terminate message to master
    if( msgsnd(toUserID, &message, sizeof(message), 0) == -1){
        perror("send_message");
        exit(1);
    }
    return;
}

void printStatus( pid_t currentProcPid) {
    printf("currentProcessPID: %d\n", currentProcPid);
    printProcessState();
    printf( "active 0: \n");
    printQueue( active[0] );
    printf( "active 1: \n");
    printQueue( active[1] );
    printf( "active 2: \n");
    printQueue( active[2] );
    printf( "active 3: \n");
    printQueue( active[3] );
    printf( "expired 1: \n");
    printQueue( expired[1] );
    printf( "expired 2: \n");
    printQueue( expired[2] );
    printf( "expired 3: \n");
    printQueue( expired[3] );
    printPCB();
    printf("inProcess: %d\n", procControl->inProcess);
}
