//pause.c
// goal of this program will be to test out the pause() function

#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

static int i = 1;
static int tValue = 3;

/* ARGUSED */
static void myhandler( int s ) {
    char timeout[] = "./oss handler: timing out processes.\n";
    int timeoutSize = sizeof( timeout );
    int errsave;

    errsave = errno;
    write(STDERR_FILENO, timeout, timeoutSize );
    errno = errsave;
    i = 0;
}

static void myhandler2( int s ){
    i = 1;
}

static void myKillSignalHandler( int s ) {
    char timeout[] = "./pause: caught ctrl+c, unpausing processes.\n";
    int timeoutSize = sizeof(timeout);
    int errsave;

    errsave = errno;
    write(STDERR_FILENO, timeout, timeoutSize);
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


int main( int argc, char **argv ){
    //interrupt signal
    if( setupUserInterrupt() == -1 ){
        perror( "./pause: failed to set up a user kill signal.\n");
        return 1;
    }

    //setting up interrupts
    if (setupinterrupt() == -1) {
        perror("./oss: Failed to set up handler for SIGALRM");
        return 1;
    }

    if (setupitimer(tValue) == -1) {
        perror("./oss: Failed to set up the ITIMER_PROF interval timer");
        return 1;
    }


    printf("I'm now pausing myself...\n");
    signal(SIGUSR1, myhandler2);
    while( i )
        pause();

    printf("I am now unpaused...\n");

    printf("am I going to pause again?\n");
    raise(SIGURS1);
    return 0;
}