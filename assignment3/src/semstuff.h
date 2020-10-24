//semstuff.h

#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>

#ifndef SEMSTUFF_H
#define SEMSTUFF_H



int initelement( int sem_id, int semNum, int semValue);
int r_semop( int sem_id, struct sembuf *sops, int nsops );
int removesem( int sem_id );
void setsembuf( struct sembuf *s, int num, int op, int flg);


#endif