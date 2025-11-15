/*
 * 檔案名稱: poll_server.c
 * 功能說明: 使用 poll 實現的簡單服務器
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>

#define PORT 9001
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

int main(void) {
    int listen_fd, new_fd, nfds = 1;
    struct sockaddr_in server_addr;
    struct pollfd fds[MAX_CLIENTS + 1];
    char buffer[BUFFER_SIZE];
    
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(listen_fd, 3);
    
    printf("Poll 服務器啟動，端口 %d\n", PORT);
    
    // 初始化poll數組
    fds[0].fd = listen_fd;
    fds[0].events = POLLIN;
    
    while (1) {
        int ret = poll(fds, nfds, -1);
        (void)ret;  /* 抑制未使用警告 */

        // 檢查監聽socket
        if (fds[0].revents & POLLIN) {
            new_fd = accept(listen_fd, NULL, NULL);
            printf("新連接: socket %d\n", new_fd);
            fds[nfds].fd = new_fd;
            fds[nfds].events = POLLIN;
            nfds++;
        }
        
        // 檢查客戶端socket
        for (int i = 1; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                int n = read(fds[i].fd, buffer, BUFFER_SIZE);
                if (n <= 0) {
                    printf("斷開: socket %d\n", fds[i].fd);
                    close(fds[i].fd);
                    fds[i] = fds[nfds-1];
                    nfds--;
                    i--;
                } else {
                    buffer[n] = '\0';
                    printf("收到[%d]: %s", fds[i].fd, buffer);
                    send(fds[i].fd, buffer, n, 0);
                }
            }
        }
    }
    
    return 0;
}
