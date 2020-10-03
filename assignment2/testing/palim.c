#include "palim.h"
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

bool isPalim( char * word );

FILE *outFilePtr1;

int main( int argc, char** argv){


    char *line = argv[argc - 1];
    bool palindrome;

    palindrome = isPalim( line );

    outFilePtr1 = fopen( "./output.log", "a");
    if( outFilePtr1 == NULL ) {
        printf("error!\n");
        exit(1);
    } else {
        printf( "writing to file...\n" );
        if (palindrome == true) {
            fputs(line, outFilePtr1);
            fputs(" is a palindrome.\n", outFilePtr1);
        } else {
            fputs(line, outFilePtr1);
            fputs(" is not a palindrome.\n", outFilePtr1);
        }
    }

    fclose( outFilePtr1 );
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
        printf( "character: %c %d \n", word[count], count);
        printf( "characterR: %c %d \n", word[reverseCount], reverseCount);

        if( word[count] == word[reverseCount] ){
            reverseCount--;
        } else {
            return false;
        }
    }

    //returning true
    return true;
}
