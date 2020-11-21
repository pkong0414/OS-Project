// C Program for Message Queue (Reader Process)
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>

// structure for message queue
struct mesg_buffer {
    long mesg_type;
    char mesg_text[100];
} message;

int main()
{
    key_t key;
    int msgToQ;
    int msgToR;

    // ftok to generate unique key
    key = ftok("progfile", 65);

   // msgget creates a message queue
    // and returns identifier
    msgToQ = msgget(key, 0666 | IPC_CREAT);
    msgToR = msgget(key, 0666 | IPC_CREAT);

    printf( "key: %d\n", key);
    printf( "msgToR: %d\n", msgToR);
    printf( "msgToQ: %d\n", msgToQ);

    // msgrcv to receive message
    msgrcv(msgToR, &message, sizeof(message), 1, 0);


    // display the message
    printf("Data Received is : %s \n", message.mesg_text);
    message.mesg_type = 2;
    strcpy(message.mesg_text, "REPLYING");

    msgsnd(msgToQ, &message, sizeof(message), 0);

    // to destroy the message queue
    msgctl(msgToR, IPC_RMID, NULL);
    msgctl(msgToQ, IPC_RMID, NULL);

    return 0;
}