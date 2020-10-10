//main.c

#include "detachandremove.h"
#include "sharedheap.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#define PERM (S_IRUSR | S_IWUSR)

void printhelp();

FILE *fPtr;
FILE *outPtr;

//this value will take in a max total of child processes master will ever create.
static int nValue = 4;

//this value will take in the number of children allowed to exist in the system at the same time.
static int sValue = 2;

//this value will take in seconds after which the process terminate, even if it has not finished.
static int tValue = 100;
static int processCount = 0;
static int stringsLeft = 0;

//shared memory section
key_t myKey;
static int shm_id;
static sharedheap sharedMem;

/* ARGUSED */
static void myhandler( int s ) {
    char timeout[] = "timing out processes.\n";
    int timeoutSize = sizeof( timeout );
    int errsave;

    errsave = errno;
    write(STDERR_FILENO, timeout, timeoutSize );
    errno = errsave;
    int i;

    //waiting for max amount of children to terminate
    for (i = 0; i <= nValue ; ++i) {
        wait(NULL);
    }
    //detaching shared memory
    if (detachandremove(shm_id, sharedMem.sharedString) == -1) {
        perror("failed to destroy shared memory segment\n");
        exit(0);
    } else {
        printf("destroyed shared memory segment\n");
    }
    exit(0);
}

static void myKillSignalHandler( int s ){
    char timeout[] = "caught ctrl+c, ending processes.\n";
    int timeoutSize = sizeof( timeout );
    int errsave;

    errsave = errno;
    write(STDERR_FILENO, timeout, timeoutSize );
    errno = errsave;
    int i;

    //waiting for max amount of children to terminate
    for (i = 0; i <= nValue ; ++i) {
        wait(NULL);
    }
    //detaching shared memory
    if (detachandremove(shm_id, sharedMem.sharedString) == -1) {
        perror("failed to destroy shared memory segment\n");
        exit(0);
    } else {
        printf("destroyed shared memory segment\n");
    }
    exit(0);
}

static int setupUserInterrupt( void ){
    struct sigaction act;
    act.sa_handler = myKillSignalHandler;
    act.sa_flags = 0;
    return (sigemptyset(&act.sa_mask) || sigaction(SIGINT, &act, NULL));
}

static int setupinterrupt( void ){
    struct sigaction act;
    act.sa_handler = myhandler;
    act.sa_flags = 0;
    return (sigemptyset(&act.sa_mask) || sigaction(SIGALRM, &act, NULL));
}

static int setupitimer(int tValue) {
    struct itimerval value;
    value.it_interval.tv_sec = tValue;
    value.it_interval.tv_usec = 0;
    value.it_value = value.it_interval;
    return (setitimer( ITIMER_REAL, &value, NULL));
}

int main( int argc, char** argv){

    if( setupUserInterrupt() == -1 ){
        perror( "failed to set up a user kill signal.\n");
        return 1;
    }

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
                if( atoi( optarg ) > 0 )
                    tValue = atoi( optarg );
                printf( "tValue: %d\n", tValue );
                break;
            default:
                break;
        }
    }

    //setting up interrupts
    if (setupinterrupt() == -1) {
        perror("Failed to set up handler for SIGALRM");
        return 1;
    }

    if (setupitimer(tValue) == -1) {
        perror("Failed to set up the ITIMER_PROF interval timer");
        return 1;
    }

    if( ( myKey = ftok( ".", 1 ) ) == (key_t)-1 ) {
        fprintf(stderr, "Failed to derive key from filename %s:%s\n",
                argv[argc - 1], strerror( errno ) );
        return 1;
    }
    //shared memory grab
    shm_id = shmget( myKey, 200, PERM | IPC_CREAT | IPC_EXCL);

    //implementing ftok( )...


    if( shm_id == -1 ){
        if(errno != EEXIST){
            perror( "shared memory already exist.\n");

        } else if ( (shm_id = shmget( myKey, sizeof(sharedheap), PERM ) ) == -1) {
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
        int concurProcesses = 0;
        pid_t childPid;
        int index;
        int processCount = 0;
        int status = 0;

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

        printf( "outside the loop (stringsLeft): %d\n", stringsLeft);

        //we'll never make more processes than the max processes allowed!
        while( processCount < nValue ) {

            if( concurProcesses < sValue ) {

                //incrementing total processes created
                processCount++;
                //increment active processes
                concurProcesses++;
                stringsLeft--;
                printf("processCount: %d\n", processCount);
                printf("concurProcesses: %d\n", concurProcesses);
                printf("stringsLeft: %d\n", stringsLeft);
                //we don't want the forking to extend beyond the end of file!
                if (stringsLeft < 0) {
                    printf("File is empty. Terminating program\n");
                    if (detachandremove(shm_id, sharedMem.sharedString) == -1) {
                        perror("failed to destroy shared memory segment\n");
                    } else {
                        printf("destroyed shared memory segment\n");
                    }
                    break;
                }

                //we don't want more processes than for what is allowed (even if nProcess is less than sValue)!
                if (processCount == nValue) {
                    printf("processCount: %d\n", processCount);
                    printf("Max processing limit reached!\n");
                    break;
                    //exit(42);
                }

                childPid = fork();

                if ( childPid < 0) {
                    perror("failed to fork");
                    exit(1);
                }

                if( childPid == 0) {
                    //************************ Child Process ******************************//

                    //debugging output
                    //processCount++;

                    printf("\n%d: My PID = %d\n", f, (int) getpid());
                    printf("concurProcesses: %d\n", concurProcesses);
                    printf("\n");
                    char argStrings[10];

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
                }

                    int activeProcesses;
                    for (activeProcesses = 0; activeProcesses <= sValue; activeProcesses++) {
                        //******************* PARENT PROCESS *********************************//
                        //parent is waiting for all the children to end process
                        wait(&status);

                        //decrementing number of processes concurrent
                        concurProcesses--;

                        //breaking out of the wait process when the number of concurrent processes is out
                        if(concurProcesses == 0 ){
                            break;
                        }

                        //incrementing total processes created
                        //processCount++;

                        printf("processCount: %d\n", (processCount));
                        printf("process is finished!\n");
                }
                if(processCount == nValue){
                    break;
                }
            }


            //we don't want more processes than for what is allowed (even if nProcess is less than sValue)!
            if(processCount == nValue){
                break;
            }
        }
    }
    if (detachandremove(shm_id, sharedMem.sharedString) == -1) {
        perror("failed to destroy shared memory segment\n");
    } else {
        printf("destroyed shared memory segment\n");
    }
    return 0;
}

void printhelp(){

}