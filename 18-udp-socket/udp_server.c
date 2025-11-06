/*
 * 檔案名稱: udp_server.c
 * 功能說明: UDP 服務器示例
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8888
#define BUFFER_SIZE 1024

int main(void) {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(client_addr);
    
    // 創建UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    // 綁定
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(1);
    }
    
    printf("UDP 服務器啟動，監聽端口 %d\n", PORT);
    
    while (1) {
        // 接收數據
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                         (struct sockaddr*)&client_addr, &addr_len);
        
        if (n < 0) {
            perror("recvfrom");
            continue;
        }
        
        buffer[n] = '\0';
        printf("收到來自 %s:%d 的消息: %s",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port),
               buffer);
        
        // 回顯
        sendto(sockfd, buffer, n, 0,
               (struct sockaddr*)&client_addr, addr_len);
    }
    
    close(sockfd);
    return 0;
}
