#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define MSG_KEY 1234
#define MAX_TEXT 512

struct msg_buffer { long msg_type; char msg_text[MAX_TEXT]; };

int main(void) {
    int msgid = msgget(MSG_KEY, 0666 | IPC_CREAT);
    struct msg_buffer msg;
    
    printf("等待消息...\n");
    while (msgrcv(msgid, &msg, MAX_TEXT, 1, 0) > 0) {
        printf("收到: %s\n", msg.msg_text);
    }
    msgctl(msgid, IPC_RMID, NULL);
    return 0;
}
