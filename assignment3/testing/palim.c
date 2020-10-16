#include "palim.h"
#include "sharedheap.h"
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
#define PERM (S_IRUSR | S_IWUSR)

bool isPalim( char *word, int currentIndex, int shm_id );

//********************* for critical section implementation **********************
//void processQueue ( const int i, bool result, char* outputWord, int shm_id );

FILE *outFilePtr1;
FILE *outFilePtr2;

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
    shm_id = shmget( myKey, 200, PERM);
    //implementing ftok( )...
    if( shm_id == -1 ){
        if(errno != EEXIST){
            strerror( errno );
            perror( "palim.c: shared memory does not exist.\n");

        } else if ( (shm_id = shmget( myKey, sizeof(struct sharedheap), PERM ) ) == -1) {
            perror( "palim.c: shared memory exist, just can't control it\n");
        }
    } else {
        printf("palim.c: created a new memory segment\n");
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
    //*****************************************************************

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

    outFilePtr1 = fopen( "./palin.log", "a");
    outFilePtr2 = fopen( "./nopalin.log", "a");
    if( outFilePtr1 == NULL || outFilePtr2 == NULL ) {
        perror("file did not open\n");
        exit(1);
        printf( "writing to file...\n" );
    } else {
        palindrome = isPalim( word, currentIndex, shm_id );
    }

    fclose( outFilePtr1 );
    fclose( outFilePtr2 );
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

//            processQueue( currentIndex, false, outputWord, shm_id);
            printf("writing to file\n");
            fprintf(outFilePtr2, "%s is not a palindrome\n", outputWord);
            printf( "%s is not a palindrome\n", outputWord);
            return false;
        }
    }
    //returning true
//    processQueue( currentIndex, true, outputWord, shm_id);
    printf("writing to file\n");
    fprintf(outFilePtr1, "%s is a palindrome\n", outputWord);
    printf( "%s is a palindrome\n", outputWord);
    return true;
}

//******************************* Critical Section implementation ********************************************

//void processQueue ( const int processingIndex, bool result, char* outputWord, int shm_id ) /* ith Process */
//{
//    bool finished = false;
////    //************************ shared memory section ***********************************
////    sharedMem.sharedQueue = shmat( shm_id, NULL, 0);
////    if(sharedMem.sharedQueue == -1 ){
////        perror("shmat sharedMem.sharedQueue failed");
////    }
////    else {
////        printf( "shmat sharedMem.sharedQueue returned %#8.8x\n", sharedMem.sharedQueue);
////    }
////
////    sharedMem.choosing = shmat( shm_id, NULL, 0);
////    if(sharedMem.choosing == -1 ){
////        perror("shmat sharedMem.choosing failed");
////    }
////
////    else {
////        printf( "shmat sharedMem.choosing returned %#8.8x\n", sharedMem.choosing);
////    }
////
////    sharedMem.ticket = shmat( shm_id, NULL, 0);
////    if(sharedMem.ticket == -1 ){
////        perror("shmat sharedMem.ticket failed");
////    }
////    else {
////        printf( "shmat sharedMem.ticket returned %#8.8x\n", sharedMem.ticket);
////    }
////    //***********************************************************************************
//
//
//    do {
//        sharedMem->choosing[processingIndex] = true;
//        //number is the id
//        sharedMem->sharedQueue[processingIndex] = sharedMem->ticket;       //giving ticket to the processes that come along.
//        sharedMem->ticket++;
//        sharedMem->choosing[processingIndex] = false;
//        int j;
//        for (j = 0; j < 20; j++) {
//            while (sharedMem->choosing[j]); // Wait while someone else is choosing
//            while ((sharedMem->sharedQueue[j]) && (sharedMem->sharedQueue[j], j) < (sharedMem->sharedQueue[processingIndex], processingIndex));
//        }
//
//        //******************* Critical Section *************************************
//        if( result == false ) {
//            printf("writing to file\n");
//            fprintf(outFilePtr2, "%s is not a palindrome\n", outputWord);
//            printf( "%s is not a palindrome\n", outputWord);
//        } else {
//            printf("writing to file\n");
//            fprintf(outFilePtr1, "%s is a palindrome\n", outputWord);
//            printf( "%s is a palindrome\n", outputWord);
//        }
//        //**************************************************************************
//        sharedMem->sharedQueue[processingIndex] = 0;
//        finished = true;
//    }while (finished = false);
//    //processing finishes and we hop out of queue
//    return;
//}
