//msgQueue.h

#include <sys/shm.h>
#include <sys/ipc.h>

#ifndef MSGQUEUE_H
#define MSGQUEUE_H

typedef struct mesg_buffer {
    long mesg_type;
    char mesg_text[100];
} mesg_buffer;

void initMsgQueue(key_t userOutKey, int userMsgId, key_t masterInKey, int masterMsgId);

#endif