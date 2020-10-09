#include "palim.h"
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
#define KEY 0x12345678


bool isPalim( char * word );
FILE *outFilePtr1;
FILE *outFilePtr2;
int main( int argc, char** argv){

    int zeroCount = 0;
    int currentIndex = atoi( argv[1] );
    char *line;

    int shm_id;

    //*********************shared memory portion**********************
    shm_id = shmget( KEY, 200, PERM);
    //implementing ftok( )...
    if( shm_id == -1 ){
        if(errno != EEXIST){
            perror( "palim.c: shared memory already exist.\n");

        } else if ( (shm_id = shmget( KEY, 200, PERM ) ) == -1) {
            perror( "palim.c: shared memory exist, just can't control it\n");
        }
    } else {
        printf("palim.c: created a new memory segment\n");
        line = (char *) shmat( shm_id, NULL, 0);
        if(line == (char *) -1 ){
            perror("palim.c: shmat failed");
            exit(1);
        }
        else {
            printf( "palim.c: shmat returned %#8.8x\n", line);
        }
    }
    //*****************************************************************

    char *currentLine = line;
    while(zeroCount < currentIndex){
//        printf( "%c\n", *word);
        if( *currentLine == 0 ){
            printf( "palim.c: zeroCount: %d\n", zeroCount);
            zeroCount++;
        }
        currentLine++;
    }

    char *word;
    word = malloc( 100 );
    strcpy( word, currentLine);

    //line = strtok( line, "\n");
    bool palindrome;

    outFilePtr1 = fopen( "./palin.log", "w+");
    outFilePtr2 = fopen( "./nopalin.log", "w+");
    if( outFilePtr1 == NULL || outFilePtr2 == NULL ) {
        perror("file did not open\n");
        exit(1);
        printf( "writing to file...\n" );
    } else {
        palindrome = isPalim( word );
    }

    fclose( outFilePtr1 );
    fclose( outFilePtr2 );
    return 0;
}

bool isPalim( char *word ){

//    printf( "word: %s\n", word);
    //we'll need a half way point for the loop
    int midpoint;
    char *outputWord = word;
    printf( "isPalim word: %s\n", word);
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

        if( word[count] == word[reverseCount] ){
            reverseCount--;
        } else {
            printf("writing to file\n");
            fprintf(outFilePtr2, "%s is not a palindrome\n", outputWord);
            printf( "%s is not a palindrome\n", outputWord);
            return false;
        }
    }
    //returning true
    printf("writing to file\n");
    fprintf(outFilePtr1, "%s is a palindrome\n", outputWord);
    printf( "%s is a palindrome\n", outputWord);
    return true;
}
