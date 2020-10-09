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
#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#define PERM (S_IRUSR | S_IWUSR)
#define KEY 0x12345678

void printhelp();
void sigquit(int);

FILE *fPtr;
FILE *outPtr;

static int processCount = 0;

//shared memory section
static int shm_id;
static sharedheap sharedMem;

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
                printf("nValue: %d\n", nValue);
                break;
            case 's':
                if( atoi(optarg) > 19 )
                    //sorry, 20 processes max.
                    sValue = 19;
                else
                    sValue = atoi( optarg );
                printf("sValue: %d\n", sValue);
                break;
            case 't':
                tValue = atoi( optarg );
                printf( "tValue: '%s'\n", optarg );
                break;
            default:
                break;
        }
    }

    //shared memory grab
    shm_id = shmget( KEY, 200, PERM | IPC_CREAT | IPC_EXCL);

    //implementing ftok( )...


    if( shm_id == -1 ){
        if(errno != EEXIST){
           perror( "shared memory already exist.\n");

        } else if ( (shm_id = shmget( KEY, sizeof(sharedheap), PERM ) ) == -1) {
            perror( "shared memory exist, just can't control it\n");
        }
    } else {
        printf("created a new memory segment\n");
        sharedMem.sharedString = shmat( shm_id, NULL, 0);
        if(sharedMem.sharedString == (char *) -1 ){
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

        int f = 1;
        int childNum = 0;
        //forking processes
        int nProcesses = 0;
        pid_t childPid;
        int index;
        int processCount = 0;
        int stringsLeft = 0;

        char *line;
        line = malloc( 100 );
        char *token;
        //forking
        char *dest = sharedMem.sharedString;
        while( fgets(line, 100,fPtr ) != NULL ){
            strtok( line, "\r\n");
            printf( "in main: line: %s\n", line);
            strcpy( dest, line);
            dest += strlen(line) + 1;
            *dest = 0;
            stringsLeft++;
        }

        //closing file after use
        fclose(fPtr);

        //kill switch
        signal(SIGINT, sigquit);
        int allowedProcesses = nValue;
        //we'll never make more processes than the max processes allowed!
        for( ;allowedProcesses > 0;) {
            for (nProcesses = 0; (nProcesses < allowedProcesses && nProcesses < sValue); nProcesses++) {

                //decrementing strings
                stringsLeft--;
                processCount++;
                nProcesses++;
                allowedProcesses = nValue - (processCount + 1);
                printf( "processCount: %d\n", processCount );
                printf( "allowedProcesses: %d\n", allowedProcesses );
                printf( "nProcesses: %d\n", nProcesses);
                printf( "stringsLeft: %d\n", stringsLeft );
                //we don't want the forking to extend beyond the end of file!
                if(stringsLeft < 0 ){
                    printf("File is empty. Terminating program\n");
                    break;
                }

                //stopping the process of forking because process limit reached!
                if( allowedProcesses <= 0 ) {
                    printf( "maximum processes reached!\n");
                    break;
                }
                //we don't want more processes than for what is allowed (even if nProcess is less than sValue)!
                if( ( processCount ) == nValue ){
                    printf("Max processing limit reached! ");
                    break;
                    //exit(42);
                }

                //fanning the forks
                if ((childPid = fork()) <= 0) {
                    break;
                }


            }
            if( childPid == 0) {
                //************************ Child Process ******************************//

                //debugging output
                //processCount++;
                allowedProcesses = nValue - (processCount + 1);

                printf("nProcesses: %d\n", nProcesses);
                printf("\n%d: My PID = %d\n", f, (int) getpid());
                printf("\n");
                char argStrings[10];
                if (allowedProcesses < 0) {
                    printf("Max processing limit reached! ");

                    return 0;
                    //exit(42);
                }

                printf("stringsLeft: %d\n", stringsLeft);
                sprintf(argStrings, "%d", stringsLeft);
                printf("argStrings: %s\n", argStrings);

                char *arguments[] = {"./palimCheck", argStrings, NULL};
                int i;
                for (i = 0; i < 3; i++) {
                    printf("arg[%d]: %s\n", i, arguments[i]);
                }
                printf("I am the child with PID: %d\n", (int) getpid());

                //fork success now exec here
                printf("execing...\n");
                execv(arguments[0], arguments);

                //add perror(), incase something fails.
                perror("exec failed\n");
            } else {
                //******************* PARENT PROCESS *********************************//

                //parent is waiting for all the children to end process
                wait(NULL);

                //incrementing processCount
                processCount++;


                printf("processCount + 1: %d\n", (processCount + 1));
                printf("process is finished!\n");

                //we don't want more processes than for what is allowed (even if nProcess is less than sValue)!
            }
        }
    }

    if (detachandremove(shm_id, sharedMem.sharedString) == -1) {
        perror("failed to destroy shared memory segment\n");
        return 1;
    } else {
        printf("destroyed shared memory segment\n");
    }
    return 0;
}

void printhelp(){

}

void sigquit( int signum ){
    printf( "Caught signal %d, quitting...\n", signum );
    if (detachandremove(shm_id, sharedMem.sharedString) == -1) {
        perror("failed to destroy shared memory segment\n");
    } else {
        printf("destroyed shared memory segment\n");
    }
    exit(1);
}