/*
 * 檔案名稱: msg_sender.c
 * 功能說明: System V 消息隊列 - 發送端
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define MSG_KEY 1234
#define MAX_TEXT 512

struct msg_buffer {
    long msg_type;
    char msg_text[MAX_TEXT];
};

int main(void)
{
    int msgid = msgget(MSG_KEY, 0666 | IPC_CREAT);
    if (msgid == -1) {
        perror("msgget");
        exit(1);
    }
    
    printf("消息隊列 ID: %d\n", msgid);
    printf("輸入消息（'quit' 退出）：\n");
    
    struct msg_buffer msg;
    while (1) {
        printf("> ");
        if (fgets(msg.msg_text, MAX_TEXT, stdin) == NULL) {
            break;  /* EOF 或錯誤 */
        }
        msg.msg_text[strcspn(msg.msg_text, "\n")] = 0;
        
        if (strcmp(msg.msg_text, "quit") == 0) break;
        
        msg.msg_type = 1;
        msgsnd(msgid, &msg, strlen(msg.msg_text) + 1, 0);
        printf("✓ 已發送\n");
    }
    return 0;
}
