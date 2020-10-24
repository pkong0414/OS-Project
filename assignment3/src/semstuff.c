//semstuff.c

#include "semstuff.h"
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>

int initelement( int sem_id, int semNum, int semValue) {
    union semun {
        int val;
        struct semid_ds *buf;
        unsigned short *array;
    } arg;
    arg.val = semValue;
    return semctl( sem_id, semNum, SETVAL, arg );
}

int r_semop( int sem_id, struct sembuf *sops, int nsops ) {
    while (semop(sem_id, sops, nsops) == -1) {
        if (errno != EINTR) {
            return -1;
        }
    }
    return 0;
}

int removesem( int sem_id ) {
    return semctl( sem_id, 0, IPC_RMID );
}

void setsembuf( struct sembuf *s, int num, int op, int flg) {
    s->sem_num = (short)num;
    s->sem_op = (short)op;
    s->sem_flg = (short)flg;
    return;
}



#define PERMS (S_IRUSR | S_IWUSR)
#define SET_SIZE 20

