/*
 * 檔案名稱: select_server.c
 * 功能說明: 使用 select 實現的簡單服務器
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>

#define PORT 9000
#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024

int main(void) {
    int listen_fd, client_fds[MAX_CLIENTS] = {0};
    struct sockaddr_in server_addr;
    fd_set readfds;
    int max_fd, activity, i, new_socket, valread;
    char buffer[BUFFER_SIZE];
    
    // 創建監聽socket
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(listen_fd, 3);
    
    printf("Select 服務器啟動，端口 %d\n", PORT);
    
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(listen_fd, &readfds);
        max_fd = listen_fd;
        
        // 添加所有客戶端socket到集合
        for (i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_fds[i];
            if (sd > 0) FD_SET(sd, &readfds);
            if (sd > max_fd) max_fd = sd;
        }
        
        // 等待活動
        activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        (void)activity;  /* 抑制未使用警告 */

        // 檢查新連接
        if (FD_ISSET(listen_fd, &readfds)) {
            new_socket = accept(listen_fd, NULL, NULL);
            printf("新連接: socket %d\n", new_socket);
            
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (client_fds[i] == 0) {
                    client_fds[i] = new_socket;
                    break;
                }
            }
        }
        
        // 檢查客戶端數據
        for (i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_fds[i];
            if (sd > 0 && FD_ISSET(sd, &readfds)) {
                valread = read(sd, buffer, BUFFER_SIZE);
                if (valread == 0) {
                    printf("斷開連接: socket %d\n", sd);
                    close(sd);
                    client_fds[i] = 0;
                } else {
                    buffer[valread] = '\0';
                    printf("收到[%d]: %s", sd, buffer);
                    ssize_t sent = send(sd, buffer, valread, 0);
                    if (sent == -1) {
                        perror("send");
                    }
                }
            }
        }
    }
    
    return 0;
}
