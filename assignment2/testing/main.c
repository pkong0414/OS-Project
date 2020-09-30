//main.c

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <string.h>
#include "palim.h"

void printhelp();

FILE *fPtr;

int main( int argc, char** argv){

    //this value will take in a max total of child processes master will ever create.
    int nValue = 4;

    //this value will take in the number of children allowed to exist in the system at the same time.
    int sValue = 2;

    //this value will take in seconds after which the process terminate, even if it has not finished.
    int tValue = 100;
    int c;

    while ( ( c = getopt(argc, argv, "hn:s:t:" ) ) != -1 ) {
        switch (c) {
            case 'h':
                //TODO make a printhelp function
                break;
            case 'n':
                nValue = atoi( optarg );
                printf( "nValue: $s\n", optarg);
                break;
            case 's':
                sValue = atoi( optarg );
                printf( "sValue: '%s'\n", optarg );
                break;
            case 't':
                tValue = atoi( optarg );
                printf( "tValue: '%s'\n", optarg );
                break;
            default:
                abort();
        }
    }

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

    return 0;

}

void printhelp(){


}