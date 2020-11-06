//user.c

#include "sharedMemory.h"
#include "detachandremove.h"
#include "queue.h"
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

//macros
#define PERM (S_IRUSR)
#define SECOND 1000000000
#define MILLISECOND 1000000
#define MICROSECOND 1000

//function prototypes
int findPCBIndex( int myPid );
void initSharedMem();
void initMsgQueue();
void processWait( int pid );
int detachShared();

// structure for message queue
struct mesg_buffer {
    long mesg_type;
    char mesg_text[100];
} message;

//runtime global
struct timeval runtimeStart;
struct timeval runtimeEnd;
struct timespec runtime;
long runtimeDiff;
struct timeval waitStart;
struct timeval waitEnd;
long waitDiff;
int tValue = 3;

//message queue globals
key_t userKey;
key_t masterKey;
int toUserID;
int toMasterID;
int messageStatus;

//shared memory section
key_t myKey;
key_t key;
static int shm_id;
sharedMem *procControl = NULL;

//kill signal handler
static void myKillSignalHandler( int s ){
    char timeout[] = "./user: caught ctrl+c, ending processes.\n";
    int timeoutSize = sizeof( timeout );
    int errsave;

    errsave = errno;
    write(STDERR_FILENO, timeout, timeoutSize );
    errno = errsave;

    //destroying message queues
    msgctl(toUserID, IPC_RMID, NULL);
    msgctl(toMasterID, IPC_RMID, NULL);

    printf( "./user: destroyed message queues\n");
    exit(0);
}

//signal interrupt
static int setupUserInterrupt( void ){
    struct sigaction act;
    act.sa_handler = myKillSignalHandler;
    act.sa_flags = 0;
    return (sigemptyset(&act.sa_mask) || sigaction(SIGINT, &act, NULL));
}

int main( int argc, char** argv ) {

    if( setupUserInterrupt() == -1 ){
        perror( "./user: failed to set up a user kill signal.\n");
        return 1;
    }

    procControl = (sharedMem *)malloc(sizeof(sharedMem));

    int userPid = getpid();
    int index;
    if( ( index = findPCBIndex( userPid ) ) == -1);
    int diceRoll;
    srand(time(0));
    printf( "inside process: %d\n",(int)getpid() );
    //initializing message queue
    initMsgQueue();
    initSharedMem();
    struct timeval tStart;
    struct timeval tEnd;
    struct timeval tCurrentStart;
    struct timeval tCurrentEnd;
    long tDifference;
    int queueProcessResult;


    //getting time for start.
    gettimeofday( &tStart, NULL );
    printf("myLocalIndex: %d\n", index);
    message.mesg_type = userPid;
    runtime.tv_nsec = 0;
    gettimeofday(&runtimeStart, NULL);

    while(1) {
        processWait(userPid);
        //we'll just keep rolling the dice till we get terminate
        diceRoll = (rand() % 100);
        printf("rolled number: %d\n", diceRoll);
        if (diceRoll >= 99) {
            //termination works, so now we'll disable this to get scheduling with the queue working.
            //the case of termination
            printf("number greater than 79. Terminating userPid: %d\n", userPid);
            strcpy(message.mesg_text, "TERMINATE");
            message.mesg_type = userPid;
            //sending terminate message to master
            if( msgsnd(toMasterID, &message, sizeof(message), 0) == -1){
                perror("send_message");
                exit(1);
            }
            printf("./user: TERMINATE message sent...\n");

            gettimeofday(&tEnd, NULL);

            //calculating system time
            tDifference = (tEnd.tv_usec - tStart.tv_usec) * MICROSECOND;
            procControl->processBlock[index].procSysTime.tv_nsec = tDifference;

            //calculating runtime for the last time
            gettimeofday( &runtimeEnd, NULL );
            runtimeDiff = (runtimeEnd.tv_usec - runtimeStart.tv_usec) * MICROSECOND;
            runtime.tv_nsec += runtimeDiff;
            procControl->processBlock[index].procCpuTime.tv_nsec = runtime.tv_nsec;

            //adding the time difference to CPU
            printf("./user PID: %d, cpuTime: %ld ns\n", userPid,
                   procControl->processBlock[index].procCpuTime.tv_nsec);
            printf("./user PID: %d, sysTime: %ld ns\n", userPid,
                   procControl->processBlock[index].procSysTime.tv_nsec);
            break;
        } else if (diceRoll >= 0 && diceRoll < 99) {
            //the case of just processing
            printf("number is: %d. Processing...\n", diceRoll);
            processWait( userPid );
        }
    }

    exit(0);
}

//initializes shared memory
void initSharedMem(){
    //grabbing key for shared memory
    if( ( myKey = ftok( "sharingIsCaring", 1 ) ) == (key_t)-1 ) {
        fprintf(stderr, "Failed to derive key from filename %s:%s\n", "sharingIsCaring", strerror( errno ) );
        return;
    }

    //shared memory creation
    shm_id = shmget( myKey, 200, PERM );

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

//function for finding pid from the PCB in the PCT
int findPCBIndex( int myPid ){
    int i;

    //debugging output
//    for( i = 0; i < 18; i++) {
//        printf( "pct->processBlock[ %d ].pid: %d\n",
//                i, pct->processBlock[ i ].pid);
//    }

    printf( "myPid: %d\n", myPid );
    for( i = 0; i < 18; i++ ){
        if( procControl->processBlock[ i ].pid == myPid ){
            return i;
        }
    }
    //return -1 or error if we cannot find anything
    return -1;
}

void processWait( int pid ) {
    int index = findPCBIndex(pid);
    int priorityID = procControl->processBlock[index].queueID;
    printf("./user priorityID: %d\n", priorityID);
    long allowedQuantum0 = 10 * MILLISECOND;
    long allowedQuantum1 = allowedQuantum0 / 2;
    long allowedQuantum2 = allowedQuantum1 / 2;
    long allowedQuantum3 = allowedQuantum2 / 2;

    int processQuantum;

    bool wait = false;
    //we need to set up a quantum count for the process itself
    if (priorityID == 0) {
        processQuantum = allowedQuantum0;
    } else if (priorityID == 1) {
        processQuantum = allowedQuantum1;
    } else if (priorityID == 2) {
        processQuantum = allowedQuantum2;
    } else if (priorityID == 3) {
        processQuantum = allowedQuantum3;
    }

    gettimeofday( &runtimeEnd, NULL );
    runtimeDiff = (runtimeEnd.tv_usec - runtimeStart.tv_usec) * MICROSECOND;
    runtime.tv_nsec += runtimeDiff;
    procControl->processBlock[index].procSysTime.tv_nsec += runtimeDiff;
    procControl->processBlock[index].procCpuTime.tv_nsec += runtimeDiff;
    printf("./user PID: %d, cpuTime: %ld ns\n", pid,
           procControl->processBlock[index].procCpuTime.tv_nsec);
    printf("./user PID: %d, runtime: %ld ns\n", pid,
           runtime.tv_nsec);
    if (runtime.tv_nsec >= processQuantum) {
        //we are now going to wait because our quantum expired
        message.mesg_type = pid;
        gettimeofday(&waitStart, NULL);
        strcpy(message.mesg_text, "PROC_WAIT");
        wait = true;
        printf("./user %d: quantum expired sending WAIT message to ./oss: %s\n", pid, message.mesg_text);
        if( msgsnd(toMasterID, &message, sizeof(message), 0) == -1){
            perror("send_message");
            exit(1);
        }

        //sending WAIT message from a process
    } else {
        return;
    }
    while (wait) {
        if ((messageStatus = msgrcv(toUserID, &message, sizeof(message),
                                    pid, 0) >= 0)) {
            //receiving PROC_RESUME message from a process
            printf("./user %d: PROC_RESUME message received: %s\n", pid, message.mesg_text);
            if (strcmp(message.mesg_text, "PROC_RESUME") == 0) {
                gettimeofday(&waitEnd, NULL);
                waitDiff = ( waitEnd.tv_usec - waitStart.tv_usec ) * MICROSECOND;
                procControl->processBlock[index].procWaitTime.tv_nsec += waitDiff;
                procControl->processBlock[index].procSysTime.tv_nsec += waitDiff;
                printf("PID %d:process is resuming...\n", pid);
                wait = false;
                //resetting runtime
                runtime.tv_nsec = 0;
                procControl->processBlock[index].state = 1;
                gettimeofday( &runtimeStart, NULL );
            }
            //setting processing flag to false
        } else {
            perror("receive_message");
            exit(1);
        }
    }
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