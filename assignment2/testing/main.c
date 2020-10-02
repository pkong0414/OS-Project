//main.c

#include "detachandremove.h"
#include "sharedheap.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <stdbool.h>
#include <string.h>
#include "palim.h"
#define PERM (S_IRUSR | S_IWUSR)

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
                if( atoi(optarg) > 20 )
                    //sorry, 20 processes max.
                    nValue = 20;
                else
                    nValue = atoi( optarg );
                printf( "nValue: $s\n", optarg);
                break;
            case 's':
                if( atoi(optarg) > 19 )
                    //sorry, 20 processes max.
                    sValue = 19;
                else
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

    /*
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

    */

    //shared memory section
    int id;
    int *sharedtotal;
    int totalbytes = 0;
    sharedheap sharedMem;

    //shared memory grab
    if((sharedMem.id = shmget( IPC_PRIVATE, sizeof(int), PERM)) == -1){
        perror("Failed to create shared memory segment\n");
        return 1;
    } else {
        printf("created a new memory segment\n");
        sharedMem.sharedaddress = (char *)shmat( sharedMem.id, NULL, PERM);
    }

    if(detachandremove(sharedMem.id, sharedMem.sharedaddress) == -1){
        perror( "failed to destroy shared memory segment\n");
        return 1;
    } else {
        printf( "destroyed shared memory segment\n" );
    }
    return 0;

}

void printhelp(){


}