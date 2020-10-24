#include "palim.h"
#include "sharedheap.h"
#include "semstuff.h"
#include <semaphore.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <pthread.h>
#define PERMS (S_IRUSR | S_IWUSR)

bool isPalim( char *word, int currentIndex, int shm_id );
void processQueue( char* filename, char* outputWord );

//********************* for critical section implementation **********************
//void processQueue ( const int i, bool result, char* outputWord, int shm_id );

FILE *outFilePtr1;

sharedheap *sharedMem = NULL;

int main( int argc, char** argv){

    int zeroCount = 0;
    int currentIndex = atoi( argv[1] );
    char *line;

    int shm_id;
    key_t myKey;

    if( ( myKey = ftok( ".", 1) ) == (key_t) -1 ) {
        fprintf(stderr, "Failed to derive key from filename %s:%s\n",
                argv[argc - 1], strerror( errno ) );
    }
    //*********************shared memory portion**********************
    shm_id = shmget( myKey, 200, PERMS);
    //implementing ftok( )...
    if( shm_id == -1 ){
        if(errno != EEXIST){
            strerror( errno );
            perror( "palim.c: shared memory does not exist.\n");

        } else if ( (shm_id = shmget( myKey, sizeof(struct sharedheap), PERMS ) ) == -1) {
            perror( "palim.c: shared memory exist, just can't control it\n");
        }
    } else {
        printf("palim.c: success! Shared memory from derived key\n");
        sharedMem = shmat( shm_id, NULL, 0);

        //for the critical section implementation
//        sharedMem = (sharedheap) shmat( shm_id, NULL, 0);
//        line = sharedMem.sharedString;
        if(sharedMem == (void *) -1 ){
            perror("palim.c: shmat failed");
            exit(1);
        }
        else {
            printf( "palim.c: shmat returned %#8.8x\n", sharedMem);
        }
    }

    char *currentLine = &sharedMem->sharedString[0];
    while(zeroCount < currentIndex){
//        printf( "%c\n", *word);
        if( *currentLine == 0 ){
//            debug output only
//            printf( "palim.c: zeroCount: %d\n", zeroCount);
            zeroCount++;
        }
        currentLine++;
    }

    char *word;
    word = malloc( 100 );
    strcpy( word, currentLine);

    //line = strtok( line, "\n");
    bool palindrome;

    palindrome = isPalim( word, currentIndex, shm_id );

    return 0;
}

bool isPalim( char *word, int currentIndex, int shm_id){

//    printf( "word: %s\n", word);
    //we'll need a half way point for the loop
    int midpoint;
    char *outputWord = word;
//    debug output only
//    printf( "isPalim word: %s\n", word);
    if( strlen(word) % 2 == 0){
        //even case
        midpoint = ( (double)strlen(word)/ 2.0 ) - 1;
    } else {
        midpoint = ( (double)strlen(word)/ 2.0 );
    }

    //the index
    int count = 0;
    int reverseCount = strlen(word) - 1;
    while( !isalpha(word[reverseCount]) ){
        reverseCount--;
    }
    printf("midpoint: %d\n", midpoint);
    printf( "reverseCount: %d\n" , reverseCount);
    for(count; count <= midpoint ;count++){
        //debugging output
//        printf( "character: %c %d \n", word[count], count);
//        printf( "characterR: %c %d \n", word[reverseCount], reverseCount);

        //add isalnum(c) into this section... Goal is to make comparison with alpha and numerical.
        if( word[count] == word[reverseCount] ){
            reverseCount--;
        } else {
            printf( "going into processQueue\n");
            processQueue( "./nopalin.log", outputWord );
            return false;
        }
    }
    //returning true
    printf( "going into processQueue\n");
    processQueue( "./palin.log", outputWord );
    return true;
}

//******************************* Critical Section implementation ********************************************

void processQueue( char* filename, char* outputWord ) {
    int error;
    int semVal;
    int valResult;
    int waiting;
    int sem_id;

    key_t mySemKey;
    //********************** semaphore section ************************

    //grabbing key for semaphores
    if (( mySemKey = ftok( ".", 1 ) ) == (key_t)-1 ) {
        printf("Failed to derive sem key from filename\n");
        return;
    }

    if(( sem_id = semget( mySemKey, 1, PERMS)) == -1 ) {
        //failed to create a semaphore
        perror("Failed to create a semaphore");
        return;
    }

    struct sembuf semSignal[1];
    struct sembuf semWait[1];

    if (initelement( sem_id, 0, 1) == -1){
        printf("Failed to initialize semaphore element to 1");
        if (( error = semctl( sem_id, 0, IPC_RMID)) == -1 )
            printf("Failed to remove semaphore.\n");
        return;
    }
    //*****************************************************************


    setsembuf(semWait, 0, -1, 0);                            // decrement element 0
    setsembuf(semSignal, 0, 1, 0);                           // increment element 0
        if ((error = r_semop(sem_id, semWait, 1)) == -1) {
            printf("Child failed to lock semid: %s", error);
            return;
        } else if (!error) {            //we were able to lock the semaphore.
            //*********************** Critical Section start ********************************************
            //debugging output
            printf("going into wait\n");

            //Critical section here
            outFilePtr1 = fopen(filename, "a");
            if (outFilePtr1 == NULL) {
                perror("file did not open\n");
                exit(1);
            } else {
                printf("file opened.\n");
            }
            printf("process %d is writing to file:%s now.\n", sem_id, filename);
            if (strcmp(filename, "./palin.log") == 0) {
                //true case of palindrome
                fprintf(outFilePtr1, "%s is a palindrome\n", outputWord);
                printf("%s is a palindrome\n", outputWord);
                printf("closing file...\n");
                close(outFilePtr1);
                waiting = false;
            } else {  //false case of palindrome
                fprintf(outFilePtr1, "%s is not a palindrome\n", outputWord);
                printf("%s not is a palindrome\n", outputWord);
                printf("closing file...\n");
                fclose(outFilePtr1);
                waiting = false;
            }
            //********************** Exit section here ***********************************************
            if ((error = r_semop(sem_id, semSignal, 1)) == -1)
                perror("Failed to unlock semid");
        }
    //********************************** Remainder section *************************************

//    if(( error = semctl( sem_id, 0, IPC_RMID)) == -1 ){
//        printf("Failed to clean up\n");
//        return;
//    }
    return;
}
