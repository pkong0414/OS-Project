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
#define MAXPROCESSLIMIT 30000000
#define SECOND 1000000000
#define MILLISECOND 1000000
#define MICROSECOND 1000

//function prototypes
int findPCBIndex( int myPid );
void initSharedMem();
void initMsgQueue();
void printPCBState();
void processWait( int pid );
int detachShared();
void messageReceive(int pid);

// structure for message queue
struct mesg_buffer {
    long mesg_type;
    pid_t mesg_pid;
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
sig_atomic_t volatile wait;
int tValue = 3;
struct sigaction sigact;

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
    char timeout[] = "./user: SIGINT caught\n";
    int timeoutSize = sizeof( timeout );
    int errsave;

    errsave = errno;
    write(STDERR_FILENO, timeout, timeoutSize );
    errno = errsave;
    wait = false;

//    //destroying message queues
//    msgctl(toUserID, IPC_RMID, NULL);
//    msgctl(toMasterID, IPC_RMID, NULL);

    //printf( "./user: destroyed message queues\n");
    //exit(0);
}

static void catcher( int sig ){
    //this will handle resuming messages from the user./
    switch(sig) {
        case SIGTSTP:
            printf("Pausing %d\n", getpid());
            wait = true;
            break;
        case SIGCONT:
            printf("Resuming %d\n", getpid());
            wait = false;
            break;

    }
}

//signal interrupt
static int setupUserInterrupt( void ){
    struct sigaction act;
    act.sa_handler = myKillSignalHandler;
    act.sa_flags = 0;
    return (sigemptyset(&act.sa_mask) || sigaction(SIGINT, &act, NULL)
            || sigaction(SIGCONT,&act,NULL));
}

int main( int argc, char** argv ) {

    if( setupUserInterrupt() == -1 ){
        perror( "./user: failed to set up a user kill signal.\n");
        return 1;
    }
    procControl = (sharedMem *)malloc(sizeof(sharedMem));

    //initializing message queue
    initMsgQueue();
    initSharedMem();

    pid_t userPid = getpid();
    int index;
    if( ( index = findPCBIndex( userPid ) ) == -1){
        printf("./user: error: index: %d\n", index);
        return 1;
    }
    int priorityID = procControl->processBlock[index].queueID;
    int diceRoll;

    srand(time(0));
    printf( "inside process: %d\n", userPid );
    printf("./user priorityID: %d\n", priorityID);

    struct timeval tStart;
    struct timeval tEnd;
    struct timeval tCurrentStart;
    struct timeval tCurrentEnd;
    long tDifference;
    int queueProcessResult;
    int termPercent = 9995;
    long allowedQuantum0 = 1 * MILLISECOND;
    long allowedQuantum1 = allowedQuantum0 / 2;
    long allowedQuantum2 = allowedQuantum1 / 2;
    long allowedQuantum3 = allowedQuantum2 / 2;
    wait = true;
    int processQuantum;
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

    processWait(userPid);
    printPCBState();
    messageReceive(userPid);

    while(1) {
        //processWait(userPid);
        //we'll just keep rolling the dice till we get terminate
        //for now we'll experiment with this
        if ((messageStatus = msgrcv(toUserID, &message, sizeof(message.mesg_text),
                                    message.mesg_type, 0) >= 0)) {
            //receiving PROC_RESUME message from a process
            printf("./user %d: PROC_RESUME message received: %s\n", userPid, message.mesg_text);
            if (strcmp(message.mesg_text, "PROC_RESUME") == 0) {
                //we are handling the case of resume
                gettimeofday(&waitEnd, NULL);
                waitDiff = (waitEnd.tv_usec - waitStart.tv_usec) * MICROSECOND;
                procControl->processBlock[index].procWaitTime.tv_nsec += waitDiff;
                procControl->processBlock[index].procSysTime.tv_nsec += waitDiff;
                procControl->processBlock[index].state = 1;
                printf("PID %d:process is resuming...\n", userPid);
                //resetting runtime
                runtime.tv_nsec = 0;
            }
        }

        //getting time for start.
        gettimeofday( &tStart, NULL );
        printf("./user line 168: index: %d\n", index);
        gettimeofday(&runtimeStart, NULL);

        diceRoll = (rand() % 10000);
        //printf("rolled number: %d\n", diceRoll);
        if (diceRoll >= termPercent || (procControl->processBlock[index].procCpuTime.tv_nsec >= MAXPROCESSLIMIT )) {
            //termination works, so now we'll disable this to get scheduling with the queue working.
            //the case of termination
            printf("number greater than %d. Terminating userPid: %d\n", termPercent, userPid);
//            strcpy(message.mesg_text, "TERMINATE");
//            message.mesg_type = 1;
//            message.mesg_pid = userPid;
//            //sending terminate message to master
//            if( msgsnd(toMasterID, &message, sizeof(message), 0) == -1){
//                perror("send_message");
//                exit(1);
//            }
//            printf("./user: TERMINATE message sent...\n");

            gettimeofday(&tEnd, NULL);

            //calculating system time
            tDifference = (tEnd.tv_usec - tStart.tv_usec) * MICROSECOND;
            procControl->processBlock[index].procSysTime.tv_nsec = tDifference;

            //calculating runtime for the last time
            gettimeofday( &runtimeEnd, NULL );
            runtimeDiff = (runtimeEnd.tv_usec - runtimeStart.tv_usec) * MICROSECOND;
            runtime.tv_nsec += runtimeDiff;
            procControl->processBlock[index].procCpuTime.tv_nsec = runtime.tv_nsec;
            procControl->processBlock[index].state = 0;
            procControl->processBlock[index].localPid = -1;
            procControl->processBlock[index].pid = -1;

            //adding the time difference to CPU
//            printf("./user PID: %d, cpuTime: %ld ns\n", userPid,
//                   procControl->processBlock[index].procCpuTime.tv_nsec);
//            printf("./user PID: %d, sysTime: %ld ns\n", userPid,
//                   procControl->processBlock[index].procSysTime.tv_nsec);
            exit(0);
        } else if (diceRoll >= 0 && diceRoll < termPercent) {
            //the case of just processing
            printf("number is: %d. Processing...\n", diceRoll);

//            printf("./user PID: %d, cpuTime: %ld ns\n", userPid,
//                   procControl->processBlock[index].procCpuTime.tv_nsec);

            printf("./user PID: %d, runtime: %ld ns\n", userPid,
                   runtime.tv_nsec);
            printf("./user PID: %d, processQuantum: %ld ns\n", userPid,
                   processQuantum);
            gettimeofday( &runtimeEnd, NULL );
            runtimeDiff = (runtimeEnd.tv_usec - runtimeStart.tv_usec) * MICROSECOND;
            runtime.tv_nsec += runtimeDiff;
            procControl->processBlock[index].procSysTime.tv_nsec += runtimeDiff;
            procControl->processBlock[index].procCpuTime.tv_nsec += runtimeDiff;
            if (runtime.tv_nsec >= processQuantum) {
                //we are now going to wait because our quantum expired
                processWait(userPid);
//                messageReceive(userPid);
            }
            gettimeofday(&runtimeStart, NULL);

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
void printPCBState() {
    int i;
    printf("processBlock state:\n");
    //debugging output
    for (i = 0; i < 18; i++) {
        printf("%d ", procControl->processBlock[i].state);
    }
}

//function for finding pid from the PCB in the PCT
int findPCBIndex( pid_t myPid ){
    int i;
    printf("processBlock pid:\n");
    //debugging output
    for( i = 0; i < 18; i++) {
        printf( "%d ", procControl->processBlock[ i ].pid);
    }

    printf( "myPid: %d\n", myPid );
    for( i = 0; i < 18; i++ ){
        if( procControl->processBlock[ i ].pid == myPid ){
            return i;
        }
    }
    //return -1 or error if we cannot find anything
    return -1;
}

void processWait( pid_t pid ) {
    int index = findPCBIndex(pid);

    //debugging output
//    printf("./user PID: %d, cpuTime: %ld ns\n", pid,
//           procControl->processBlock[index].procCpuTime.tv_nsec);
//    printf("./user PID: %d, runtime: %ld ns\n", pid,
//           runtime.tv_nsec);

    message.mesg_type = 1;
    message.mesg_pid = pid;
    gettimeofday(&waitStart, NULL);
    printf("before wait:\n");
    printPCBState();
    procControl->processBlock[index].state = 0;
    printf("after wait:\n");
    printPCBState();

    strcpy(message.mesg_text, "PROC_WAIT");
    printf("./user %d: quantum expired sending WAIT message to ./oss: %s\n",
           pid, message.mesg_text);
    if( msgsnd(toMasterID, &message, sizeof(message.mesg_text), IPC_NOWAIT) == -1){
        perror("send_message");
        exit(1);
    }

//    signal( SIGTSTP, catcher );
//    signal( SIGCONT, catcher );
//    raise(SIGTSTP);
//    raise(SIGCONT);
//    while(pause()){
//        if(!wait) {
//            break;
//        }
//    }
    return;
}

void messageReceive( pid_t pid ){

    int index = findPCBIndex( pid );
    bool received = false;
        if ((messageStatus = msgrcv(toUserID, &message, sizeof(message),
                                    message.mesg_type, 0) >= 0)) {
            //receiving PROC_RESUME message from a process
            printf("./user %d: PROC_RESUME message received: %s\n", pid, message.mesg_text);
            if (strcmp(message.mesg_text, "PROC_RESUME") == 0) {
                //we are handling the case of resume
                gettimeofday(&waitEnd, NULL);
                waitDiff = (waitEnd.tv_usec - waitStart.tv_usec) * MICROSECOND;
                procControl->processBlock[index].procWaitTime.tv_nsec += waitDiff;
                procControl->processBlock[index].procSysTime.tv_nsec += waitDiff;
                procControl->processBlock[index].state = 1;
                printf("PID %d:process is resuming...\n", pid);
                //resetting runtime
                runtime.tv_nsec = 0;
                received = true;
            }
//            if (strcmp(message.mesg_text, "PROC_WAIT") == 0) {
//                //we are handling the case of wait
//                gettimeofday(&waitStart, NULL);
//                printf("PID %d:process is waiting...\n", pid);
//                //resetting runtime
//                runtime.tv_nsec = 0;
//            }
            //setting processing flag to false
        } else {
            perror("receive_message");
            exit(1);
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