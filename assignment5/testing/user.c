//user.c

#include "sharedMemory.h"
#include "detachandremove.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
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
#define MAXCPUTIME 150000
#define SECOND 1000000000
#define MILLISECOND 1000000
#define MICROSECOND 1000

//function prototypes
int findPCBIndex( int myPid );
void initSharedMem();
void initMsgQueue();
void initMaxClaim( int index );
void printMaxClaim( int index );
void resourceRequest( pid_t pid );
void resourceRelease( pid_t pid );
int resourceCheck(int index);
int detachShared();

// structure for message queue
static struct mesg_buffer {
    pid_t mesg_pid;
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
bool wait;

//message queue globals
key_t userKey;
key_t masterKey;
int toUserID;
int toMasterID;

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
//
//    if( setupUserInterrupt() == -1 ){
//        perror( "./user: failed to set up a user kill signal.\n");
//        return 1;
//    }

    procControl = (sharedMem *)malloc(sizeof(sharedMem));

    pid_t userPid = getpid();
    printf( "userPid: %d\n", userPid);
    int index;                       //This will hold the index of the PCB.
    int diceRoll;
    srand(time(0));
    printf( "inside process: %d\n",(int)getpid() );
    //initializing message queue
    initMsgQueue();
    initSharedMem();
    if( ( index = findPCBIndex( userPid ) ) == -1){
        printf("./user: ERROR did not find the correct index.\n");
        exit( 1 );
    }
    initMaxClaim( index );
    printMaxClaim( index );
    int priorityID = procControl->processBlock[index].queueID;
    struct timeval tStart, tEnd, tCurrentStart, tCurrentEnd;
    long tDifference;
    int queueProcessResult;
    long maxTime = MAXCPUTIME;
    int termPercent = 90;
    int requestBound;
    int releasePercent = 45;

    //getting time for start.
    gettimeofday(&tStart, NULL);
    printf("myLocalIndex: %d\n", index);
    message.mesg_type = userPid;
    runtime.tv_nsec = 0;


    while(1) {
        gettimeofday(&runtimeStart, NULL);
        //we'll just keep rolling the dice till we get terminate
        diceRoll = (rand() % 100);
        printf("rolled number: %d\n", diceRoll);



        if ((procControl->processBlock[index].procCpuTime.tv_nsec >= maxTime) || diceRoll >= termPercent ) {
            //termination works, so now we'll disable this to get scheduling with the queue working.
            //the case of termination
            printf("Terminating userPid: %d\n", userPid);
            strcpy(message.mesg_text, "PROC_TERM");
            message.mesg_type = 1;
            message.mesg_pid = userPid;
            //sending terminate message to master
            if( msgsnd(toMasterID, &message, sizeof(message), 0) == -1){
                perror("send_message");
                exit(1);
            }
            printf("./user: TERMINATE message sent...\n");

            gettimeofday(&tEnd, NULL);

            //calculating system time
            tDifference = (((long)(tEnd.tv_usec - tStart.tv_usec))* MICROSECOND);
            printf("147 tDifference: %ld\n", tDifference);
            procControl->processBlock[index].procSysTime.tv_nsec = tDifference;

            procControl->sysTime.tv_nsec = tDifference;
            //calculating runtime for the last time
            gettimeofday(&runtimeEnd, NULL);
            runtimeDiff = (runtimeEnd.tv_usec - runtimeStart.tv_usec) * MICROSECOND;
            printf("154 runtimeDiff: %ld\n", runtimeDiff);
            runtime.tv_nsec += runtimeDiff;
            procControl->processBlock[index].procCpuTime.tv_nsec += runtimeDiff;
            procControl->sysTime.tv_nsec += runtimeDiff;

            //adding the time difference to CPU
            printf("./user PID: %d, cpuTime: %ld ns\n", userPid,
                   procControl->processBlock[index].procCpuTime.tv_nsec);
            printf("./user PID: %d, sysTime: %ld ns\n", userPid,
                   procControl->processBlock[index].procSysTime.tv_nsec);
            exit(14);
        } else if (diceRoll >= releasePercent && diceRoll < termPercent) {
            //the case of requesting resources
            resourceRequest( userPid );
        } else if (diceRoll >= 0 && diceRoll < releasePercent) {
            //the case of releasing resources
            if( resourceCheck(index) == 1 ) {
                resourceRelease( userPid );
            } else {
                printf("no resources allocated. Making resource request Instead\n");
                resourceRequest( userPid );
            }
        }

        gettimeofday( &runtimeEnd, NULL );
        runtimeDiff = (runtimeEnd.tv_usec - runtimeStart.tv_usec) * MICROSECOND;
        runtime.tv_nsec += runtimeDiff;
        procControl->processBlock[index].procSysTime.tv_nsec += runtimeDiff;
        procControl->processBlock[index].procCpuTime.tv_nsec += runtimeDiff;
    }
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

void initMaxClaim( int index ){
    //seeding RNG
    int dice;
    int r;
    for(r = 0; r < 10 ; r++){
        dice = ( rand() % 20 );
        procControl->procResource[ index ].maxClaim[r] = dice;
    }
}

void printMaxClaim( int index ){
    int i;
    printf("maxClaim: \n");
    for( i = 0; i < 10; i++){
        printf("%d ", procControl->procResource[ index ].maxClaim[i]);
    }
    printf("\n");
}

void printAllocate( int index ){
    int i;
    printf("allocate resourceType: %d;");
    for(i = 0; i < 10; i++){
        printf("%d ", procControl->procResource[ index ].allocated[i][0]);
    }
    printf("\n");
    for(i = 0; i < 10; i++){
        printf("%d ", procControl->procResource[ index ].allocated[i][1]);
    }
}

//function for finding pid from the PCB in the PCT
int findPCBIndex( pid_t myPid ){
    int i;

    //debugging output
//    for( i = 0; i < 18; i++) {
//        printf( "pct->processBlock[ %d ].pid: %d\n",
//                i, pct->processBlock[ i ].pid);
//    }

    printf( "myPid: %d\n", myPid );
    for( i = 0; i <= 18; i++ ){
        if( procControl->processBlock[ i ].pid == (int)myPid ){
            return i;
        }
    }
    //return -1 or error if we cannot find anything
    return -1;
}

void resourceRequest( pid_t pid ){
    int index = findPCBIndex(pid);
    int resourceType;
    int resourceBound;
    int totalAllocated;
    int amountRequested;
    int messageStatus;
    bool waiting;

    //rolling to see which resource type to request
    resourceType = (rand() % 10);

    //getting the total allocated amount from both resource sets
    totalAllocated = procControl->procResource[index].allocated[resourceType][0] +
                     procControl->procResource[index].allocated[resourceType][1];
    //calculating the resource bound now
    resourceBound = procControl->procResource[index].maxClaim[resourceType] -
                    totalAllocated;

    //randomly generating resource request amount
    amountRequested = ( (rand() % resourceBound) + 1 );
    //assigning amountRequested to shared memory
    procControl->procResource[index].request[resourceType] = amountRequested;

    //debugging output
    printf("./user resourceType: %d\n", resourceType);
    printf("./user maxClaim: %d\n", procControl->procResource[index].maxClaim[resourceType]);
    printf("./user allocated set1: %d\n",procControl->procResource[index].allocated[resourceType][0]);
    printf("./user allocated set2: %d\n",procControl->procResource[index].allocated[resourceType][1]);
    printf("./user amountRequested: %d\n", amountRequested);
    printf("./user resourceBound: %d\n", resourceBound);

    message.mesg_pid = pid;
    message.mesg_type = 1;
    strcpy(message.mesg_text, "REQ_RESOURCE");
    procControl->processBlock[index].state = 0;
    waiting = true;
    printf("./user %d: sending REQ_RESOURCE message to ./oss: %s\n", pid, message.mesg_text);
    if (msgsnd(toMasterID, &message, sizeof(message), 0) == -1) {
        perror("send_message");
    }

    //if we don't get APPROVE, we'll be sitting in wait till we get what we need.
    msgrcv(toUserID, &message, sizeof(message), pid, 0);
    //receiving PROC_RESUME message from a process
    if (strcmp(message.mesg_text, "APPROVE") == 0) {
        printf("./user %d: APPROVE message received: %s\n", message.mesg_pid, message.mesg_text);
        printf("PID %d: process ...\n", message.mesg_type);
        //resetting runtime
        runtime.tv_nsec = 0;
        procControl->processBlock[index].state = 1;
    }
}

void resourceRelease( pid_t pid ) {
    int index = findPCBIndex(pid);
    //this function will release resources atleast one or more.
    printf("releasing resources...\n");
    int diceRoll;
    int amountRelease;
    bool finished = false;      //this will be true when one resource has been released
    int r;
    int s;
    while( !finished ) {
        //this is in the case that we really haven't released any resources (highly unlikely)
        for (r = 0; r < 10; r++) {
            if (procControl->procResource[index].allocated[r][0] > 0 ||
                procControl->procResource[index].allocated[r][1] > 0) {
                //this means there's resources to release else we move on.

                //we'll be randomly rolling to see if the resources can be released
                diceRoll = (rand() % 2);
                if (diceRoll == 1) {
                    if (procControl->procResource[index].allocated[r][0] > 0) {
                        amountRelease = (rand() % procControl->procResource[index].allocated[r][0]) + 1;
                        procControl->procResource[index].allocated[r][0] -= amountRelease;
                        printf("%d is released from released resource: %d from system resource set: %d\n",
                               amountRelease, r, 1);

                        //now resources is released, let's feed it back into SysResources
                        procControl->rSet1.totalResources[r] += amountRelease;

                        //now we can mark the flag for true
                        finished = true;
                    }
                    if (procControl->procResource[index].allocated[r][1] > 0) {
                        amountRelease = (rand() % procControl->procResource[index].allocated[r][1]) + 1;
                        procControl->procResource[index].allocated[r][1] -= amountRelease;
                        printf("%d is released from resourceType: %d from system resource set: %d\n",
                               amountRelease, r, 2);

                        //now resources is released, let's feed it back into SysResources
                        procControl->rSet2.totalResources[r] += amountRelease;

                        //now we can mark the flag for true
                        finished = true;
                    }
                }
            }
        }

    }

    //since we just released some resources let's send a message back to the ./oss
    message.mesg_pid = pid;
    message.mesg_type = 1;
    strcpy(message.mesg_text, "REL_RESOURCE");
    procControl->processBlock[index].state = 0;
    printf("./user %d: sending REL_RESOURCE message to ./oss: %s\n", pid, message.mesg_text);
    if (msgsnd(toMasterID, &message, sizeof(message), 0) == -1) {
        perror("send_message");
        exit(1);
    }
}

int resourceCheck(int index){
    int i;
    for( i = 0; i < 10; i++){
        if( procControl->procResource[index].allocated[i][0] > 0){
            return 1;
        }
        if( procControl->procResource[index].allocated[i][2] > 0){
            return 1;
        }
    }
    //if we don't find any resources allocated
    return -1;
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