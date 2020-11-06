//quantumSleep.c

/* This function will sleep for 1 ms ( 1000000 ns )
 * We are using the nanotest.c example of the book to make this quantum count
 * */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#define COUNT 1
#define D_BILLION 1000000000.0
#define D_MILLION 1000000.0
#define MILLION 1000000L
#define NANOSECONDS 1000
#define MILLISECONDS 1000000

long quantumSleep() {
    int i;
    struct timespec slptm;
    long tdif;
    struct timeval tend, tstart;


    slptm.tv_sec = 0;
    // we need to set this so that sleep till be in terms of quantum
    // or 1000000 nanosecs
    slptm.tv_nsec = ( MILLISECONDS );
    if(gettimeofday(&tstart, NULL) == -1) {
        fprintf( stderr, "Failed to get start time\n");
        return 1;
    }

    for(i = 0; i < COUNT; i++)
        if( nanosleep(&slptm, NULL) == -1) {
            perror( "Failed to nanosleep" );
            return 1;
        }
    if( gettimeofday(&tend, NULL) == -1) {
        fprintf(stderr, "Failed to get end time\n");
        return 1;
    }

    //debugging output
    tdif = MILLION*( tend.tv_sec - tstart.tv_sec) +
           tend.tv_usec - tstart.tv_usec;
    printf("%d nanosleeps of %d nanoseconds or 1 millisecond\n", COUNT, (NANOSECONDS*NANOSECONDS ));
    printf("Should take %11d microseconds or %f seconds\n",
           NANOSECONDS*COUNT/1000, NANOSECONDS*COUNT/D_BILLION);
    printf("Number of seconds per nanosleep was     %f\n",
           (tdif/(double)COUNT)/MILLION);
    printf("Number of seconds per nanosleep should be %f\n",
           NANOSECONDS/D_BILLION);
    return MILLISECONDS;
}