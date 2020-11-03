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

// structure for message queue
struct mesg_buffer {
    long mesg_type;
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

//output file global
FILE *outFilePtr1;

//constants global
const int QUEUE_QUANTUM = 10; //10 ms for the base
const int NANOS_PER_SEC = 1000000000;
const int NANOS_PER_MILLISEC = 1000000;
const int TEN_MICRO_SECS = 10000;
const int SEC_PER_MILLISEC = 1000;
const int MAX_TIME_BETWEEN_NEW_PROCS_NS = 150000;
const int MAX_TIME_BETWEEN_NEW_PROCS_SEC = 1;
const int RT_PERCENT = 80;

//this value will take in seconds after which the process terminate, even if it has not finished.
static int tValue = 3;
static int processCount = 0;
static int maxChildren = 18;
static int localPid = 0;

//Time until next execution
Time nextExec = {0, 0};

//timing statistics
Time totalCpuTime = {0, 0};
Time totalWaitTime = {0, 0};
Time totalBlockedTime = {0, 0};
Time totalTime = {0, 0};
Time idleTime = {0, 0};

//shared memory section
key_t myKey;
key_t key;
int msgid;
static int shm_id;
static sharedMem *procControl;

//message queue globals
key_t userKey;
key_t masterKey;
int userId;
int masterId;

//queue global
Queue *qset1[4];
Queue *qset2[4];
Queue blocked;
Queue **active;
Queue **expired;

//misc globals
int currentProcIndex = 0;

/* ARGUSED */
static void myhandler( int s ) {
    char timeout[] = "timing out processes.\n";
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
        printf( "everything is finished... exiting program\n");
    }
    else {
        printf( "unable to detach shared memory... please check outside of the program\n" );
    }

    //destroying message queues
    msgctl(userId, IPC_RMID, NULL);
    msgctl(masterId, IPC_RMID, NULL);

    //closing file
    fclose(outFilePtr1);

    printf( "./oss: destroyed message queues\n");
    exit(0);
}

static void myKillSignalHandler( int s ){
    char timeout[] = "caught ctrl+c, ending processes.\n";
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
        printf( "everything is finished... exiting program\n");
    }
    else {
        printf( "unable to detach shared memory... please check outside of the program\n" );
    }

    //destroying message queues
    msgctl(userId, IPC_RMID, NULL);
    msgctl(masterId, IPC_RMID, NULL);

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
        perror( "failed to set up a user kill signal.\n");
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
        perror("Failed to set up handler for SIGALRM");
        return 1;
    }

    if (setupitimer(tValue) == -1) {
        perror("Failed to set up the ITIMER_PROF interval timer");
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

    //Queue max time limit. 3rd queue is infinity with 1.25 ms
    int queueTimeLimit0 = ( QUEUE_QUANTUM ) * NANOS_PER_MILLISEC;
    int queueTimeLimit1 = ( QUEUE_QUANTUM / 2) * 2 * NANOS_PER_MILLISEC;
    int queueTimeLimit2 = ( QUEUE_QUANTUM / 4) * 3 * NANOS_PER_MILLISEC;
    int queueTimeLimit3 = ( QUEUE_QUANTUM / 8) * NANOS_PER_MILLISEC;

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
    shm_id = shmget( myKey, 200, PERM | IPC_CREAT | IPC_EXCL);

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
    int concurrentProc = 0;
    int messageStatus;
    int currentPBIndex;
    int diceRoll;
    int status;

    //seeding random number generator
    srand(time(0));

    while(1) {
        //normally we only allow 18 processes (maxChildren) but for now we'll leave it at 1
        //processCount should be limited to 100 but for now we'll have it be 1 in this case

        //I think the system time should be kept in nanoseconds and converted accordingly
        //incrementing the system time
        procControl->sysTime.ms += TEN_MICRO_SECS / 1000;
        procControl->sysTime.ns += TEN_MICRO_SECS;
        idleTime.ms += TEN_MICRO_SECS / 1000;
        idleTime.ns += TEN_MICRO_SECS;

        if( processCount <= 100 && concurrentProc < maxChildren && ( procControl->sysTime.ns >= nextExec.ns ) ) {

            childPid = fork();
            printf( "childPid: %d\n", childPid);
            if (childPid < 0) {
                perror("failed to fork");
                processCount--;
                exit(1);
            }

            currentPBIndex = findEmptyPB();

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
                exit(0);
            }
            //*************************** process handling section ***************************
            //incrementing nextExec
            nextExec.ms = MAX_TIME_BETWEEN_NEW_PROCS_NS / NANOS_PER_MILLISEC;
            nextExec.ns = MAX_TIME_BETWEEN_NEW_PROCS_NS;

            int index = findEmptyPB();
            if( index >= 0 ) {
                procControl->processBlock[index].pid = childPid;

                //determining if process is a realtime
                diceRoll = ( rand() % 100 );

                //assigning RT to process if it rolls the right numbers.
                if(diceRoll >= RT_PERCENT ) {
                    procControl->processBlock[index].realTime = 1;
                    procControl->processBlock[index].queueID = 0;
                } else {
                    procControl->processBlock[index].realTime = 0;
                    procControl->processBlock[index].queueID = 1;
                }

                //initializing time
                procControl->processBlock[index].procCpuTime.ms = 0;
                procControl->processBlock[index].procSysTime.ms = 0;
                procControl->processBlock[index].procBlockedTime.ms = 0;
                procControl->processBlock[index].localPid = localPid++;

                if( procControl->processBlock[index].queueID == 0) {
                    enqueue(active[0], procControl->processBlock[index].localPid);
                } else {
                    enqueue(active[1], procControl->processBlock[index].localPid);
                }

                //outputting to file
                fprintf( outFilePtr1, "./oss: Process created at: %d, PID:%d, localPid: %d, Queue:%d \n",
                         procControl->processBlock[index].procSysTime.ms,
                         procControl->processBlock[index].pid,
                         procControl->processBlock[index].localPid,
                         procControl->processBlock[index].queueID );

                //incrementing concurrent processes
                concurrentProc++;
            } else {
                //killing process when there's no room in PCB
                printf("no room in the PCB, killing process...\n");
                kill( childPid, SIGTERM );
            }



            /* Setup the next exec for process*/


            //capture current time
            nextExec.ms = procControl->sysTime.ms;

            //add additional time to current time to setup the next attempted exec
            int secstoadd = abs(rand() % (MAX_TIME_BETWEEN_NEW_PROCS_SEC + 1));
            int nstoadd = abs((rand() * rand()) % (MAX_TIME_BETWEEN_NEW_PROCS_NS + 1));


            //receiving message that a process wants to terminate itself
            if( ( messageStatus = msgrcv(masterId, &message, sizeof(message),
                                         procControl->processBlock[currentProcIndex].pid, 0) >= 0) ) {
                //receiving termination message from a process
                printf("message received: %s\n", message.mesg_text);
                if (strcmp(message.mesg_text, "TERMINATE") == 0) {
                    printf("process is terminating...\n");
                }
            }

            if ( (childPid = waitpid( (pid_t) -1, &status, WNOHANG ) ) > 0){
                //if a PID is returned meaning the child died
                if (WEXITSTATUS(status) == 0) {
                    processCount++;
                    concurrentProc--;

                    int localPid = findLocalPID(childPid);
                    //we'll need to store long numbers
                    long time1;
                    long time2;
                    long time3;
                    long totalTime1;
                    long totalTime2;
                    Time waitReportTime = {0,0};

                    //capture current time as wait time
                    procControl->processBlock[localPid].procWaitTime.ms = procControl->processBlock[localPid].procSysTime.ms;
                    procControl->processBlock[localPid].procWaitTime.ns = procControl->processBlock[localPid].procSysTime.ns;

                    //calculating wait time. cpuTime - blockedTime;
                    time1 = procControl->processBlock[localPid].procWaitTime.ns +
                            (procControl->processBlock[localPid].procWaitTime.ms * NANOS_PER_SEC);
                    time2 = procControl->processBlock[localPid].procCpuTime.ns +
                            (procControl->processBlock[localPid].procCpuTime.ms * NANOS_PER_SEC);
                    totalTime1 = abs(time1 - time2);


                    time3 = procControl->processBlock[localPid].procBlockedTime.ns +
                    (procControl->processBlock[localPid].procBlockedTime).ms * NANOS_PER_SEC;

                    totalTime2 = abs( totalTime1 - time3);
                    //obtain wait time by subtracting blocked and cpuTime from blocked time in the system
                    waitReportTime.ns = totalTime2;
                    waitReportTime.ms = totalTime2 / NANOS_PER_MILLISEC;

                    printf("Local PID: %d statistics: %d\n", procControl->processBlock[localPid].localPid);

                    printf("CPU Time: secs: %d, nanos:%d\n",
                           ( procControl->processBlock[localPid].procCpuTime.ms * SEC_PER_MILLISEC ),
                           procControl->processBlock[localPid].procCpuTime.ns);
                    printf("Wait Time: secs: %d, nanos:%d\n",
                           ( waitReportTime.ms * SEC_PER_MILLISEC ),
                           waitReportTime.ns);
                    printf("Blocked Time: secs: %d, nanos:%d\n",
                           ( procControl->processBlock[localPid].procBlockedTime.ms * SEC_PER_MILLISEC ),
                           procControl->processBlock[localPid].procBlockedTime.ns);

                    if (localPid > -1)
                        procControl->processBlock[localPid].pid = -1;
                }
            }
        }
    }

    //we need to make a user process.

    //we want to make a message queue for the processes running.

    //destroying message queues
    msgctl(userId, IPC_RMID, NULL);
    msgctl(masterId, IPC_RMID, NULL);

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
    userId = msgget(userKey, 0666 | IPC_CREAT);

    if (userId == -1) { //checking see if msgget worked
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
    masterId = msgget(masterKey, 0666 | IPC_CREAT);

    if (masterId == -1) { //checking see if msgget worked
        printf("failed to attach masterInKey\n");
        return;
    }
}

/* Find the next empty proccess block. Returns proccess block position if one is available or -1 if one is not */
int findEmptyPB(){
    int i;
    for (i = 0; i < maxChildren; i++)
    {
        if (procControl->processBlock[i].pid == -1)
            //return an empty index proccess table
            return i;
    }
    //we found nothing in this case.
    return -1;
}

/* Initializing Process Blocks*/
void initProcBlocks(){
    int i;
    for (i = 0; i < maxChildren; i++)
        procControl->processBlock[i].pid = -1;
}

int findLocalPID(int pid){
    int i;
    for( i = 0; i < maxChildren; i++ ){
        if(procControl->processBlock[i].pid == pid)
            return i;
    }
    //if we find nothing
    return -1;
}
