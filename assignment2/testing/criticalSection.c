//criticalSection.c

extern bool choosing[n]; /* Shared Boolean array */
extern int number[n]; /* Shared integer array to hold turn number */


void process_i ( const int i ) /* ith Process */
{
    do
        choosing[i] = true;
    number[i] = 1 + max(number[0], ..., number[n-1]);
    choosing[i] = false;
    for ( int j = 0; j < n; j++ )
    {
        while ( choosing[j] ); // Wait while someone else is choosing
        while ( ( number[j] ) && (number[j],j) < (number[i],i) );
    }
    critical_section();
    number[i] = 0;
    remainder_section();
    while ( 1 );
}

extern bool lock ( false );
do {
while ( test_and_set ( lock ) );

critical_section();

lock = false;

remainder_section();

}while ( 1 );

