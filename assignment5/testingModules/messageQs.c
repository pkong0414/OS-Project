// C Program for Message Queue (Writer Process)
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
    int msgToR;
    int msgToQ;

    // ftok to generate unique key
    key = ftok("progfile", 65);

    // msgget creates a message queue
    // and returns identifier
    msgToR = msgget(key, 0666 | IPC_CREAT);
    msgToQ = msgget(key, 0666 | IPC_CREAT);
    message.mesg_type = 1;

    printf("Write Data : ");
    gets(message.mesg_text);

    // msgsnd to send message
    msgsnd(msgToR, &message, sizeof(message), 0);
    msgrcv(msgToQ, &message, sizeof(message), 2, 0);
    if( strcmp(message.mesg_text, "REPLYING") == 0){
        printf("messageR received my message.\n");
    }

    // display the message
    printf("Data send is : %s \n", message.mesg_text);

    return 0;
}
