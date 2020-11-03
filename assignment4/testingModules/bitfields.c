//bitfields.c
#include <stdio.h>
#include <stdlib.h>

int main( int argc, char ** argv) {
    unsigned int v;
    unsigned int c;

    for( c = 0; v; v >>= 1) {
        c+= v & 1;
    }

    return 0;
}