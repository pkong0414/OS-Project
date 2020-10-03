#include "palim.h"
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

bool isPalim( char * word );

FILE *fPtr;

int main( int argc, char** argv){

    fPtr = fopen( argv[ argc - 1 ], "r");

    if( fPtr == NULL ){
        printf( "error!\n" );
        exit(1);
    } else {
        char line[100];
        bool palindrome;
        int f;
        while( !feof(fPtr) ) {
            fgets( line, 100, fPtr);
            palindrome = isPalim(line);
            if( palindrome == true ) {
                printf("this is a palindrome.\n");
            }
            else {
                printf("this is not a palindrome\n");
            }
        }
    }

    fclose( FILE *fptr );


    return 0;
}

bool isPalim( char * word ){

    printf( "word: %s\n", word);
    char *ptr = (char*)malloc(sizeof(word));
    ptr = word;

    printf("ptr: %c\n", *ptr);
    //we'll need a half way point for the loop
    int midpoint;

    if( strlen(word) % 2 == 0){
        //even case
        midpoint = ( (double)strlen(word)/ 2.0 ) - 1;
    } else {
        midpoint = ( (double)strlen(word)/ 2.0 ) - 1;
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
        //printf( "character: %c %d \n", word[count], count);
        //printf( "characterR: %c %d \n", word[reverseCount], reverseCount);

        if( word[count] == word[reverseCount] ){
            reverseCount--;
        } else {
            return false;
        }
    }

    //returning true
    return true;
}
