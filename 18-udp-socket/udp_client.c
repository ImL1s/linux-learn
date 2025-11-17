/*
 * 檔案名稱: udp_client.c
 * 功能說明: UDP 客戶端示例
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8888
#define BUFFER_SIZE 1024

int main(void) {
    int sockfd;
    struct sockaddr_in server_addr;
    char send_buf[BUFFER_SIZE], recv_buf[BUFFER_SIZE];
    socklen_t addr_len = sizeof(server_addr);
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(PORT);
    
    printf("UDP 客戶端啟動，連接到 %s:%d\n", SERVER_IP, PORT);
    printf("輸入消息（'quit' 退出）：\n");
    
    while (1) {
        printf("> ");
        if (!fgets(send_buf, BUFFER_SIZE, stdin)) break;
        
        if (strncmp(send_buf, "quit", 4) == 0) {
            break;
        }
        
        // 發送
        ssize_t sent = sendto(sockfd, send_buf, strlen(send_buf), 0,
                              (struct sockaddr*)&server_addr, sizeof(server_addr));
        if (sent == -1) {
            perror("sendto");
            continue;
        }

        // 接收回顯
        int n = recvfrom(sockfd, recv_buf, BUFFER_SIZE, 0,
                         (struct sockaddr*)&server_addr, &addr_len);
        recv_buf[n] = '\0';
        printf("服務器回應: %s", recv_buf);
    }
    
    close(sockfd);
    return 0;
}
