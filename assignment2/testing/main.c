//main.c

#include "detachandremove.h"
#include "sharedheap.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
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

    int f = 1;
    int childNum = 0;
    //forking processes
    for( f; f <= 2; f++){
        //forking
        pid_t pid = fork();
        printf("%d: My PID = %d\n", f, (int) getpid());
        if( pid < 0 )
            perror( "fork failed.\n");
        if( pid == 0 ){
            printf( "I am the child with PID: %d\n", (int) getpid() );

            //shared memory section
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

            //exit(42);
        }

    }



    //shared memory section
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

    /*
    printf("I am the parent, waiting for child to end.\n");
    sleep(10);

    int status = 0;
    pid_t childpid = wait( &status );
    printf("Parent received message child %d is finished with status: %d\n", (int)childpid, status);
    int childReturnValue = WEXITSTATUS(status);
    printf("Return value was %d\n", childReturnValue);
    sleep(1);
    */
    return 0;

}

void printhelp(){


}