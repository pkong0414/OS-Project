//msgQueue.c

#include "msgQueue.h"
#include <stdio.h>



void initMsgQueue(key_t userOutKey, int userMsgId, key_t masterInKey, int masterMsgId){
    // ftok to generate unique key
    //this one will be used for the processing to master queues
    userOutKey = ftok("processOutMsg", 65);

    if (userOutKey == -1) { //checking if we derived a key
        printf("failed to derive a key for userOutKey\n");
        return;
    }

    // msgget creates a message queue
    // and returns identifier
    userMsgId = msgget(userOutKey, 0666 | IPC_CREAT);

    if (userMsgId == -1) { //checking see if msgget worked
        printf("failed to attach userOutKey\n");
        return;
    }

    //ftok to generate unique key
    //this one will be use for receiving messages from the master
    masterInKey = ftok("MasterInMsg", 66);

    if (masterInKey == -1) { //checking if we derived a key
        printf("failed to derive a key for masterInKey\n");
        return;
    }

    //msgget to generate another message queue
    masterMsgId = msgget(masterMsgId, 0666 | IPC_CREAT);

    if (masterMsgId == -1) { //checking see if msgget worked
        printf("failed to attach masterInKey\n");
        return;
    }

}