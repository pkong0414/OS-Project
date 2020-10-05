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
#include <errno.h>
#define PERM (S_IRUSR | S_IWUSR)
#define KEY 0x12345678

void printhelp();

FILE *fPtr;
FILE *outPtr;

int main( int argc, char** argv){

    //this value will take in a max total of child processes master will ever create.
    int nValue = 4;

    //this value will take in the number of children allowed to exist in the system at the same time.
    int sValue = 2;

    //this value will take in seconds after which the process terminate, even if it has not finished.
    int tValue = 100;
    int c;

    char *outPath = "output.log";

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
                break;
        }
    }

    //shared memory section
    sharedheap sharedMem;


    //shared memory grab
    sharedMem.id = shmget( KEY, sizeof( sharedheap ), PERM | IPC_CREAT | IPC_EXCL);

    printf( "hellow world \n");
    if( sharedMem.id == -1 ){
        if(errno != EEXIST){
           perror( "shared memory already exist.\n");
        } else if ( (sharedMem.id = shmget( KEY, sizeof( sharedheap ), PERM ) ) == -1) {
            perror( "shared memory exist, just can't control it\n");
        }
    }

//    sharedMem->sharedString = (char *)shmat( sharedMem->id, NULL, 0 );
//    if( sharedMem->sharedString == (char *) -1 ){
//        perror( "could not attach shared memory.\n");
//    }


    else {
        printf("created a new memory segment\n");
        sharedMem.sharedString = (char **)shmat( sharedMem.id, NULL, 0);
        if(sharedMem.sharedString == (char **) -1 ){
            perror("shmat failed");
        }
        else {
            printf( "shmat returned %#8.8x\n", sharedMem.sharedString);
        }
    }

    //opening file
    fPtr = fopen( argv[ argc - 1 ], "r");

    if( fPtr == NULL ){
        printf( "error! Cannot open the input file.\n" );
        exit(1);
    } else {
        char line[100];
        int i = 0;
        sharedMem.sharedString = malloc( 100 );
        while( !feof(fPtr) ) {
            fgets( line, 100, fPtr);
            printf( "line: %s\n", line );

            sharedMem.sharedString[i] = malloc( strlen(line) + 1 );
            strcpy(sharedMem.sharedString[i], line );
            printf( "args: %s\n", sharedMem.sharedString[i++]);
        }

    }

    int argCount = 0;
    for( argCount ; argCount < 4 ; argCount++) {
        printf("sharedMem.sharedString: %s\n", sharedMem.sharedString[argCount]);
    }
    outPtr =  fopen( outPath, "a" );

    if ( outPtr == NULL ){
        printf( "error! Cannot open output file.\n" );
        exit(1);
    }

    fclose( fPtr );

    int f = 1;
    int childNum = 0;
    //forking processes
    for( f; f <= 2; f++){
        //forking
        pid_t pid = fork();
        printf("\n%d: My PID = %d\n", f, (int) getpid());
        if( pid < 0 )
            perror( "fork failed.\n");
        if( pid == 0 ){
            printf( "I am the child with PID: %d\n", (int) getpid() );

            //exit(42);
        }

    }


    if(detachandremove(sharedMem.id, sharedMem.sharedString) == -1){
        perror( "failed to destroy shared memory segment\n");
        return 1;
    } else {
        printf( "destroyed shared memory segment\n" );
    }

    /*
    //shared memory grab
    if((sharedMem.id = shmget( KEY, sizeof(struct sharedheap), PERM | IPC_CREAT)) == -1){
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
    */

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