//user.c

#include "sharedMemory.h"
#include "queue.h"
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

//macros
#define PERM (S_IRUSR)

// structure for message queue
struct mesg_buffer {
    long mesg_type;
    char mesg_text[100];
} message;

//function prototypes
void initSharedMem();
void initMsgQueue();

//message queue globals
key_t userKey;
key_t masterKey;
int userId;
int masterId;

//shared memory section
key_t myKey;
key_t key;
static int shm_id;
static sharedMem *procControl;

int main( int argc, char** argv ) {
    int userPid;
    int diceRoll;
    srand(time(0));
    printf( "inside process: %d\n",(int)getpid() );
    //initializing message queue
    initMsgQueue();

    while(1) {
        userPid = getpid();
        message.mesg_type = userPid;
        //we'll just keep rolling the dice till we get terminate
        diceRoll = ( rand() % 100 );
        printf( "rolled number: %d\n", diceRoll);
        if( diceRoll >= 80){
            //termination works, so now we'll disable this to get scheduling with the queue working.
            //the case of termination
            printf( "number greater than 8. Terminating\n");
            strcpy(message.mesg_text, "TERMINATE");
            message.mesg_type = 1;
            //sending terminate message to master
            msgsnd(masterId, &message, sizeof(message), 0);
            printf( "TERMINATE message sent...\n");
            break;
        }
        else if( diceRoll >= 0   && diceRoll < 80) {
            //the case of just processing
            printf( "number is: %d. Processing...\n", diceRoll);
        }
    }
    //************************************ finishing details *******************************

    msgctl(userId, IPC_RMID, NULL);
    msgctl(masterId, IPC_RMID, NULL);

    printf( "./user: destroyed message queues\n");
    return 0;
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